//=========================
//  author: bibibi
//  description: LRU_PAGE_REPLACE
//=========================
#include<stdio.h>
#include<time.h>
#include<stdlib.h>

#define LRU_WORK 1

#define ap 100                                         //进程页面数
#define pp 30                                           //内存分配页面数
#define total_instruction 100             //记录所有页面总的命中次数
static int diseffect=0;                             //记录未命中次数,即缺页中断发生次数

typedef struct LRU_PAGE                     //记录使用记录的双向链表
{
    int control_index;                                            //内存分页数组的下标
    int page_index;                                                //记录在内存分配页面中的下标
    struct LRU_PAGE* preptr;                           //指向使用时间更近的块
    struct LRU_PAGE* postptr;                        //指向使用时间更远的块
}LRU_PAGE ;



//LRU页面置换
int LRU(){
    int page[ap];                                                   //进程页面
    LRU_PAGE pagecontrol[pp];                    //内存分配的页面

     //进程操作的顺序
    int work[total_instruction]={
        11,15,14,12,13,16,19,17,15,11,13,16,17,18,15,11,19,12,17,15,
        51,55,54,52,53,56,59,57,55,51,53,56,57,58,55,51,59,52,57,55,
        81,85,84,82,83,86,89,87,85,81,83,86,87,88,85,81,89,82,87,85,
        31,35,34,32,33,36,39,37,35,31,33,36,37,38,35,31,39,32,37,35,
        1,5,4,2,3,6,9,7,5,1,3,6,7,8,5,1,9,2,7,5
    };
    int work_index=0;                       //进程当前操作的下标

    srand((int)time(0));                                                         //初始化随机种子
    for(int i=0;i<ap;i++) page[i]=-1;                                    //初始化进程页面

    if(!LRU_WORK){                                                                     //选择随机执行
        for(int i=0;i<total_instruction;i++)                             //初始化进程操作的顺序
            work[i]=(int)(total_instruction*(rand()/(RAND_MAX+1.0)));
    }

    for(int i=0;i<pp-1;i++){                                                         //初始化内存分配的页面
        pagecontrol[i].control_index=i;
        pagecontrol[i].page_index=-1;
        pagecontrol[i].postptr=&pagecontrol[i+1];
        pagecontrol[i+1].preptr=&pagecontrol[i];
    }
    pagecontrol[pp-1].control_index=pp-1;
    pagecontrol[pp-1].page_index=-1;

    LRU_PAGE head,tail;                     //声明空白的头和尾节点
    head.postptr=&pagecontrol[0];
    tail.preptr=&pagecontrol[pp-1];
    pagecontrol[0].preptr=&head;
    pagecontrol[pp-1].postptr=&tail;
    int temp_index=0;

    //算法执行过程
    while(work_index<total_instruction){                    //如果存在下一个进程操作

        //lfind(work[work_index], base, &pp, sizeof(pagecontrol[0]), equal) == NULL
       temp_index = page[work[work_index]];

        if(temp_index!=-1){          //如果在内存分配页面数组中存在
            work_index++;
            
            //命中,调整常用块的双向链表

            //如果命中第一个节点,则直接跳过即可
            if(temp_index==head.postptr->control_index) continue;

            (pagecontrol[temp_index].preptr)->postptr=(pagecontrol[temp_index].postptr);
            (pagecontrol[temp_index].postptr)->preptr=(pagecontrol[temp_index].preptr);
            pagecontrol[temp_index].preptr=&head;
            pagecontrol[temp_index].postptr=head.postptr;
            head.postptr=&(pagecontrol[temp_index]);
            (pagecontrol[temp_index].postptr)->preptr=&pagecontrol[temp_index];
        }
        else{                           //未命中
            
            (tail.preptr)->page_index=work[work_index];                     //将进程块放入链表最后一个节点
            page[work[work_index]]=(tail.preptr)->control_index;    

            (head.postptr)->preptr=tail.preptr;
            (tail.preptr)->postptr=head.postptr;                                        //将最后一个节点转移到第一个节点
            head.postptr=tail.preptr;
            tail.preptr=(tail.preptr)->preptr;
            (tail.preptr)->postptr=&tail;
            (head.postptr)->preptr=&head;

            diseffect++;
            work_index++;
        }
    }

    printf("总命中率: %f \r\n",(1-((float)diseffect)/(float)total_instruction));                                 //输出命中率
}

int main(){
    
    //执行LRU策略的页面替换
    LRU();

    return 0;
}



