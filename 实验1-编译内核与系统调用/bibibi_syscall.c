#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/fcntl.h>
#include <linux/string.h>


//定义系统调用号和系统调用表地址
#define my_syscall_num 279
#define sys_call_table_adress 0xffffffff81a001c0        
//使用cat /boot/System.map-`uname -r`|grep sys_call_table命令查看系统调用表在内存中的位置

unsigned int clear_and_return_cr0(void);
void setback_cr0(unsigned int val);
asmlinkage long sys_mycall(
    char *user_cpu,char * user_version,char *user_memory, char *user_process,int user_processID);
//    int len_cpu, int len_version,int len_memory, int len_process);
int orig_cr0;
unsigned long *sys_call_table = 0;
static int (*anything_saved)(void);

//修改cr0超级写权限
unsigned int clear_and_return_cr0(void)
{
    unsigned int cr0 = 0;
    unsigned int ret;
    
    asm("movq %%cr0, %%rax"
        : "=a"(cr0));
    ret = cr0;

    cr0 &= 0xfffffffffffeffff;
    asm("movq %%rax, %%cr0" ::"a"(cr0));

    //修改前:0x80050033
    //修改后:0x80040032
    //16位为写保护位
    //0x00010000
    //最低位改变=>无问题
    //说明不是因为写保护位的原因导致无法无法读取内存

    return ret;
}

//复位cr0寄存器
void setback_cr0(unsigned int val) //读取val的值到eax寄存器，再将eax寄存器的值放入cr0中
{
    asm volatile("movq %%rax, %%cr0" ::"a"(val));
}


//初始化
static int __init init_addsyscall(void)
{
    sys_call_table = (unsigned long *)sys_call_table_adress; //获取系统调用服务首地址
    anything_saved = (int (*)(void))(sys_call_table[my_syscall_num]); //保存原始系统调用的地址
    orig_cr0 = clear_and_return_cr0(); //设置cr0可更改
    sys_call_table[my_syscall_num] = (unsigned long )(&sys_mycall); //更改原始的系统调用服务地址
    setback_cr0(orig_cr0); //设置为原始的只读cr0
    return 0;
}


//系统调用的实现
//CPU型号
//操作系统的版本号
//系统中的进程等类似于Windows的任务管理器的信息
asmlinkage long sys_mycall(
    char *user_cpu,     char * user_version,     char *user_memory,     char *user_process, int user_processID)
//    int len_cpu, int len_version,int len_memory, int len_process);
{
    struct file *file_pointer;                             //文件结构体指针
    mm_segment_t file_segment;                 //内存环境设置变量
    loff_t pos;                                                       //读写指针
    char buffer[1000];                                       //缓冲区
    int len_buffer=sizeof(buffer)-1;             //缓冲区大小-1
    int len_read=len_buffer;                           //目标读取数
    ssize_t byte_read=0;                                   //实际读取数，用来清空缓冲区
    
    //将内存设置为内核空间并保存原本内存空间
    file_segment=get_fs();
    set_fs(KERNEL_DS);


    //读取CPU信息
    file_pointer=filp_open("/proc/cpuinfo",O_RDONLY,0);                       //打开文件
    pos=0;                                                                                                                   //设置读开始位置
    //len_read=(len_buffer>len_cpu)?len_cpu:len_buffer;                           //计算合法的最大读取数
    byte_read = vfs_read(file_pointer,buffer,len_read,&pos);                //将数据读入缓冲区
    buffer[byte_read]=0;                                                                                       //保证在正确的位置以0结束
    copy_to_user(user_cpu,buffer,sizeof(buffer)+1);                                 //将输入送往用户空间
    filp_close(file_pointer,NULL);                                                                      //cpu信息读取结束 关闭文件

    //读取Version信息
    file_pointer=filp_open("/proc/version",O_RDONLY,0);                       //打开文件
    pos=0;                                                                                                                   //设置读开始位置
    //len_read=(len_buffer>len_version)?len_version:len_buffer;             //计算合法的最大读取数
    byte_read = vfs_read(file_pointer,buffer,len_read,&pos);                //将数据读入缓冲区
    buffer[byte_read]=0;                                                                                       //保证在正确的位置以0结束
    copy_to_user(user_version,buffer,sizeof(buffer));                               //将输入送往用户空间
    filp_close(file_pointer,NULL);                                                                      //version信息读取结束 关闭文件

    //读取Memory信息
    file_pointer=filp_open("/proc/meminfo",O_RDONLY,0);                     //打开文件
    pos=0;                                                                                                                    //设置读开始位置
    //len_read=(len_buffer>len_memory)?len_memory:len_buffer;          //计算合法的最大读取数
    byte_read = vfs_read(file_pointer,buffer,len_read,&pos);                  //将数据读入缓冲区
    buffer[byte_read]=0;                                                                                       //保证在正确的位置以0结束
    copy_to_user(user_memory,buffer,sizeof(buffer));                             //将输入送往用户空间
    filp_close(file_pointer,NULL);                                                                      //memory信息读取结束 关闭文件

    //读取进程信息
    char process_dir[30]="/proc/";
    char temp_char[10]="/status";
    char temp_ID[10]="";
    int index=0,temp_num=user_processID;
    for(index=0;temp_num>9;index++,temp_num/=10);
    while(index>=0){
        temp_ID[index]=(user_processID%10)+48;
        index--;
        user_processID/=10;
    }
    strcat(process_dir,temp_ID);
    strcat(process_dir,temp_char);

    //char final[]="/proc/20/status";

    file_pointer=filp_open( process_dir,O_RDONLY,0);                               //打开文件
    file_pointer=filp_open( process_dir,O_RDONLY,0);                               //打开文件
    pos=0;                                                                                                                    //设置读开始位置
    //len_read=(len_buffer>len_process)?len_process:len_buffer;          //计算合法的最大读取数
    byte_read = vfs_read(file_pointer,buffer,len_read,&pos);                 //将数据读入缓冲区
    buffer[byte_read]=0;                                                                                       //保证在正确的位置以0结束
    //copy_to_user(user_process,process_dir,sizeof(buffer));                             //将输入送往用户空间
    copy_to_user(user_process,buffer,sizeof(buffer));                             //将输入送往用户空间
    filp_close(file_pointer,NULL);                                                                      //memory信息读取结束 关闭文件


    //运行结束前将内存设置回用户空间
    set_fs(file_segment);
    
    return current->pid;
}


//清理工作
static void __exit exit_addsyscall(void)
{
    orig_cr0 = clear_and_return_cr0(); //设置cr0可更改
    sys_call_table[my_syscall_num] = (unsigned long)anything_saved;
    setback_cr0(orig_cr0);
    printk("module unloaded\n");
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);
MODULE_LICENSE("Dual BSD/GPL");
