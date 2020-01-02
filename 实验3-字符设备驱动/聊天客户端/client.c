//=========================
//  author: bibibi
//  description: 使用字符设备通信
//=========================

//使用bibibi_device字符设备
//实现一个可以实现一对多通信的客户端
//每个客户端地位相等,每个客户端开启时需要进行一个命名
//每个客户端发送一个信息的时候其他所有客户端都会收到信息
//使用写者读者模型,写者优先
//
//每个客户端可以执行两个操作——读或者写
//每次写操作将覆盖设备全部的缓冲区
//
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/un.h> 
#include <signal.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_BUFFER 64
#define WRITER 0
#define READER 1
#define SHARED_MEM 111

//用户名变量(暂时不确保所有用户不重名)
static char my_name[16];
static char buf[MAX_BUFFER]={0};
static char prebuf[MAX_BUFFER]={0};
static char exit_signal[]="quit";
static int fd=0;

//更改客户端模式
static int running=1;
static int mode=READER;
void change_mode();
void client_write();
void client_read();

// wmutex:semaphore=1    //读者与写者之间、写者与写者之间互斥使用共享数据
// S：semaphore=1        //当至少有一个写者准备访问共享数据时，它可使后续的读者等待写完成
// S2：semaphore＝1      //阻塞第二个以后的等待读者
// Readcount,writecount: int = 0,0;  //当前读者数量、写者数量
// mutex1 :semaphore =  1      //多个读者互斥使用 readcount
// mutex2 :semaphore = 1       //多个写者互斥使用 writecount

//同步信号量
struct client_syn{
    sem_t data_available;                       //操作权限      相当于wmutex
    sem_t writer_syn;                               //写者操作时的阻塞信号量    相当于S
    sem_t reader_syn;                               //读者操作时的阻塞信号量    相当于S2
    int reader_count;                              //读者计数器
    int writer_count;                               //写者计数器
    sem_t reader_count_en;               //读者计数器操作信号量
    sem_t writer_count_en;               //读者计数器操作信号量
}client_syn;
static struct client_syn *syn_var;

//单进程客户端
int main(char argc,char *argv[]){

    signal(SIGQUIT , change_mode);                                             //设置键盘中断  

    //设置公共信号量
    int shared_mem = shmget((key_t)SHARED_MEM, sizeof(client_syn), 0666 | IPC_CREAT);
    void *shm = shmat(shared_mem, 0, 0);
    syn_var=(struct client_syn*)shm;

    //如果有参数则初始化公共信号量
    if(argc>1){
        //每次只能有一个进程进行操作
        sem_init(&(syn_var->data_available), 1, 1); 
        sem_init(&(syn_var->writer_syn), 1, 1); 
        sem_init(&(syn_var->reader_syn), 1, 1); 
        sem_init(&(syn_var->reader_count_en), 1, 1); 
        sem_init(&(syn_var->writer_count_en), 1, 1); 
        //初始化chengggong
        printf("init success!\n");
    }

    //获取用户名称
    printf("please input your name:\t");
    // scanf("%10[a-z]",&my_name);

    char temp[3]=":";
    scanf("%10[a-z]",my_name);
    strncat(&my_name,temp,2);
    //打开文件
    fd = open("/dev/bibibi_device",O_RDWR);
    if (fd == -1) {
        printf("open bibibi_device failed!\n");
        return -1;
    }

    printf("====================client %s start================\n",my_name);

    while(running){
        sleep(1);                           //更新频率为1Hz
        switch (mode){
        case WRITER:
            client_write();
            break;
        case READER:
            client_read();
            break;
        default:
            break;
        }
    }

    printf("====================client %s exit================\n",my_name);

    //清理工作


    return 0;
}

//切换模式
void change_mode(){
    mode=1-mode;
    printf("\b\bplease type in\n");
}


//写入函数
void client_write(){

        // P(mutex2);   // 申请访问 writecount
        // if writecount=0 then P(S);  // 写者进程进入等待队列
        // writecount++; 
        // V(mutex2）;   // 释放 writecount
        // P(wmutex);  // 是否有读者或者写者正在操作
        // writing …
        // V(wmutex);   // 写操作完成
        // P(mutex2);  
        // writecount--;
        // if writecount=0 then V(S);    // 没有写者在等待，允许读者进程进入操作     
        // V(mutex2）;
    
    // sem_t data_available;                       //操作权限      相当于wmutex
    // sem_t writer_syn;                               //写者操作时的阻塞信号量    相当于S
    // sem_t reader_syn;                               //读者操作时的阻塞信号量    相当于S2
    // int reader_count;                              //读者计数器
    // int writer_count;                               //写者计数器
    // sem_t reader_count_en;               //读者计数器操作信号量
    // sem_t writer_count_en;               //读者计数器操作信号量

    //同步操作
    sem_wait(&(syn_var->writer_count_en));
    if(syn_var->writer_count==0) sem_wait(&(syn_var->writer_syn));              //阻塞读者操作
    syn_var->writer_count++;
    sem_post(&(syn_var->writer_count_en));
    sem_wait(&(syn_var->data_available));


    //读取输入
    printf("%s",my_name);
    setbuf(stdin,NULL);
    scanf("%50[^\n]",buf);
        
    if(strcmp(buf,exit_signal)==0){
        printf("client quit\n");
        running=0;
    }

    write(fd,my_name,strlen(my_name));
    lseek(fd, strlen(my_name), SEEK_SET);
    write(fd,buf, sizeof(buf));
    lseek(fd, 0, SEEK_SET);

    read(fd, prebuf, sizeof(prebuf));
    lseek(fd, 0, SEEK_SET);
    mode=READER;


    sem_post(&(syn_var->data_available));
    sem_wait(&(syn_var->writer_count_en));
    syn_var->writer_count--;
    if(syn_var->writer_count==0) sem_post(&(syn_var->writer_syn));              //释放读者操作
    sem_post(&(syn_var->writer_count_en));
}

//读取函数
void client_read(){

        // P(S2);     // 是否已经有读者进程在等待队列中
        // P(S);      // 是否有写者进程在等待队列中
        // P(mutex1);   // 没有其他的读者进程在访问 readcount
        // if readcount=0 then P(wmutex);   // 判断是否有写者进程在写
        // readcount++;
        // V(mutex1);   // 释放 readcount
        // V(S);      // 允许写者进程等待
        // V(S2);    // 允许读者进程等待
        // reading …
        // P(mutex1);  
        // readcount--;
        // if readcount=0 then V(wmutex);  //  允许写者进程写
        // V(mutex1);    // 释放 readcount

    // sem_t data_available;                       //操作权限      相当于wmutex
    // sem_t writer_syn;                               //写者操作时的阻塞信号量    相当于S
    // sem_t reader_syn;                               //读者操作时的阻塞信号量    相当于S2
    // int reader_count;                              //读者计数器
    // int writer_count;                               //写者计数器
    // sem_t reader_count_en;               //读者计数器操作信号量
    // sem_t writer_count_en;               //读者计数器操作信号量


    //同步操作
    sem_wait(&(syn_var->reader_syn));
    sem_wait(&(syn_var->writer_syn));
    sem_wait(&(syn_var->reader_count_en));
    if(syn_var->reader_count==0) sem_wait(&(syn_var->data_available));          //读的时候禁止写入
    syn_var->reader_count++;
    sem_post(&(syn_var->reader_count_en));
    sem_post(&(syn_var->writer_syn));
    sem_post(&(syn_var->reader_syn));

    //读取文件
    read(fd, buf, sizeof(buf));
    lseek(fd, 0, SEEK_SET);

    //如果两次内容不完全一致则输出
    if(strcmp(buf,prebuf)!=0){
        printf("%s\n",buf);
        strncpy(prebuf,buf,MAX_BUFFER);
    }

    sem_wait(&(syn_var->reader_count_en));
    syn_var->reader_count--;
    if(syn_var->reader_count==0) sem_post(&(syn_var->data_available));          //释放写权限
    sem_post(&(syn_var->reader_count_en));
}



//int *bibibi_read(void *ptr);                             //读取内容
// int *bibibi_write(void *ptr);                           //写入内容
// void get_input();                                               //开启一个输入
// int fd;

// //读写操作互斥 
// static int running=1;
// static int writing=0;
// static int inputing=0;
// static char buf[MAX_BUFFER]={0};
// static char prebuf[MAX_BUFFER]={0};


    // //声明发送线程和接收线程
    // pthread_t bibibi_sender,bibibi_reciver;
    // int ret = pthread_create(&bibibi_sender,NULL,bibibi_write,NULL);
    // if(ret!=0){
    //     printf("创建sender错误");
    //     exit(1);
    // }
    // ret = pthread_create(&bibibi_reciver,NULL,bibibi_read,NULL);
    // if(ret!=0){
    //     printf("创建reciver错误");
    //     exit(1);
    // }

    // pthread_join(bibibi_sender,NULL);
    // pthread_join(bibibi_reciver,NULL);


// //读取内容
// int *bibibi_read(void *ptr){
//     printf("bibibi_read start\n");

//     //程序持续运行
//     while (running){
//         sleep(1);
//         //读取进程阻塞在写入进程运行的时候
//         while(writing);


//         //读取文件
//         read(fd, buf, sizeof(buf));
//         lseek(fd, 0, SEEK_SET);

//         //如果两次内容不完全一致则输出
//         if(strcmp(buf,prebuf)!=0){
//             printf("%s\n",buf);
//             strncpy(prebuf,buf,MAX_BUFFER);
//         }

//     }
//     return 0;
// }

// int *bibibi_write(void *ptr){
//     printf("bibibi_write start\n");

//     //程序持续运行
//     while (running){
//         while(!inputing);
//         inputing=0;
//         writing=1;

//         //读取输入
//         printf("%s",my_name);
//         setbuf(stdin,NULL);
//         scanf("%50[a-z]",buf);
        
//         if(strcmp(buf,exit_signal)==0){
//             printf("client quit\n");
//             running=0;
//         }


//         //写入文件
//         fd = open("/dev/bibibi_device",O_RDWR);
//         if (fd == -1) {
//             printf("open memdev failed!\n");
//             return -1;
//         }

//         write(fd,my_name,strlen(my_name));
//         lseek(fd, strlen(my_name), SEEK_SET);
//         write(fd,buf, sizeof(buf));
//         lseek(fd, 0, SEEK_SET);
//         writing=0;
//     }
//     return 0;
// }



// void get_input(){
//     printf("\b\bplease type in\n");
//     inputing=1;
// }
    // int fd;
    // char buf[4096];

    // strcpy(buf,"memory simulate char device test...\n");
    // printf("original buf:%s\n",buf); 

    // fd = open("/dev/bibibi_device",O_RDWR);
    // if (fd == -1) {
    //     printf("open memdev failed!\n");
    //     return -1;
    // }
    // write(fd, buf, sizeof(buf));
    // lseek(fd, 0, SEEK_SET);
    // strcpy(buf, "nothing here");
    // read(fd, buf, sizeof(buf));
    // printf("bibibi_read buf:%s\n", buf);
