/*
 * Crawler fetch one page
 */

#include <stdio.h>
#include <stdlib.h>

#include "webpage.h"
#include "hash.h"
#include "queue.h"

static void print_url(void *wp) {
	webpage_t *wip = wp;
	printf("%s\n", webpage_getURL(wip));
}

int main(void) {
	//root page creation
	webpage_t *root_page = webpage_new("https://thayer.github.io/engs50/", 0, NULL);

	if (!webpage_fetch(root_page)) {
		webpage_delete(root_page);
		printf("Fetch failed");
		exit(EXIT_FAILURE);
	}

	//queue creation
	queue_t *qp = qopen();
	if (qput(qp, root_page) != 0) printf("qput failed\n");
	
	//go through and get urls
	int pos = 0;
	char *result;

	while ((pos = webpage_getNextURL(root_page, pos, &result)) > 0) {
		if (IsInternalURL(result)) {
			webpage_t *new_page = webpage_new(result, 0, NULL);
			if (qput(qp, new_page) != 0) printf("qput failed\n");
		}
		free(result);
	}

	//print queue of webpages
	qapply(qp, print_url);

	//delete everything
	qapply(qp, webpage_delete);
	qclose(qp);
	
  exit(EXIT_SUCCESS);
}
