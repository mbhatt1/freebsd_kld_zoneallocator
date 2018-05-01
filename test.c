#include <stdio.h>

#include <sys/syscall.h>
#include <sys/types.h>

#include <sys/module.h>

int main (int argc, char *argv[]){
	struct module_stat stat;
	struct module_stat stat1;
	struct module_stat stat2;
	unsigned int addr;
	printf ("Usage ./test only for initialization ./test <sizebuf> 1 for allocation ./test <address> 2 for deallocation \n");

	
	stat.version = sizeof(stat);
//init
	modstat(modfind("sys/mem_init_syscall"), &stat);

	syscall(stat.data.intval);

//allocate memory
	if (argc == 3 && (unsigned int)atoi(argv[2]) == 1){
	stat1.version = sizeof(stat1);
	modstat(modfind("sys/mem_alloc_syscall"), &stat1);

	syscall(stat1.data.intval, (unsigned int)atoi(argv[1]), &addr);

	
	printf("ADDR OF BUFFER 0x%x \n", addr);
	}

//deallocate memory
	if (argc == 3 && (unsigned int) atoi(argv[2]) == 2){
	stat2.version = sizeof(stat2);

	modstat(modfind("sys/mem_free_syscall"), &stat2);

	
	syscall(stat2.data.intval, (unsigned int)atoi(argv[1]));
	exit(0);
	
	}
		exit(0);


}
