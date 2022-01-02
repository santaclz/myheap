#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <sys/types.h>

// Metadata for chunks
typedef struct metadata_t {
	size_t prev_size;
	size_t size;
	struct metadata_t* next; 	// Used only in free chunks
	struct metadata_t* prev; 	// Used only in free chunks
} metadata_t;

typedef struct arena_state {
	long long page_count;
	long long used_size;
	void* first_chunk;
	void* last_chunk;
	void* first_free_chunk;
	void* last_free_chunk;
} arena_t;

void* mymalloc(size_t);
void myfree(void*);
void debug_list_free();

#endif
