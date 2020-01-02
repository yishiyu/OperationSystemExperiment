#!/bin/bash

sudo make
sudo insmod ./bibibi_syscall.ko
gcc ./test.c -o test.o
sudo ./test.o
sudo rmmod bibibi_syscall
