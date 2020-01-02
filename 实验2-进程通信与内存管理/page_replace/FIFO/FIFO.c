//=========================
//  author: bibibi
//  description: FIFO_PAGE_REPLACE
//=========================
#include<stdio.h>
#include<time.h>
#include<stdlib.h>

static int  ap=100;                                      //进程页面数
static int pp=30;                                        //内存分配页面数
static int total_instruction=100;           //记录所有页面总的命中次数
static int diseffect=0;                           //记录未命中次数,即缺页中断发生次数

// 用来判断两数字是否相等的函数
// int equal(int a,int b){
//     return (a==b)?0:-1;
// }

//FIFO页面置换
int FIFO(){
    int page[ap];                                   //进程页面
    int pagecontrol[pp];                    //内存分配的页面
    int page_head=0;                         //队列头
    int page_tail=pp-1;                      //队列尾
    int *base=&pagecontrol;          //内存分配页面的基地址

    int work[total_instruction];     //进程操作的顺序
    int work_index=0;                       //进程当前操作的下标

    srand((int)time(0));                                                         //初始化随机种子
    for(int i=0;i<ap;i++) page[i]=-1;                                   //初始化进程页面
    for(int i=0;i<pp;i++) pagecontrol[i]=-1;                    //初始化内存分配的页面
    for(int i=0;i<total_instruction;i++)                             //初始化进程操作的顺序
        work[i]=(int)(total_instruction*(rand()/(RAND_MAX+1.0)));

    //算法执行过程
    while(work_index<total_instruction){                    //如果存在下一个进程操作

        //lfind(work[work_index], base, &pp, sizeof(pagecontrol[0]), equal) == NULL

        if(page[work[work_index]]!=-1){          //如果在内存分配页面数组中存在
            work_index++;
        }
        else{                           //未命中
            if(page_head==page_tail){                                         //队列已满
                page[pagecontrol[page_tail]]=-1;                                 //取出一个进程页
                page_tail=(page_tail+1)%pp;                                         //队列尾修改
                pagecontrol[page_head]=work[work_index];          //添加进程
                page[work[work_index]]=page_head;                        //加入一个进程
                page_head=(page_head+1)%pp;                                  //队列头修改
            }
            else{                                                                                   //队列未满
                pagecontrol[page_head]=work[work_index];
                page[work[work_index]]=page_head;
                page_head=(page_head+1)%pp;
            }

            diseffect++;
            work_index++;
        }
    }

    printf("总命中率: %f \r\n",(1-((float)diseffect)/(float)total_instruction));                                 //输出命中率
}

int main(){
    
    //执行FIFO策略的页面替换
    FIFO();

    return 0;
}



