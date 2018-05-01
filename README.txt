This module provides functionality to allocate kernel memory

compile: $make
load: kldload kmalloc.ko

test: use the provided test program

gcc -o test test.c

this program doesn't handle any errors. However it will display the address of the allocated buffer if the buffer can be created. From the address range, we can tell if the allocation is done in the kernel or not. 


