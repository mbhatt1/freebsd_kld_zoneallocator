#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/malloc.h>
/*
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
*/
#include "allocation.h"

#define PAGE_SIZE1 2048
#define PAGE_COUNT (MEM_SIZE / PAGE_SIZE1)
#define PAGE_TABLE ((page_type **) memory)
#define FREE_TABLE_SIZE (log_2(PAGE_SIZE1 / 2) + 1)
#define FREE_TABLE (((page_type **) memory) + PAGE_COUNT)
#define BUSY ((page_type *) 1)
#define MIN_BLOCK_SIZE (sizeof(free_block_type))

//#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define max_block_count(page) ((int)(PAGE_SIZE1 / page->block_size))
#define is_large_block(page) (page->block_size >= PAGE_SIZE1)
#define is_remote_descriptor(size) (size < sizeof(page_type) || size > sizeof(page_type) * 3)
#define MEM_SIZE 65535


static char memory[MEM_SIZE];


struct free_block_struct
{
    struct free_block_struct *next;
};

typedef struct free_block_struct free_block_type;



struct page_struct
{
    size_t block_size;
    free_block_type *free_block;
    int free_block_count;
    struct page_struct *next;
    struct page_struct *prev;
};


typedef struct page_struct page_type;

void *mem_alloc(size_t);
page_type *create_page(size_t);
char *get_page_offset(int);
int get_page_num(void *);
void *mem_alloc_large(size_t);
void mem_free_large(void *);
size_t round_to_4(size_t);
int log_2(int);


int power (int x, int y){
	if (y==0){
		return 1;
	}
	else if (y%2 == 0){
		return power(x, y/2)*power(x, y/2);
	}
	else{
		return x*power(x,y/2)*power(x,y/2);
	}
}
void mem_init()
{
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        PAGE_TABLE[i] = NULL;
    }

    // first page BUSY
    PAGE_TABLE[0] = (page_type *) BUSY;

    for (int i = 0; i < FREE_TABLE_SIZE; ++i)
    {
        FREE_TABLE[i] = NULL;
    }
}



void *mem_alloc(size_t size)
{
    int free_id = log_2((int) size);
    size_t real_size = (size_t)power(2, free_id);

    // size too small
    if(real_size < MIN_BLOCK_SIZE)
    {
        return NULL;
    }

    if(real_size > PAGE_SIZE1 / 2)
    {
        return mem_alloc_large(real_size);
    }

    page_type *page = *(FREE_TABLE + free_id);
    if(!page)
    {
        page = create_page(real_size);
        if(!page)
        {
            return mem_alloc(real_size + 1);
        }

        *(FREE_TABLE + free_id) = page;
    }

    free_block_type *free_block = page->free_block;
    page->free_block = free_block->next;
    page->free_block_count--;

    if(page->free_block_count == 0)
    {
        *(FREE_TABLE + free_id) = page->next;
        page_type *next_page = page->next;
        if(next_page)
        {
            next_page->prev = NULL;
            page->next = NULL;
        }
    }
    return (void *) free_block;
}



void *mem_alloc_large(size_t size)
{
    int page_count = size / PAGE_SIZE1;

    int page_num = 0;
    for (bool found = false; !found; ++page_num)
    {
        found = true;
        for (int j = 0; j < page_count; ++j)
        {
            if (*(PAGE_TABLE + page_num + j))
            {
                found = false;
                break;
            }
        }

        if(page_num >= (PAGE_COUNT - page_count))
        {
            if(!found)
            {
                return NULL;
            }
        }
    }
   page_num--;
    for (int i = 0; i < page_count; ++i)
    {
        *(PAGE_TABLE + page_num + i) = BUSY;
    }

    page_type *page = mem_alloc(sizeof(page_type));
    page->block_size = size;
    page->free_block = NULL;
    page->free_block_count = 0;
    page->next = NULL;
    page->prev = NULL;
    *(PAGE_TABLE + page_num) = page;
    return (void *)get_page_offset(page_num);
}


void mem_free(void *addr)
{
    int page_num = get_page_num(addr);
    page_type *page = *(PAGE_TABLE + page_num);
    if(is_large_block(page))
    {
        mem_free_large(addr);
        return;
    }
    free_block_type *new_free_block = (free_block_type *) addr;
    new_free_block->next = NULL;

    free_block_type *free_block = page->free_block;
    if(free_block)
    {
        while(free_block->next)
        {
            free_block = free_block->next;
        }
        free_block->next = new_free_block;
    }
    else
    {
        page->free_block = new_free_block;
    }

    page->free_block_count++;

    int free_id = log_2((int) page->block_size);
    if(page->free_block_count == max_block_count(page)
       || (!is_remote_descriptor(page->block_size)
           && page->free_block_count == (max_block_count(page) - 1)))
    {
        page_type *prev_page = page->prev;
        if(prev_page)
        {
            prev_page->next = page->next;
            page_type *next_page = page->next;
            if(next_page)
            {
                next_page->prev = prev_page;
            }
        }
        else
        {
            *(FREE_TABLE + free_id) = NULL;
        }
        *(PAGE_TABLE + page_num) = NULL;

        if(is_remote_descriptor(page->block_size))
        {
            mem_free((void *)page);
        }
        return;
    }
   if(page->free_block_count == 1)
    {
        page_type *free_page = *(FREE_TABLE + free_id);
        if(free_page)
        {
            while(free_page->next)
            {
                free_page = free_page->next;
            }
            free_page->next = page;
            page->prev = free_page;
            page->next = NULL;
        }
        else
        {
            *(FREE_TABLE + free_id) = page;
        }
    }
    return;
}


void mem_free_large(void *addr)
{
    int page_num = get_page_num(addr);
    page_type *page = *(PAGE_TABLE + page_num);
    int page_count = page->block_size / PAGE_SIZE1;
    for (int i = 0; i < page_count; ++i)
    {
        *(PAGE_TABLE + page_num + i) = NULL;
    }
    mem_free((void *)page);
}



void *mem_realloc(void *addr, size_t size)
{
    void *new_addr = mem_alloc(size);
    if(new_addr)
    {
        page_type *page = *(PAGE_TABLE + get_page_num(addr));
        memcpy(new_addr, addr, MIN(page->block_size, size));
        mem_free(addr);
        return new_addr;
    }
    else
    {
        return NULL;
    }
}


page_type *create_page(size_t size)
{
    page_type *page;
    int page_num= 0;
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        page = *(PAGE_TABLE + i);
        if(!page)
        {
            page_num = i;
            PAGE_TABLE[page_num] = BUSY;
            break;
        }
        // No more free pages
        if(i == PAGE_COUNT - 1)
        {
            return NULL;
        }
    }
    char *page_offset = get_page_offset(page_num);
    free_block_type *free_block;
    int free_block_count;
 
    if(!is_remote_descriptor(size))
    {
        page = (page_type *)page_offset;
        free_block = (free_block_type *)(page_offset + size);
        free_block_count = PAGE_SIZE1 / size - 1;
    }
    else
    {
        page = mem_alloc(sizeof(page_type));
        if(!page)
        {
            PAGE_TABLE[page_num] = NULL;
            return NULL;
        }

        free_block = (free_block_type *)page_offset;
        free_block_count = PAGE_SIZE1 / size;
    }

    page->block_size = size;
    page->free_block = free_block;
    page->free_block_count = free_block_count;
    page->next = NULL;
    page->prev = NULL;

    for (int i = 0; i < free_block_count - 1; ++i)
    {
        free_block->next = (free_block_type *)((char *)free_block + size);
        free_block = free_block->next;
    }
    free_block->next = NULL;
    PAGE_TABLE[page_num] = page;
    return page;
}




char *get_page_offset(int page_num)
{
    return (char *)(memory + page_num * PAGE_SIZE1);
}


int get_page_num(void *addr)
{
    int offset = (char *)addr - memory;
    return offset / PAGE_SIZE1;
}


size_t round_to_4(size_t size)
{
    return (size + 3) & ~3;
}


size_t round_size(size_t size)
{
    size_t res_size = 1;
    while (res_size < size) res_size <<= 1;
    return res_size;
}


int log_2(int val)
{
    int res = 1;
    int pred = 1;
    while((pred <<= 1) < val) res++;
    return res;
}




//Syscalls



static  int mem_init_syscall (){
	mem_init();
	return 0;

}

static struct sysent mem_init_sysent = {
	0,
	mem_init_syscall

};

struct mem_alloc_args {
    unsigned long size;
    void *addr;

};


static int mem_alloc_syscall (struct thread *td, void *syscall_args){
	struct mem_alloc_args *arguments;
	
    arguments = (struct mem_alloc_args *) syscall_args;

    void *address = mem_alloc(arguments->size);
	int error;
	
	
	error = copyout(address, arguments->addr, sizeof(address));
	return (error);

}

static struct sysent mem_alloc_sysent = {
	2,
	mem_alloc_syscall

};




static int mem_free_syscall (struct thread *td, void *syscall_args){
//	struct mem_free_args *arguments;
	
    void * arguments = (void *) syscall_args;
    

    mem_free( arguments);
    uprintf("Argument at address 0x%p freed", arguments);
    return 0;
}


static struct sysent mem_free_sysent = {
	2,
	mem_free_syscall

};




static int mem_init_offset = NO_SYSCALL;
static int mem_alloc_offset = NO_SYSCALL ;
static int mem_free_offset = NO_SYSCALL ;

//load/unload handler

static int load1 (struct module * module, int cmd, void *args){
	int error = 0;
	switch (cmd) {
		case MOD_LOAD:
			uprintf("Loaded 1");
			break;
		case MOD_UNLOAD:
			uprintf("Unloaded 1");
			break;
		default:
			error = EOPNOTSUPP;
			break;
	
	}

	return (error);


}

static int load2 (struct module * module, int cmd, void *args){
	int error = 0;
	switch (cmd) {
		case MOD_LOAD:
			uprintf("Loaded 2");
			break;
		case MOD_UNLOAD:
			uprintf("Unloaded 2");
			break;
		default:
			error = EOPNOTSUPP;
			break;
	
	}

	return (error);


}

static int load3 (struct module * module, int cmd, void *args){
	int error = 0;
	switch (cmd) {
		case MOD_LOAD:
			uprintf("Loaded 3");
			break;
		case MOD_UNLOAD:
			uprintf("Unloaded 3");
			break;
		default:
			error = EOPNOTSUPP;
			break;
	
	}

	return (error);


}
SYSCALL_MODULE(mem_init_syscall, &mem_init_offset, &mem_init_sysent, load1, NULL);

SYSCALL_MODULE(mem_alloc_syscall, &mem_alloc_offset, &mem_alloc_sysent, load2, NULL);

SYSCALL_MODULE(mem_free_syscall, &mem_free_offset, &mem_free_sysent, load3, NULL);

