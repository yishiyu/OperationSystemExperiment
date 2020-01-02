//=========================
//  author: bibibi
//  description: 字符设备驱动模块
//=========================
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>


// > ============字符驱动模块开始启动========   
// > 1. 实现相关操作函数:open, read, write, close, ioctl  
// > 2. 申请并分配设备号  
// > 3. 创建设备节点(node文件)  
// > 4. 初始化cdev(绑定file_operations与操作函数的具体实现)  
// > 5. 注册cdev  
// > ============字符驱动模块完成启动========  
// > ============字符驱动模块开始卸载========  
// > 6. 注销cdev  
// > 7. 删除设备节点(node文件)  
// > 8. 释放设备号  
// > ============字符驱动设备完成卸载========

//描述设备的结构体
// struct cdev {
//     struct kobject kobj;                                 每个 cdev 都是一个 kobject
//     struct module *owner;                          指向实现驱动的模块
//     const struct file_operations *ops;      操纵这个字符设备文件的方法
//     struct list_head list;                               与 cdev 对应的字符设备文件的 inode->i_devices 的链表头
//     dev_t dev;                                                 起始设备编号
//     unsigned int count;                                 设备范围号大小
// };


//=======================宏定义与功能函数声明===================

//定义设备号,如果定义为0,则动态申请设备号
#ifndef BIBIBI_DEV_MAJOR
#define BIBIBI_DEV_MAJOR  333
#endif

//定义子设备的数量
#ifndef BIBIBI_DEV_NUM
#define BIBIBI_DEV_NUM 2
#endif

//定义字符设备的缓冲区的大小
#ifndef BIBIBI_DEV_SIZE
#define BIBIBI_DEV_SIZE 1024
#endif

//相关操作函数声明
static int bibibi_open(struct inode *inode, struct file *filp);                                                      //打开字符设备
static ssize_t bibibi_read(struct file *filp, char __user *buf, size_t size, loff_t *pos);                      //读取字符设备(一个字节)
static ssize_t bibibi_write(struct file *filp, const char __user *buf, size_t size, loff_t *pos);        //写入字符设备(一个字节)
static loff_t bibibi_llseek(struct file *filp, loff_t offset, int whence);       //控制指针位置
static int bibibi_release(struct inode *inode, struct file *filp);                                                      //关闭字符设备
static int bibibi_ioctl(int fd, int cmd);                                                                              //控制字符设备


//=======================相关的数据结构定义===================
//驱动子设备的结构体
struct bibibi_device{
    char *data;                                     //缓冲区指针
    unsigned long size;                     //缓冲区大小变量
};

//驱动子设备结构体数组的指针
struct bibibi_device *bibibi_devices;

//设备节点文件指针
struct class *bibibi_class;

//驱动模块的cdev
struct cdev bibibi_cdev;

//主设备号
static int bibibi_major=BIBIBI_DEV_MAJOR;
//设定模块向内核传递的参数
//可以通过这一个参数来在加载模块的时候决定设备号,默认设备号为333
module_param(bibibi_major,int, S_IRUGO);

//模块操作定义结构体
static const struct file_operations bibibi_operations={
    .owner=THIS_MODULE,
    .llseek=bibibi_llseek,
    .read=bibibi_read,
    .write=bibibi_write,
    .open=bibibi_open,
    //.ioctl=bibibi_ioctl,          报错说没有这一个成员,先注释掉
    .release=bibibi_release,
};



//初始化字符驱动模块
static int __init init_char_driver(void){
    //2. 申请并分配设备号
    //如果指定了设备号,采用静态分配设备号
    //如果没有指定,采用动态分配设备号

    int result=0;                                                                           //用来接收一些函数的返回值
    dev_t device_num=MKDEV(bibibi_major, 0);            //将主次设备号合成一个设备号


    //申请设备号
    if(bibibi_major){
        //静态申请
        result=register_chrdev_region(device_num, BIBIBI_DEV_NUM, "bibibi_char_dev");
    }
    else{
        //动态申请
        result = alloc_chrdev_region(&device_num, 0, BIBIBI_DEV_NUM, "bibibi_char_dev");
        bibibi_major = MAJOR(device_num);
    }
    
    //申请设备号失败
    if(result<0) return result;


    //3. 创建设备节点(node文件)  
    //静态申请BIBIBI_DEV_NUM个节点 标志为"进程上下文，可以睡眠"
    bibibi_devices=kmalloc(BIBIBI_DEV_NUM * sizeof(struct bibibi_device), GFP_KERNEL);
    if(!bibibi_devices){
        //创建失败,释放 设备号并返回
        result = -ENOMEM;
        unregister_chrdev_region(device_num, BIBIBI_DEV_NUM);
        return result;
    }

    // //创建设备文件
    // bibibi_class = class_create(THIS_MODULE, "bibibi_class");
    
    // printk("error type: %d",IS_ERR(bibibi_class));
    // result = -ENOMEM;
    // unregister_chrdev_region(device_num, BIBIBI_DEV_NUM);
    // return result;

    // if(IS_ERR(bibibi_class)){
    //     printk("Err: failed in creating class.\n");
    //     return -1;
    // }
    // device_create(bibibi_class, NULL, MKDEV(bibibi_major, 0), "bibibi_device",0);



    //填充已经申请的内存空间为0
    //void * memset (void *s ,int c, size_t n);
    memset(bibibi_devices,0,BIBIBI_DEV_NUM * sizeof(struct bibibi_device));

    int i=0;
    for(i=0;i<BIBIBI_DEV_NUM;i++){
        //赋值缓冲区长度变量并填充空间  标志为"进程上下文，可以睡眠"
        bibibi_devices[i].size=BIBIBI_DEV_SIZE;
        bibibi_devices[i].data=kmalloc(BIBIBI_DEV_SIZE,GFP_KERNEL );
        memset(bibibi_devices[i].data,0,BIBIBI_DEV_SIZE);
    }


    //4. 初始化cdev(绑定file_operations与操作函数的具体实现)  
    cdev_init(&bibibi_cdev,&bibibi_operations);
    bibibi_cdev.owner=THIS_MODULE;
    bibibi_cdev.ops=&bibibi_operations;

    //5. 注册cdev 
    cdev_add(&bibibi_cdev,MKDEV(bibibi_major, 0),BIBIBI_DEV_NUM);


    //驱动模块启动成功
    printk("device init success\n");
    return 0;
}


//清理工作
static void __exit exit_char_driver(void){
    // > 6. 注销cdev  
    // > 7. 删除设备节点(node文件)  
    // > 8. 释放设备号  

    //6. 注销cdev
    cdev_del(&bibibi_cdev);

    //7. 删除设备节点
    kfree(bibibi_devices);
    //删除磁盘设备文件
    // device_destroy(bibibi_class, MKDEV(bibibi_major, 0));
    // class_destroy(bibibi_class);

    //8. 释放设备号
    unregister_chrdev_region(MKDEV(bibibi_major, 0), 2);

    //驱动模块卸载完毕
    printk(KERN_INFO "device exit success");
}


//相关参数的具体操作
// static int bibibi_open(struct inode *inode, struct file *filp);                                                      //打开字符设备
// static ssize_t bibibi_read(struct file *filp, char __user *buf, size_t size, loff_t *pos);                      //读取字符设备(一个字节)
// static ssize_t bibibi_write(struct file *filp, const char__user *buf, size_t size, loff_t *pos);        //写入字符设备(一个字节)
// static loff_t bibibi_llseek(struct file *filp, loff_t offset, int whence);       //控制指针位置
// static int bibibi_release(struct inode *inode, struct file *filp);                                                      //关闭字符设备
// static int bibibi_ioctl(int fd, int cmd);                                                                              //控制字符设备

static int bibibi_open(struct inode *inode, struct file *filp){
    struct bibibi_device *device;

    //判断是否已经达到了支持的子设备的上限
    int num=MINOR(inode->i_rdev);
    if(num>=BIBIBI_DEV_NUM)
        return -ENODEV;

    //分配设备
    //device=&bibibi_devices[num];
    //为了实现聊天室,不同的设备请求返回同一个子设备
    //互斥和同步由客户端完成
    device=&bibibi_devices[0];
    filp->private_data=device;

    return 0;
}

static ssize_t bibibi_read(struct file *filp, char __user *buf, size_t size, loff_t *pos){
    unsigned long position=*pos;
    unsigned int count=size;
    int result;
    struct bibibi_device *device=filp->private_data;

    //如果要求位置非法则直接退出
    if(position>=BIBIBI_DEV_SIZE)
        return 0;
    //调整读取数据长度
    if(count>BIBIBI_DEV_SIZE-position)
        count=BIBIBI_DEV_SIZE-position;

    //传送数据给用户
    if(copy_to_user(buf, (void*)(device->data + position), count)) {
        result = -EFAULT;
    } else {
        *pos += count;
        result = count;
        printk(KERN_INFO "read %d bytes from %lu\n", count, position);
    }

    return result;
}

static ssize_t bibibi_write(struct file *filp, const char __user *buf, size_t size, loff_t *pos){
    unsigned long position = *pos;
    unsigned int count = size;
    int result = 0;
    struct bibibi_device *device = filp->private_data;

    //确保写入安全
    if (position >= BIBIBI_DEV_SIZE)
        return 0;
    if (count > BIBIBI_DEV_SIZE-position)
        count = BIBIBI_DEV_SIZE - position;

    //写入数据
    if (copy_from_user(device->data + position, buf, count)) {
        result = -EFAULT;
    } else {
        *pos += count;
        result = count;
        printk(KERN_INFO "write %d bytes from %lu\n", count, position);
    }

    return result;
}

static loff_t bibibi_llseek(struct file *filp, loff_t offset, int whence){
    loff_t newpos;

    //选择操作类型
    switch (whence) {
        //指定新位置
        case 0:
            newpos = offset;
            break;
        //后移指针
        case 1:
            newpos = filp->f_pos + offset;
            break;
        //反向指定新位置
        case 2:
            newpos = BIBIBI_DEV_SIZE - 1 + offset;
            break;
        default:
        return -EINVAL;
    }
    if ((newpos < 0) || (newpos > BIBIBI_DEV_SIZE))
        return -EINVAL;

    filp->f_pos = newpos;
    return newpos;
}

static int bibibi_release(struct inode *inode, struct file *filp){
    //释放子设备操作
    //其实没什么需要做的
    return 0;
}

static int bibibi_ioctl(int fd, int cmd){
    //暂时不支持操纵设备
    return 0;
}

//=======================驱动模块声明==========================

module_init(init_char_driver);
module_exit(exit_char_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yishiyu");
