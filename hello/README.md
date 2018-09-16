# 自定义系统调用

## 方案一

#### 1. 实现思路

编写一个可加载模块，在可加载模块新建时修改`sys_call_table`中系统调用序号与服务例程的对应关系，将自定义的服务例程绑定一个系统调用序号。则可以在其他进程中通过系统调用序号执行自定义系统调用

#### 2. 测试环境

`Ubuntu 16.04 i386 4.4.0-92-generic`

## 操作流程

#### 1. 查看sys_call_table的内存地址

`sys_call_table`的内存地址，存放在`/boot/System.map-${uname -r}`中，`uname -r`为内核版本号。使用以下命令查看

```shell
sudo cat /boot/System.map-`uname -r` | grep sys_call_table
```

![系统调用表地址](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-15-123426.jpg)

图中可知 `0xc17c91c0` 为`sys_call_table`在内存中的地址



#### 2. 查找未使用的系统调用序号

系统调用序号的使用情况存放在`/usr/include/i386-linux-gnu/asm/unistd_32.h`中

![未使用的系统调用号](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-15-123424.jpg)

图中可知222和223号未使用

#### 3. 编写可加载模块

```c
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/sched.h>

MODULE_LICENSE("Dual BSD/GPL");

#define SYS_CALL_TABLE_ADDRESS 0xc17c91c0  //sys_call_table对应的地址
#define NUM 223  //系统调用号为223
int orig_cr0;  //用来存储cr0寄存器原来的值
unsigned long *sys_call_table_my=0;

static int(*anything_saved)(void);  //定义一个函数指针，用来保存一个系统调用

static int clear_cr0(void) //使cr0寄存器的第17位设置为0（内核空间可写）
{
    unsigned int cr0=0;
    unsigned int ret;
    asm volatile("movl %%cr0,%%eax":"=a"(cr0));//将cr0寄存器的值移动到eax寄存器中，同时输出到cr0变量中
    ret=cr0;
    cr0&=0xfffeffff;//将cr0变量值中的第17位清0,将修改后的值写入cr0寄存器
    asm volatile("movl %%eax,%%cr0"::"a"(cr0));//将cr0变量的值作为输入，输入到寄存器eax中，同时移动到寄存器cr0中
    return ret;
}

static void setback_cr0(int val) //使cr0寄存器设置为内核不可写
{
    asm volatile("movl %%eax,%%cr0"::"a"(val));
}

asmlinkage long sys_mycall(void) //定义自己的系统调用
{   
    printk("hello world\n");
    return current->pid;    
}
static int __init call_init(void)
{
    sys_call_table_my=(unsigned long*)(SYS_CALL_TABLE_ADDRESS);
    printk("call_init......\n");
    anything_saved=(int(*)(void))(sys_call_table_my[NUM]);//保存系统调用表中的NUM位置上的系统调用
    orig_cr0=clear_cr0();//使内核地址空间可写
    sys_call_table_my[NUM]=(unsigned long) &sys_mycall;//用自己的系统调用替换NUM位置上的系统调用
    setback_cr0(orig_cr0);//使内核地址空间不可写
    return 0;
}

static void __exit call_exit(void)
{
    printk("call_exit......\n");
    orig_cr0=clear_cr0();
    sys_call_table_my[NUM]=(unsigned long)anything_saved;//将系统调用恢复
    setback_cr0(orig_cr0);
}

module_init(call_init);
module_exit(call_exit);

MODULE_AUTHOR("25");
MODULE_VERSION("BETA 1.0");
MODULE_DESCRIPTION("a module for replace a syscall");
```

该代码的作用是将223号系统调用绑定为自定义的系统调用，即输出"hello world\n"，并返回调用进程的pid

#### 4. 编写Makefile

```makefile
obj-m:=hello.o
CURRENT_PATH:=$(shell pwd)
LINUX_KERNEL_PATH:=/usr/src/linux-headers-$(shell uname -r)
all:
	make -C  $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C  $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
```

#### 5. 编写测试代码

```c
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

# define __NR_mycall 223

int main()
{
        unsigned long x = 0;
        x = syscall(__NR_mycall);        //测试223号系统调用
        printf("Hello, %ld\n", x);
        return 0;
}
```



#### 6. 运行调试

![编译-加载模块](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-15-123427.jpg)

编译，加载模块，运行测试程序，输出了该进程的pid

![调试信息](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-15-123425.jpg)

使用`dmesg`查看输出信息，发现输出了"hello world\n"，系统调用被成功执行



## 方案二

#### 1. 实现思路

下载内核源码，加入新的系统调用，编译安装新的内核



#### 2. 测试环境

`Ubuntu 16.04 i386 4.4.0-92-generic`



## 操作流程

#### 1. 下载内核源码

方便起见，下载apt软件源中的内核源码

查看可用的内核的源码

`apt-cache search linux-source`

![可用内核源码](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-16-145246.jpg)

下载`4.4.0`的内核源码

`sudo apt install linux-source-4.4.0`



#### 2. 修改内核代码，增加系统调用

在`include/linux/syscalls.h`文件中添加系统调用的函数声明

```c
asmlinkage long sys_hello(void);
```

在`kernel/sys.c`中添加系统调用的函数实现

```c
asmlinkage long sys_hello(void) {
        printk("hello world xll\n");
        return current->pid;
}
```

在`arch/x86/entry/syscalls/syscall_64.tbl`中添加系统调用号

```c
378     i386    hello                   sys_hello
```



#### 3. 编译内核

编译镜像，编译模块，安装模块，安装内核

```shell
make bzImage -j2 && make modules -j2 && make modules_install && make install
```

更新grup

```shell
update-grub
```

重新启动，检查是否成功安装

![新的内核版本号](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-16-145248.jpg)

内核版本号为下载的内核版本号，安装成功



#### 4. 测试系统调用是否增加成功

编写测试程序

```c
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

# define __NR_mycall 378

int main()
{
        unsigned long x = 0;
        x = syscall(__NR_mycall);        //测试223号系统调用
        printf("Hello, %ld\n", x);
        return 0;
}
```

运行，查看调试信息

![测试2](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-16-145247.jpg)

![dmesg2](http://7xqsr4.com1.z0.glb.clouddn.com/2018-09-16-145249.jpg)

成功输出进程pid，成功打印调试信息

