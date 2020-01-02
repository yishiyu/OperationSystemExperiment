#include<stdio.h>
int main()
{
    //缓冲区定义
    char CPU_buf[1024]={0};
    char Version_buf[1024]={0};
    char Memory_buf[1024]={0};
    char Process_buf[1024]={0};
    

    //调用系统调用填充缓冲区
    syscall(333,CPU_buf,Version_buf,Memory_buf,Process_buf,20);

    //输出系统信息
    printf("\r\n===================CPU INFO===================\r\n");
    printf("%s \r\n",CPU_buf);
    printf("\r\n==================Version INFO=================\r\n");
    printf("%s\r\n",Version_buf);
    printf("\r\n==================MemoryINFO=================\r\n");
    printf("%s\r\n",Memory_buf);
    printf("\r\n==================Process INFO=================\r\n");
    printf("%s\r\n",Process_buf);
}