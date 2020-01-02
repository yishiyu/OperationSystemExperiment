//=========================
//  author: bibibi
//  description: 进程间管道通信
//=========================
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include<sys/file.h>
int pid1, pid2;                                                     // 定义两个进程变量
int process_pause;
void restart();
main(){

    int fd[2];
    char OutPipe[100], InPipe[100];                                          // 定义两个字符数组
    pipe(fd);                                                                                       // 创建管道
    process_pause=1;                                                                    //创建用来阻塞的变量

    while ((pid1 = fork()) == -1);                                                  //创建子进程1
    if (pid1 == 0){                                                                                                               //子进程1创建成功

        signal(17,restart);                                                                                                   //使用变量锁定子进程
        while(process_pause);

        flock(fd[1],LOCK_EX|LOCK_NB);                                                                       // 锁定管道写入端
        sprintf(OutPipe, "\n Child process 1 is sending message!\n");               // 给Outpipe赋值
        write(fd[1],OutPipe,strlen(OutPipe));                                                           // 向管道写入数据
        flock(fd[1],LOCK_UN);                                                                                        // 解除管道的锁定

        exit(0);                                                                                                                     // 结束子进程1
    }
    else{                                                                                             //父进程路径
        while ((pid2 = fork()) == -1);                                             //创建子进程2
        if (pid2 == 0){                                                                                                          //子进程2创建成功

            signal(17,restart);                                                                                              //使用变量锁定子进程
            while(process_pause);

            flock(fd[1],LOCK_EX|LOCK_NB);                                                                  //锁定管道写入端
            sprintf(OutPipe, "\n Child process 2 is sending message!\n");          //给Outpipe赋值
            write(fd[1], OutPipe, strlen(OutPipe));                                                     // 向管道写入数据
            flock(fd[1],LOCK_UN);                                                                                    // 解除管道的锁定

            exit(0);                                                                                                                // 结束子进程2
        }
        else{                                                                                      //父进程路径
            sleep(1);                                                                         //等待子进程就绪

            kill(pid1,17);                                                                  //唤醒子进程1
            read(fd[0],InPipe,sizeof(InPipe));                           //从管道中读出数据
            printf("%s\n", InPipe);                                               // 显示读出的数据

            kill(pid2,17);                                                                //唤醒子进程2
            read(fd[0],InPipe,sizeof(InPipe));                           //从管道中读出数据
            printf("%s\n", InPipe);                                               // 显示读出的数据

            exit(0);                                                                            // 父进程结束
        }
    }
}

//重启子进程
void restart(){
    process_pause=0;
}