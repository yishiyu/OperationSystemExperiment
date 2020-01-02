//=========================
//  author: bibibi
//  description: 进程软中断实验
//=========================
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
int wait_flag;                          //全局阻塞变量
void stop();
void parent_stop();
main(){
    
    int pid1, pid2;                                                                                 // 定义两个进程号变量
    signal(SIGINT, parent_stop);                                                                    //设置键盘中断
    //signal(SIGTSTP, stop);                                                                 //设置键盘中断
    wait_flag=1;

    while ((pid1 = fork()) == -1);                                                                            // 创建子进程1
    
    if (pid1 > 0){                                                                                  //父进程路径

        while ((pid2 = fork()) == -1);                                                                                                // 创建子进程2
        if (pid2 > 0){                                                                              //父进程路径

            sleep(5);                                                                               //父进程阻塞
            
            kill(pid1, 16);     		                                                          // 杀死进程1发中断号16
            kill(pid2, 17);                                                                       // 杀死进程2发终端号17
            wait(0);                                                                                 // 等待第1个子进程1结束的信号
            wait(0);                                                                                 // 等待第2个子进程2结束的信号
            printf("\n Parent process is killed !!\n");
            exit(0);                                                                                 // 父进程结束
        }
        else{
            signal(17,stop);                                                                                                                 // 等待进程2被杀死的中断号17
            while(wait_flag);                                                                                                               //阻塞子进程2
            printf("\n Child process 2 is killed by parent !!\n");
            exit(0);
        }
    }
    else{                                                                                                                            // 等待进程1被杀死的中断号16
        signal(16, stop); 
        while(wait_flag);                                                                                                 //阻塞子进程1
        printf("\n Child process 1 is killed by parent !!\n");
        exit(0);
    }
}
void stop(){
    printf("child  stop!!!\r\n");
    wait_flag=0;
}
void parent_stop(){
    printf("parent stop!!!\r\n");
}