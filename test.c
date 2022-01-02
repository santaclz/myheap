#include <stdio.h>
#include "mymalloc.h"

void info_chunk(int* p) {

	long long chunk_size = *((long long*)p-1);
	chunk_size -= chunk_size % 2; 					// Remove last_inuse
	chunk_size -= 16; 								// Minus medatada
	int chunk_int_size = chunk_size / sizeof(int); 	// Convert to int size
	long long i;

	for (i = 0; i < chunk_int_size; i++) {
		p[i] = i;
	}

	puts("\n-----------------------------------------------------");
	printf("Allocated chunk at: %p\n", p);
	printf("Ends at: %p\n", p + chunk_int_size);
	printf("Chunk metadata: %02llx %02llx\n", *((long long*)p-2), *((long long*)p-1));
	printf("Usable chunk size: %lld\n", chunk_size);
	printf("Chunk size (int): %d\n", chunk_int_size);
	printf("Chunk content: ");

	for (i = 0; i < chunk_int_size; i++) {
		printf("%02x ", *(p+i));
	}
	puts("\n-----------------------------------------------------");
}

int main() {

	int* p1 = mymalloc(10 * sizeof(int));
	int* p2 = mymalloc(20 * sizeof(int));
	int* p3 = mymalloc(30 * sizeof(int));

	info_chunk(p1);
	info_chunk(p2);
	info_chunk(p3);

	myfree(p1);
	myfree(p2);
	myfree(p3);

	debug_list_free();

	int* p4 = mymalloc(30 * sizeof(int));
	int* p5 = mymalloc(10 * sizeof(int));
	int* p6 = mymalloc(20 * sizeof(int));
	int* p7 = mymalloc(1000 * sizeof(int));
	int* p8 = mymalloc(1000 * sizeof(int));
	int* p9 = mymalloc(1000 * sizeof(int));
	int* p10 = mymalloc(1000 * sizeof(int));

	info_chunk(p4);
	info_chunk(p5);
	info_chunk(p6);
	info_chunk(p7);
	info_chunk(p8);

	debug_list_free();
	myfree(p10); 		// free PAGE
	debug_list_free();

	myfree(p9);
	myfree(p8);
	myfree(p7);
	myfree(p6);
	myfree(p5);
	myfree(p4);
	debug_list_free();

	int* p11 = mymalloc(30 * sizeof(int)); // merged chunk
	info_chunk(p11);

	debug_list_free();

	/*
	myfree(p4);
	myfree(p5);
	myfree(p6);
	*/

	return 0;
}
