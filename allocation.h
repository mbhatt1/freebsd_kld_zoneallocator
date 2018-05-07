#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/malloc.h>

#define PAGE_SIZE1 2048
#define PAGE_COUNT (MEM_SIZE / PAGE_SIZE1)
#define PAGE_TABLE ((page_type **) memory)
#define FREE_TABLE_SIZE (log_2(PAGE_SIZE1 / 2) + 1)
#define FREE_TABLE (((page_type **) memory) + PAGE_COUNT)
#define BUSY ((page_type *) 1)

#define MIN_BLOCK_SIZE (sizeof(free_block_type))
#define max_block_count(page) ((int)(PAGE_SIZE1 / page->block_size))
#define is_large_block(page) (page->block_size >= PAGE_SIZE1)
#define is_remote_descriptor(size) (size < sizeof(page_type) || size > sizeof(page_type) * 3)
#define MEM_SIZE 65535

int power(int x, int y);
void mem_init(void);
void *mem_alloc(size_t size);
void mem_free(void * addr);
void *mem_realloc(void *addr, size_t size);

size_t round_size(size_t argument);
