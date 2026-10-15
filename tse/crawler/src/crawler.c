/*
 * Crawler fetch one page
 */

#include <stdio.h>
#include <stdlib.h>

#include "webpage.h"
#include "hash.h"
#include "queue.h"

int main(void) {
	webpage_t *root_page = webpage_new("https://thayer.github.io/engs50/", 0, NULL);

	if (!webpage_fetch(root_page)) {
		webpage_delete(root_page);
		exit(EXIT_FAILURE);
	}

	//go through and get urls
	int pos = 0;
	char *result;

	while ((pos = webpage_getNextURL(root_page, pos, &result)) > 0) {
		if (IsInternalURL(result)) {
			printf("Found internal url: %s\n", result);
		}
		
		else {
			printf("Found external url: %s\n", result);
		}
		
		free(result);
	}

	webpage_delete(root_page);
	
  exit(EXIT_SUCCESS);
}
