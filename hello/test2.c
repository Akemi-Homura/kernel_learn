#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

# define __NR_mycall 378

int main()
{
        unsigned long x = 0;
        x = syscall(__NR_mycall);        //测试378号系统调用
        printf("Hello, %ld\n", x);
        return 0;
}
