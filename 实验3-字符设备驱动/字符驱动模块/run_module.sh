#!/bin/bash

sudo make

sudo insmod char_driver.ko bibibi_major=181
sudo mknod /dev/bibibi_device c 181 0

#mknod  创建特殊文件 c表示面向字符 
#mknod Name { b | c } Major Minor 

#删除设备文件可以直接使用rm -f 设备文件名
#rm -f memdev0

