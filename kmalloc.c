#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/malloc.h>


struct kmalloc_args{
	unsigned long size;
	unsigned long *addr;
};

//Syscall

static int kmalloc (struct thread *td, void *syscall_args){
	struct kmalloc_args *arguments;
	arguments = (struct kmalloc_args *) syscall_args;
	int error;
	unsigned long addr;
	MALLOC(addr, unsigned long, arguments->size, M_TEMP, M_NOWAIT);
	error = copyout(&addr, arguments->addr, sizeof(addr));
	return (error);


}


//sysent
//
static struct sysent kmalloc_sysent = {
	2,
	kmalloc

};


static int offset = NO_SYSCALL;

//load/unload handler

static int load (struct module * module, int cmd, void *args){
	int error = 0;
	switch (cmd) {
		case MOD_LOAD:
			uprintf("Loaded at %d \n", offset);
			break;
		case MOD_UNLOAD:
			uprintf("Unloaded at %d \n", offset);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	
	}

	return (error);


}

SYSCALL_MODULE(kmalloc, &offset, &kmalloc_sysent, load, NULL);


