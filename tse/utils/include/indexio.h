#pragma once

#include <stdint.h>

#include "queue.h"

/*
 * Doc for doc_queue
 * contains doc_id and count
 */
typedef struct doc {
	uint32_t id;
	uint32_t count;
} doc_t;


/*
 * Hashtable entry structure
 * Contains word and doc_queue
 */
typedef struct entry {
	char *word;
	queue_t *doc_queue;
} entry_t;
