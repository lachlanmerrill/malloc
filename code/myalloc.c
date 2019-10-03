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

struct prefooter {
	int offset;
};

static int offset_limit = 1; // Current max mapped memory

	// Static variables are fun and definitely going to break stuff later
static void* addressable;

int calculate_new_footer(struct footer *f1, struct footer *f2) {
	return (f1 + f1->next - f2);
}

void *myalloc(int size){

	static int init_flag = 1; // Do we need to perform first-call setup?


	struct header *curr_header; /// Initialize a struct header pointer
	curr_header = addressable; // Point it to the start of addressable space

	struct footer *curr_footer;
	struct prefooter *prefooter;

	// Perform first-time setup
	if (init_flag == 1) {
		init_flag = 0;

		addressable = mmap(NULL, (PAGE_SIZE * INIT_SIZE),
												PROT_READ | PROT_WRITE,
												MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		curr_header = addressable;

		curr_header->active = FIELD_INACTIVE; // Set active bit to 0 - inactive
		curr_header->size = (PAGE_SIZE * INIT_SIZE)/CHUNK_SIZE; // Set size to CHUNK_SIZE
		offset_limit = (PAGE_SIZE * INIT_SIZE)/CHUNK_SIZE - HEADER_SIZE;

		// Create footer at end of mapped memory
		// curr_footer = (struct footer*) addressable + curr_header->size - 4;

		curr_footer = (struct footer*) addressable + curr_header->size - HEADER_SIZE;


		curr_footer->next = 0;
		curr_footer->active = FIELD_ACTIVE;
		curr_footer->last = FIELD_LAST;


		// setup pre-footer for initial block which gives the header's offset from addressable
		prefooter = (struct prefooter*) curr_footer - 1;
		prefooter->offset = 0;

	}


	printf("Assigned header at %ld\n", (long int) (curr_header - (struct header*) addressable));
	printf("Assigned footer %ld\n", (long int) (curr_footer - (struct footer*) addressable));
	printf("Assigned prefooter %ld\n", (long int) (prefooter - (struct prefooter*) addressable));
	printf("Header size is %d\n", curr_header->size);

	// Increase size to accommodate the header and footer fields.
	int needed_size = size + CHUNK_SIZE * 3;


	// Convert size in bytes to size in words
	needed_size = needed_size / CHUNK_SIZE;


	// Now we start doing malloc-y stuff

	// int current_offset = 0;
	struct footer *old_footer = curr_footer;

	// Keep hopping forward until we find an inactive field with sufficient space
	while (curr_header->active == FIELD_ACTIVE
		|| (
			curr_header->active == FIELD_INACTIVE
			&& curr_header->size < needed_size
		)) { // This will have horrifying fragmentation

		old_footer = curr_footer;

		curr_footer = (struct footer*) (curr_header + curr_header->size);

		curr_header = (struct header*) (curr_footer + curr_footer->next);


		// For now let's assume we won't run out of memory from the initially
		// allocated page.
	}


	// Jump to new footer (or space for the new footer) and initialize it
	curr_footer = (struct footer*) curr_header + needed_size - HEADER_SIZE;

	curr_footer->active = FIELD_ACTIVE;
	curr_footer->next = 1;
	curr_footer->last = FIELD_NOTLAST;


	// If the free block we assigned was larger than needed
	if (curr_header->size > needed_size) {

		// Create a new header after the allocated block
		struct header *new_header = (struct header*) curr_footer + HEADER_SIZE;
		new_header->size = curr_header->size - needed_size;
		new_header->active = FIELD_INACTIVE;

		// Create a new footer for the new free block and initialize it
		curr_footer = (struct footer*) (curr_header + curr_header->size - HEADER_SIZE);
		curr_footer->active = FIELD_INACTIVE;
		curr_footer->next = calculate_new_footer(old_footer, curr_footer);
		curr_footer->last = FIELD_NOTLAST;


		// Update the old footer to point to the new free block
		old_footer->next = (struct footer*) new_header - old_footer;


		struct prefooter* prefooter = (struct prefooter*) curr_footer - 1;
		prefooter->offset = (int) (new_header - (struct header*) addressable);

	} else { // In the unlikely case it was exactly the right size

		// Point old_footer to the free block after this one.
		old_footer->next = calculate_new_footer(curr_footer, old_footer);
	}


	// Return a pointer to the start of the now allocated memory.
	return (void*) curr_header + HEADER_SIZE;
}


void myfree(void *ptr)	{

	// Find the header of the block. We assume we've been given a valid pointer
	struct header* block = (struct header*) (ptr - HEADER_SIZE);

	// Find the footer of the block
	struct footer* curr_footer = (struct footer*) (block + block->size);

	struct prefooter* curr_prefooter = (struct prefooter*) curr_footer - HEADER_SIZE;
	curr_prefooter->offset = (int) (block - (struct header*) addressable);

	// Set header and footer active fields to inactive
	curr_footer->active = FIELD_INACTIVE;
	block->active = FIELD_INACTIVE;

	printf("Set current block to inactive.\n");

	printf("%ld, %ld\n", (long int) block, (long int) addressable);


	// If the header isn't the first in the mapped memory we check previous blocks
	if (block != (struct header*) addressable) {
		struct footer* prev_footer = (struct footer*) (block - HEADER_SIZE);


		// If the previous footer indicates the field is inactive then coalesce them
		if (prev_footer->active == FIELD_INACTIVE) {
			struct prefooter* prev_prefooter = (struct prefooter*) prev_footer - HEADER_SIZE;

			struct header* prev_header = (struct header*) addressable + prev_prefooter->offset;

			prev_header->size += block->size;
			curr_prefooter->offset = prev_prefooter->offset;

			block = prev_header;
		}

	}


	// // What the fuck.
	// if (curr_footer->last != FIELD_LAST) {
	// 	struct header* next_header = (struct header*) curr_footer + HEADER_SIZE;
	// 	struct footer* next_footer = (struct footer*) curr_footer + next_header->size - HEADER_SIZE;
	// 	struct prefooter* next_prefooter;
	//
	// 	// Coalesce all the following free blocks.
	// 	while (next_header->active == FIELD_INACTIVE && next_footer->last != FIELD_LAST) {
	// 		block->size += next_header->size;
	// 		curr_footer = next_footer;
	// 		next_prefooter = (struct prefooter*) curr_footer;
	// 		next_prefooter->offset = (int) (block - (struct header*) addressable);
	//
	// 		next_header = (struct header*) curr_footer + HEADER_SIZE;
	// 		next_footer = (struct footer*) next_header + next_header->size - HEADER_SIZE;
	// 	}
	//
	// 	// Iterate through until we find the next free field
	// 	while (next_header->active != FIELD_INACTIVE && next_footer->last != FIELD_LAST) {
	// 		next_header = (struct header*) next_footer + HEADER_SIZE;
	// 		next_footer = (struct footer*) next_header + next_header->size - HEADER_SIZE;
	// 	}
	//
	// 	if (next_footer->last != FIELD_LAST) {
	// 		curr_footer->next = (struct footer*) next_header - curr_footer;
	// 	}
	//
	// }

}
