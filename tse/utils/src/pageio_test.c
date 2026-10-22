/* 
 * pageio_test.c --- test for pageio.c
 * 
 * Description: 
 * 
 */

#include "pageio.h"
#include "webpage.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int32_t main(void) {
	int fails = 0;
	char url[] = "https://thayer.github.io/engs50/";
	int32_t depth = 0;
	int32_t id = 1;
	char dirname[] = "test";

	//initialize page
	webpage_t *page_1 = webpage_new(url, depth, NULL);
	webpage_fetch(page_1);

	if (pagesave(page_1, id, dirname) != 0) {
		printf("failure to save\n");
		exit(EXIT_FAILURE);
	}

	//load second page
	webpage_t *page_2 = pageload(id, dirname);
	if (page_2 == NULL) {
		printf("failure to load\n");
		exit(EXIT_FAILURE);
	}

	//check things
	char *url_1 = webpage_getURL(page_1);
	int depth_1 = webpage_getDepth(page_1);
	int len_1 = webpage_getHTMLlen(page_1);
	char *html_1 = webpage_getHTML(page_1);
	char *url_2 = webpage_getURL(page_2);
	int depth_2 = webpage_getDepth(page_2);
	int len_2 = webpage_getHTMLlen(page_2);
	char *html_2 = webpage_getHTML(page_2);

	if (strcmp(url_1, url_2) != 0) {
		fails++;
		printf("wrong url\n");
		printf("url_1: %s\n", url_1);
		printf("url_2: %s\n", url_2);
	}
	if (depth_1 != depth_2) {
		fails++;
		printf("wrong depth\n");
		printf("depth_1: %d\n", depth_1);
		printf("depth_2: %d\n", depth_2);
	}
	if (len_1 != len_2) {
		fails++;
		printf("wrong len\n");
		printf("len_1: %d\n", len_1);
		printf("len_2: %d\n", len_2);
	}
	if (strcmp(html_1, html_2) != 0) {
		fails++;
		printf("wrong html\n");
	}
	
	if (fails == 0) {
		printf("all tests passed\n");
		exit(EXIT_SUCCESS);
	}

	printf("%d failures", fails);
	exit(EXIT_FAILURE);
}
