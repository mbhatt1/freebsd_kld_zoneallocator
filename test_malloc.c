#include <stdio.h>

#include <sys/syscall.h>
#include <sys/types.h>

#include <sys/module.h>

int main (int argc, char *argv[]){
	struct module_stat stat;
	unsigned int addr;

	stat.version = sizeof(stat);

	modstat(modfind("sys/kmalloc"), &stat);

	syscall(stat.data.intval, (unsigned int) atoi(argv[1]), &addr);

	printf("ADDR OF BUFFER 0x%x \n", addr);

	exit(0);

}
