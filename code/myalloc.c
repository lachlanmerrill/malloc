/*	Stuart Norcross - 12/03/10 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "myalloc.h"

#define FIELD_INACTIVE 0
#define FIELD_ACTIVE 1
#define FIELD_LAST 1
#define FIELD_NOTLAST 0

#define CHUNK_SIZE 4
#define HEADER_SIZE 1
#define PAGE_SIZE 4096
#define INIT_SIZE 8

struct header { // header is exactly 32 bits (4 bytes, 1 word)
	int size : 31; // Size field takes 31 bits
	int active : 1; // Active flag takes 1 bit
};
struct footer {
	int next : 30; // Offset to next free block (pointer increments by one for each word)
	int active : 1; // Is this block free or not
	int last : 1; // Is this the last block of mapped memory
};

static int offset_limit = 1; // Current max mapped memory

	// Static variables are fun and definitely going to break stuff later
static void* addressable;

int calculate_new_footer(struct footer *f1, struct footer *f2) {
	return (f1 + f1->next - f2);
}

void *myalloc(int size){
	printf("myalloc called\n");

	static int init_flag = 1; // Do we need to perform first-call setup?


	struct header *curr_header; /// Initialize a struct header pointer
	curr_header = addressable; // Point it to the start of addressable space

	struct footer *curr_footer;

	// Perform first-time setup
	if (init_flag == 1) {
		init_flag = 0;

		addressable = mmap(NULL, (PAGE_SIZE * INIT_SIZE),
												PROT_READ | PROT_WRITE,
												MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		curr_header = addressable;

		printf("mapped init space\n");

		curr_header->active = FIELD_INACTIVE; // Set active bit to 0 - inactive
		curr_header->size = (PAGE_SIZE * INIT_SIZE)/CHUNK_SIZE; // Set size to CHUNK_SIZE
		offset_limit = (PAGE_SIZE * INIT_SIZE)/CHUNK_SIZE - HEADER_SIZE;

		printf("Assigned header. Offset limit is %d\n", offset_limit);

		// Create footer at end of mapped memory
		// curr_footer = (struct footer*) addressable + curr_header->size - 4;

		curr_footer = (struct footer*) addressable + curr_header->size - HEADER_SIZE;

		printf("Initialized footer\n");

		curr_footer->next = 0;
		curr_footer->active = FIELD_ACTIVE;
		curr_footer->last = FIELD_LAST;

		printf("Assigned footer\n");
	}

	printf("Initialized\n");

	// Increase size to accommodate the header and footer fields.
	int needed_size = size + CHUNK_SIZE * 2;

	// Convert size in bytes to size in words
	needed_size = needed_size / CHUNK_SIZE;

	// Now we start doing malloc-y stuff

	// int current_offset = 0;
	struct footer *old_footer;


	// Keep hopping forward until we find an inactive field with sufficient space
	while (curr_header->active == FIELD_ACTIVE
		|| (
			curr_header->active == FIELD_INACTIVE
			&& (curr_header->size) < needed_size
		)) { // This will have horrifying fragmentation

		old_footer = curr_footer;

		curr_footer = (struct footer*) (curr_header + curr_header->size);

		curr_header = (struct header*) (curr_footer + curr_footer->next);

		printf("Searching for free block\n");

		// For now let's assume we won't run out of memory from the initially
		// allocated page.
	}

	// Jump to new footer (or space for the new footer) and initialize it
	curr_footer = (struct footer*) (curr_header) + needed_size;
	curr_footer->active = FIELD_ACTIVE;
	curr_footer->next = calculate_new_footer(old_footer, curr_footer);
	curr_footer->last = FIELD_NOTLAST;

	printf("Found free block. Initializing footer\n");

	// If the free block we assigned was larger than needed
	if (curr_header->size > needed_size) {
		printf("Size mismatch. Re-organized blocks to retain free space\n");

		// Create a new header after the allocated block
		struct header *new_header = (struct header*) curr_footer + HEADER_SIZE;
		new_header->size = curr_header->size - needed_size;
		new_header->active = FIELD_INACTIVE;

		printf("Initialized new header.\n");

		// Create a new footer for the new free block and initialize it
		curr_footer = (struct footer*) (curr_header + curr_header->size - HEADER_SIZE);
		curr_footer->active = FIELD_INACTIVE;
		curr_footer->next = calculate_new_footer(old_footer, curr_footer);
		curr_footer->last = FIELD_NOTLAST;

		printf("Initialized new footer.\n");

		// Update the old footer to point to the new free block
		old_footer->next = (struct footer*) new_header - old_footer;

		printf("Updated old footer.\n");

	} else { // In the unlikely case it was exactly the right size

		// Point old_footer to the free block after this one.
		old_footer->next = calculate_new_footer(curr_footer, old_footer);
	}

	printf("Success. Returning pointer to allocated memory\n");

	// Return a pointer to the start of the now allocated memory.
	return curr_header + HEADER_SIZE;
}

void myfree(void *ptr)	{

	// Find the header of the block. We assume we've been given a valid pointer
	struct header* block = (struct header*) (ptr - HEADER_SIZE);

	// Find the footer of the block
	struct footer* curr_footer = (struct footer*) (block + block->size);

	// Set header and footer active fields to inactive
	curr_footer->active = FIELD_INACTIVE;
	block->active = FIELD_INACTIVE;

	// Let's not do coalescing for now.
	// // If the header isn't the first in the mapped memory we check previous blocks
	// if (block != addressable) {
	// 	struct footer* prev_footer = (struct footer*) (block - HEADER_SIZE);
	// }


}
