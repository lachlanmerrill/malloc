/*	Stuart Norcross - 12/03/10 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>

#include "myalloc.h"

#define FIELD_INACTIVE 0
#define FIELD_ACTIVE 1
#define FIELD_LAST 1
#define FIELD_NOTLAST 0

#define WORD_SIZE 4
#define HEADER_SIZE 1
#define INIT_SIZE 4096

typedef struct header_ { // header is exactly 32 bits (4 bytes, 1 word)
	int size : 30; // Size field takes 31 bits
	int last : 1;
	int active : 1; // Active flag takes 1 bit
} *header;


typedef struct prefooter_ {
	int next; // pointer to the next free block
} *prefooter;


	// Static variables are fun and definitely going to break stuff later
static void* addressable;

// Potential optimisation: reserve a prefooter at addressable with an offset
// to the first free block.

void *myalloc(int size) {

	static int init_flag = 1; // Do we need to perform first-call setup?

	header curr_header; /// Initialize a struct header pointer
	header curr_footer;
	prefooter pre_footer;

	// Perform first-time setup
	if (init_flag == 1) {
		init_flag = 0;

		int page_size = getpagesize();

		addressable = mmap(NULL, (page_size * INIT_SIZE),
												PROT_READ | PROT_WRITE,
												MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		prefooter first_free;
		first_free = addressable;
		curr_header = (header) first_free + HEADER_SIZE;

		first_free->next = HEADER_SIZE;

		int ini_size = (page_size * INIT_SIZE)/WORD_SIZE - HEADER_SIZE;

		curr_header->active = FIELD_INACTIVE; // Set active bit to 0 - inactive
		curr_header->size = ini_size; // Set size to WORD_SIZE
		curr_header->last = FIELD_LAST;

		// Create footer at end of mapped memory
		// curr_footer = (struct footer*) addressable + curr_header->size - 4;

		curr_footer = (header) addressable + curr_header->size - HEADER_SIZE;


		curr_footer->size = ini_size;
		curr_footer->active = FIELD_ACTIVE;
		curr_footer->last = FIELD_LAST;


		// setup pre-footer for initial block which gives the next free block's offset
		// from addressable
		pre_footer = (prefooter) curr_footer - HEADER_SIZE;
		pre_footer->next = HEADER_SIZE;

		// printf("Initialized with %d words.\n", curr_header->size);

	}

	// printf("%ld\n", (long int)addressable);

	prefooter first_free = (prefooter) addressable;

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

	curr_header = (header) addressable + first_free->next;
	curr_footer = curr_header + curr_header->size - HEADER_SIZE;

	header old_footer = curr_footer;

	pre_footer = (prefooter) curr_footer - HEADER_SIZE;

	// printf("First free block at %d\n", first_free->next);
	// printf("Starting search from %ld\n", (long int) (curr_header - (header) addressable));


	int fail_flag = 1;

	while (curr_header->size < needed_size) {
		curr_header = (header) addressable + pre_footer->next;
		old_footer = curr_footer;
		curr_footer = curr_header + curr_header->size - HEADER_SIZE;
		pre_footer = (prefooter) old_footer - HEADER_SIZE;

		fail_flag = 0;
	}

	// // Jump to new footer (or space for the new footer) and initialize it
	// curr_footer = curr_header + needed_size - HEADER_SIZE;
	//
	// curr_footer->active = FIELD_ACTIVE;
	// curr_footer->size = needed_size;
	// // curr_footer->last = FIELD_NOTLAST;

	header next_footer = curr_footer;
	header next_header;

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

		pre_footer->next = next_header - (header) addressable - HEADER_SIZE;

		if (fail_flag) {
			first_free->next = next_header - (header) addressable;
		}

		// printf("Size excess of %d\n", next_header->size);

	} else {
		prefooter curr_prefooter = (prefooter) curr_footer - HEADER_SIZE;
		pre_footer->next = curr_prefooter->next;

		curr_header->active = FIELD_ACTIVE;
		curr_footer->active = FIELD_ACTIVE;

		if (fail_flag) {
			first_free->next = curr_prefooter->next;
		}
	}

	// printf("Assigned header at %ld\n", (long int) (curr_header - (header) addressable));
	// printf("Assigned footer %ld\n", (long int) (curr_footer - (header) addressable));
	// printf("Assigned prefooter %ld\n", (long int) (prefooter - (prefooter) addressable));
	// printf("Header size is %d\n", curr_header->size);


	header ret_val = curr_header + HEADER_SIZE;

	// Return a pointer to the start of the now allocated memory.
	return (void*) ret_val;
}

prefooter find_prev_prefooter(header search_header) {

	// We'll start the search at the first free block
	prefooter first_free = (prefooter) addressable;

	header curr_header = (header) addressable + first_free->next;

	// printf("Starting search from %ld\n", (long int) curr_header - (long int) addressable);
	// printf("Search header is at %ld\n", (long int) search_header - (long int) addressable);

	// in the case there is no free block before the new block
	if (curr_header >= search_header) {
		return first_free;
	}

	prefooter curr_prefooter = (prefooter) curr_header + curr_header->size - 2;
	prefooter prev_prefooter = curr_prefooter;

	// Go through the linked list of free blocks until a prefooter next pointer skips over the new block.
	while (curr_header < search_header && curr_header->last == FIELD_NOTLAST) {
		curr_header = (header) addressable + curr_prefooter->next;
		prev_prefooter = curr_prefooter;
		curr_prefooter = (prefooter) curr_header + curr_header->size - 2;
	}

	// Return that prefooter
	return prev_prefooter;
}


void myfree(void *ptr)	{

	// Find the header of the block. We assume we've been given a valid pointer
	header curr_header = (header) ptr - HEADER_SIZE;

	// Find the footer of the block
	header curr_footer = curr_header + curr_header->size - HEADER_SIZE;

	prefooter curr_prefooter = (prefooter) curr_footer - HEADER_SIZE;

	// Set header and footer active fields to inactive
	curr_footer->active = FIELD_INACTIVE;
	curr_header->active = FIELD_INACTIVE;

	// printf("Set current block to inactive.\n");
	//
	// printf("%ld\n", (long int) curr_header - (long int) addressable);
	// printf("Size: %d\nActive: %d\nLast: %d\n", curr_header->size, curr_header->active, curr_header->last);

	// printf("%ld\n", (long int) curr_footer - (long int) addressable);
	// printf("Size: %d\nActive: %d\nLast: %d\n", curr_footer->size, curr_footer->active, curr_footer->last);

	int coal_for = 0;
	int coal_back = 0;

	header prev_footer;
	header prev_header;

	header next_header;
	header next_footer;


	if (curr_header != (header) addressable + HEADER_SIZE) {
		prev_footer = curr_header - HEADER_SIZE;

		// printf("owo\n");

		if (prev_footer->active == FIELD_ACTIVE) {

			coal_back = 0;

		} else if (prev_footer->active == FIELD_INACTIVE) {

			// printf("what's this\n");

			coal_back = 1;
			prev_header = prev_footer - prev_footer->size + HEADER_SIZE;

		}
	} else if (curr_header == (header) addressable || prev_footer->active == FIELD_ACTIVE) {

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


		// printf("%ld\n", (long int) next_header - (long int) addressable);
		// printf("Size: %d\nActive: %d\nLast: %d\n", next_header->size, next_header->active, next_header->last);

	} else {

		coal_for = 0;

	}



	if (!coal_for && !coal_back) {
		// No coalescing required

		prefooter prev_prefooter = find_prev_prefooter(curr_header);
		// printf("Found previous prefooter at %ld\n", (long int) prev_prefooter - (long int) addressable);
		prefooter curr_prefooter = (prefooter) curr_footer - HEADER_SIZE;

		curr_prefooter->next = prev_prefooter->next;
		prev_prefooter->next = curr_header - (header) addressable;
		// printf("No coalescing\n");

	} else if (coal_for && coal_back) {

		// printf("For and back\n");
		// Forwards and backwards required
		prev_header->size = prev_header->size + curr_header->size + next_header->size;
		next_footer->size = prev_header->size;

	} else if (coal_for) {

		// printf("For only\n");
		// Forwards only required

		prefooter prev_prefooter = find_prev_prefooter(curr_header);
		prev_prefooter->next = curr_header - (header) addressable;

		curr_header->size += next_header->size;
		next_footer->size = curr_header->size;

	} else if (coal_back) {
		// Backwards only required

		// printf("Back only\n");

		prev_header->size += curr_header->size;prev_header->size;

		curr_footer->size = prev_header->size;

		prefooter prev_prefooter = (prefooter) prev_footer - HEADER_SIZE;
		prefooter curr_prefooter = (prefooter) curr_footer - HEADER_SIZE;

		curr_prefooter->next = prev_prefooter->next;

		// printf("%ld: %d\n", prev_header - (header) addressable, prev_header->size);

	}

	prefooter first_free = (prefooter) addressable;
}
