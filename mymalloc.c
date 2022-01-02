#include <unistd.h>
#include <assert.h>

#include <stdio.h> // FOR TESTING
#include "mymalloc.h"

#define PAGE_SIZE 4096

void* heap;

void myunlink(metadata_t* chunk) {
	metadata_t* prev_chunk = chunk->prev;
	metadata_t* next_chunk = chunk->next;

	if (prev_chunk) {
		prev_chunk->next = chunk->next;
	} else {
		// Update first_free_chunk
		((arena_t*)heap)->first_free_chunk = chunk->next;
	}
	if (next_chunk) {
		next_chunk->prev = chunk->prev;
	} else {
		// Update last_free_chunk
		((arena_t*)heap)->last_free_chunk = chunk->prev;
	}
}

void merge_chunks(size_t size) {
	// Loop throught chunks
	// Find two that are near each other
	// Check if merging would create desired size
	// Merge
}

void* mymalloc(size_t size) {
	size += sizeof(size_t) * 2; 	// Get real size of chunk
	size += size % 16; 				// 16 byte alignment
	//printf("%ld\n", size);
	assert(size < PAGE_SIZE); 		// Check boundaries

	if (size == 0) {
		return NULL;
	}

	// Check if heap exists
	if (!heap) {
		// Get pointer from current program break
		void* current_sbrk = sbrk(0);

		heap = current_sbrk;
		void* request_sbrk = (void*)sbrk(PAGE_SIZE);

		if (request_sbrk == (void*)-1) {
			return NULL;
		} else {
			// Should be the same
			assert(current_sbrk == request_sbrk);

			// Write heap metadata on heap
			arena_t* arena = (arena_t*)heap;
			arena->first_free_chunk = NULL;
			arena->last_free_chunk = NULL;

			// Write chunk metadata
			metadata_t* new_chunk = (metadata_t*)((void*)current_sbrk + sizeof(arena_t));
			new_chunk->prev_size = 0;
			new_chunk->size = size;
			new_chunk->next = NULL;
			new_chunk->prev = NULL;

			// Add chunk addr in heap metadata
			arena->first_chunk = (void*)new_chunk;
			arena->last_chunk = (void*)new_chunk;
			arena->page_count = 1;
			arena->used_size += sizeof(arena_t) + size;
			//printf("ARENA SIZE: %ld\n", sizeof(arena_t));

			return (void*)new_chunk + sizeof(size_t) * 2;
		}
	}
	// Check freed chunks
	// Loop over each one check for size
	arena_t* arena = heap;
	metadata_t* current_chunk = arena->first_free_chunk;
	metadata_t* most_suitable_chunk = NULL;
	
	// Find most suitable chunk
	while (current_chunk) {
		if (size <= current_chunk->size) {
			if (!most_suitable_chunk) {
				// First instance
				most_suitable_chunk = current_chunk;
			} else {
				// Chunk is more suitable only if its size is less than previous most suitable
				if (current_chunk->size < most_suitable_chunk->size) {
					most_suitable_chunk = current_chunk;
				}
			}
		} else {
			// Keep track of size if next 2 chunks are near each other
			// For merging
			// If next to each other
			printf("SANITY CHECK %p %p\n", (void*)current_chunk + current_chunk->size, (void*)current_chunk->next);
			// Sort before checking
			if ((void*)current_chunk + current_chunk->size == (void*)current_chunk->next) {
				printf("NEXT TO EACH OTHER AWWW\n");
			}
		}
		current_chunk = current_chunk->next;
	}
	if (most_suitable_chunk) {
		// Check if chunk needs shrinking or expanding
		if (size <= most_suitable_chunk->size / 2) {
			// Shrink chunk
			// TODO
		}
		// Remove chunk from linked list
		myunlink(most_suitable_chunk);

		// Return chunk from free
		return (void*)most_suitable_chunk + sizeof(size_t) * 2;

	} else {
		// Merge chunks if it makes sense
		// TODO
		merge_chunks(size);

		// No suitable chunk found
		printf("NO SUITABLE CHUNK FROM FREE\n");

		//printf("COMPARE SIZE AND AVAILABLE PAGESIZE %d %d\n", size, (PAGE_SIZE * arena->page_count - arena->used_size));
		if (size > (PAGE_SIZE * arena->page_count - arena->used_size)) {
			// Change data segment size if no space on heap
			// and top_chunk exists

			// Get pointer from current program break
			void* current_sbrk = sbrk(0);

			void* request_sbrk = (void*)sbrk(PAGE_SIZE);

			if (request_sbrk == (void*)-1) {
				return NULL;
			} else {
				// Should be the same
				assert(current_sbrk == request_sbrk);
				printf("MORE SPACE ON HEAP! %lld/4096\n", arena->used_size + size);
				arena->page_count += 1;
			}
		}
		void* end_of_last_chunk = (void*)arena + arena->used_size;
		//printf("END OF LAST CHUNK: %p\n", end_of_last_chunk);

		// Write chunk metadata
		metadata_t* new_chunk = (metadata_t*)((void*)end_of_last_chunk);
		new_chunk->prev_size = ((metadata_t*)arena->last_chunk)->size;
		new_chunk->size = size;
		new_chunk->next = NULL;
		new_chunk->prev = NULL;

		// Add chunk addr in heap metadata
		arena->last_chunk = (void*)new_chunk;
		arena->used_size += size;

		return (void*)new_chunk + sizeof(size_t) * 2;
	}
}

void myfree(void* ptr) {
	metadata_t* chunk = (metadata_t*)(ptr - sizeof(size_t) * 2);
	arena_t* arena = (arena_t*)heap;

	// Check if first free
	if (!arena->first_free_chunk) {
		arena->first_free_chunk = chunk;
	}
	// Check if page should be freed
	//printf("\nWHAT?! %p %p\n", (void*)chunk, (void*)arena->last_chunk);
	if ((void*)chunk == (void*)arena->last_chunk) {
		chunk->next = NULL;

		// Calculate size without last chunk
		int heap_size_without_last_chunk = arena->used_size - chunk->size;
		int prev_chunk_size = chunk->prev_size;

		// Check if new size of heap would be in boundaries of previous page
		if (heap_size_without_last_chunk <= (arena->page_count - 1) * PAGE_SIZE) {

			// Release freed chunks in new page
			metadata_t* current_chunk = arena->first_free_chunk;
			while (current_chunk) {
				if ((void*)current_chunk >= (void*)(heap + (arena->page_count - 1) * PAGE_SIZE)) {
					current_chunk->prev->next = NULL;
					arena->last_free_chunk = current_chunk->prev;

					if (current_chunk == arena->first_free_chunk) {
						arena->first_free_chunk = NULL;
					}
				}
				current_chunk = current_chunk->next;
			}
			arena->used_size = heap_size_without_last_chunk;

			// Release page
			void* current_sbrk = sbrk(0);
			void* request_sbrk = (void*)sbrk(-PAGE_SIZE);

			assert(request_sbrk != (void*)-1);
			assert(current_sbrk == request_sbrk);

			// Unset first_free_chunk if it was the released chunk
			if (arena->last_chunk == arena->first_free_chunk) {
				arena->first_free_chunk = NULL;
			}

			arena->last_chunk = heap + heap_size_without_last_chunk - prev_chunk_size;
			printf("\nSBRK(-PAGE_SIZE) CALLED LAST CHUNK UPDATED: %p\n", arena->last_chunk);

			return;
		}
	}

	// Update headers
	if (arena->last_free_chunk) {
		chunk->prev_size = ((metadata_t*)arena->last_free_chunk)->size;
		((metadata_t*)arena->last_free_chunk)->next = chunk;
		chunk->prev = arena->last_free_chunk;
	} else {
		chunk->prev_size = 0;
		chunk->prev = NULL;
	}
	chunk->next = NULL;

	arena->last_free_chunk = chunk;
}

void debug_list_free() {
	metadata_t* current_chunk = ((arena_t*)heap)->first_free_chunk;

	puts("\n-------FREED_CHUNKS---------");
	while (current_chunk) {
		printf("Addr: %p", current_chunk);
		printf("\nprev_size: %ld", current_chunk->prev_size);
		printf("\nsize: %ld", current_chunk->size);
		printf("\nnext: %p", current_chunk->next);
		printf("\nprev: %p", current_chunk->prev);
	
		current_chunk = current_chunk->next;
		puts("\n----------------------------");
	}
}
