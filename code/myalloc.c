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

#define WORD_SIZE 4
#define HEADER_SIZE 1
#define PAGE_SIZE 4096
#define INIT_SIZE 8

struct header { // header is exactly 32 bits (4 bytes, 1 word)
	int size : 30; // Size field takes 31 bits
	int last : 1;
	int active : 1; // Active flag takes 1 bit
};
struct prefooter {
	int next; // pointer to the next free block
};

	// Static variables are fun and definitely going to break stuff later
static void* addressable;

// Potential optimisation: reserve a prefooter at addressable with an offset
// to the first free block.

void *myalloc(int size) {

	static int init_flag = 1; // Do we need to perform first-call setup?

	struct header *curr_header; /// Initialize a struct header pointer
	struct header *curr_footer;
	struct prefooter *prefooter;

	// Perform first-time setup
	if (init_flag == 1) {
		init_flag = 0;

		addressable = mmap(NULL, (PAGE_SIZE * INIT_SIZE),
												PROT_READ | PROT_WRITE,
												MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		struct prefooter* first_free = addressable;
		curr_header = (struct header*) first_free + HEADER_SIZE;

		first_free->next = HEADER_SIZE;

		int ini_size = (PAGE_SIZE * INIT_SIZE)/WORD_SIZE - HEADER_SIZE;

		curr_header->active = FIELD_INACTIVE; // Set active bit to 0 - inactive
		curr_header->size = ini_size; // Set size to WORD_SIZE
		curr_header->last = FIELD_LAST;

		// Create footer at end of mapped memory
		// curr_footer = (struct footer*) addressable + curr_header->size - 4;

		curr_footer = (struct header*) addressable + curr_header->size - HEADER_SIZE;


		curr_footer->size = ini_size;
		curr_footer->active = FIELD_ACTIVE;
		curr_footer->last = FIELD_LAST;


		// setup pre-footer for initial block which gives the next free block's offset
		// from addressable
		prefooter = (struct prefooter*) curr_footer - HEADER_SIZE;
		prefooter->next = HEADER_SIZE;

	}

	struct prefooter* first_free = (struct prefooter*) addressable;


	printf("Assigned header at %ld\n", (long int) (curr_header - (struct header*) addressable));
	printf("Assigned footer %ld\n", (long int) (curr_footer - (struct header*) addressable));
	printf("Assigned prefooter %ld\n", (long int) (prefooter - (struct prefooter*) addressable));
	printf("Header size is %d\n", curr_header->size);

	// Word align by ensuring size is divisible by four
	size += size%WORD_SIZE;

	// Increase size to accommodate the header and footer fields.
	int needed_size = size / WORD_SIZE;


	// Convert size in bytes to size in words
	needed_size = needed_size + HEADER_SIZE * 2;


	// Now we start doing malloc-y stuff


	// TODO: Implement out of mapped bounds checking
	// Keep hopping forward until we find an inactive field with sufficient space
	// while (curr_header->active == FIELD_ACTIVE){ // This will have horrifying fragmentation
	//
	// 	old_footer = curr_footer;
	//
	// 	curr_header = curr_footer + HEADER_SIZE;
	//
	// 	curr_footer = curr_header + curr_header->size - HEADER_SIZE;
	//
	// 	// For now let's assume we won't run out of memory from the initially
	// 	// allocated page.
	// }

	curr_header = (struct header*) addressable + first_free->next;
	curr_footer = curr_header + curr_header->size - HEADER_SIZE;

	struct header* old_footer = curr_footer;

	prefooter = (struct prefooter*) curr_footer - HEADER_SIZE;

	int fail_flag = 1;

	while (curr_header->size < needed_size) {
		curr_header = (struct header*) addressable + prefooter->next;
		old_footer = curr_footer;
		curr_footer = curr_header + curr_header->size - HEADER_SIZE;
		prefooter = (struct prefooter*) old_footer - HEADER_SIZE;

		fail_flag = 0;
	}


	// // Jump to new footer (or space for the new footer) and initialize it
	// curr_footer = curr_header + needed_size - HEADER_SIZE;
	//
	// curr_footer->active = FIELD_ACTIVE;
	// curr_footer->size = needed_size;
	// // curr_footer->last = FIELD_NOTLAST;

	struct header* next_footer = curr_footer;
	struct header* next_header;

	// If the free block we assigned was larger than needed
	if (curr_header->size > (needed_size + (HEADER_SIZE * 3))) {

		// Create new footer and header at split.
		next_header = curr_header + needed_size;
		next_footer = curr_footer;
		curr_footer = next_header - HEADER_SIZE;

		// Update new header size and next footer size.
		next_header->size = curr_header->size - needed_size;
		next_footer->size = next_header->size;

		// Update new header active and last fields.
		next_header->active = FIELD_INACTIVE;
		next_header->last = next_footer->last;

		// Update current header and new footer size, active and last fields.
		curr_header->size = needed_size;
		curr_footer->size = needed_size;

		curr_header->active = FIELD_ACTIVE;
		curr_footer->active = FIELD_ACTIVE;

		curr_header->last = FIELD_NOTLAST;
		curr_footer->last = FIELD_NOTLAST;

		prefooter->next = next_header - (struct header*) addressable - HEADER_SIZE;

		if (fail_flag) {
			first_free->next = next_header - (struct header*) addressable - HEADER_SIZE;
		}

	} else {
		struct prefooter* curr_prefooter = (struct prefooter*) curr_footer - HEADER_SIZE;
		prefooter->next = curr_prefooter->next;

		curr_header->active = FIELD_ACTIVE;
		curr_footer->active = FIELD_ACTIVE;

		if (fail_flag) {
			first_free->next = curr_prefooter->next;
		}
	}

	struct header* ret_val = curr_header + HEADER_SIZE;

	// Return a pointer to the start of the now allocated memory.
	return (void*) ret_val;
}

struct prefooter* find_prev_prefooter(struct header* search_header) {
	struct header* curr_header = (struct header*) addressable;

	while (curr_header->active == FIELD_ACTIVE && curr_header->last == FIELD_NOTLAST) {
		curr_header += curr_header->size;
	}

	if (curr_header >= search_header) {
		return (struct prefooter*) addressable;
	}


}


void myfree(void *ptr)	{

	// Find the header of the block. We assume we've been given a valid pointer
	struct header* curr_header = (struct header*) ptr - HEADER_SIZE;

	// Find the footer of the block
	struct header* curr_footer = curr_header + curr_header->size - HEADER_SIZE;

	struct prefooter* curr_prefooter = (struct prefooter*) curr_footer - HEADER_SIZE;
	curr_prefooter->next = (int) (curr_header - (struct header*) addressable);

	// Set header and footer active fields to inactive
	curr_footer->active = FIELD_INACTIVE;
	curr_header->active = FIELD_INACTIVE;

	printf("Set current block to inactive.\n");

	printf("%ld, %ld\n", (long int) curr_header, (long int) addressable);

	int coal_for = 0;
	int coal_back = 0;

	struct header* prev_footer;
	struct header* prev_header;

	struct header* next_header;
	struct header* next_footer;


	if (curr_header != (struct header*) addressable + HEADER_SIZE) {
		prev_footer = curr_header - HEADER_SIZE;

		if (prev_footer->active == FIELD_ACTIVE) {

			coal_back = 0;

		} else if (prev_footer->active == FIELD_INACTIVE) {

			coal_back = 1;
			prev_header = prev_footer - prev_footer->size + HEADER_SIZE;

		}
	} else if (curr_header == (struct header*) addressable || prev_footer->active == FIELD_ACTIVE) {

		coal_back = 0;

	}


	if (curr_header->last == FIELD_NOTLAST) {
		next_header = curr_footer + HEADER_SIZE;

		if (next_header->active == FIELD_ACTIVE) {

			coal_for = 0;

		} else if (next_header->active == FIELD_INACTIVE) {

			coal_for = 1;
			next_footer = next_header + next_header->size - HEADER_SIZE;

		}

	} else {

		coal_for = 0;

	}

	if (!coal_for && !coal_back) {
		// No coalescing required


	} else if (coal_for && coal_back) {
		// Forwards and backwards required
		prev_header->size = prev_header->size + curr_header->size + next_header->size;
		next_footer->size = prev_header->size;

	} else if (coal_for) {
		// Forwards only required


	} else if (coal_back) {
		// Backwards only required


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
