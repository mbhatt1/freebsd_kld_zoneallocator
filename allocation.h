int power(int x, int y);
void mem_init(void);
void *mem_alloc(size_t size);
void mem_free(void * addr);
void *mem_realloc(void *addr, size_t size);

size_t round_size(size_t argument);
