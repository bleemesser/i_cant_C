/* 
 * pageio.c --- pageload and pagesave
 * 
 * Description: 
 * 
 */

#include "pageio.h"
#include <sys/stat.h>

/*
 * Write a fetched page to the file <dirname>/<id>
 * The directory must already exist, `id` must be positive (>0), and the
 * page must have a url and fetched html.
 *
 * FILE FORMAT:
 * ```
 * <url>
 * <depth>
 * <length of html>
 * <html content>
 * ```
 *
 * Returns 0 on success, nonzero on error.
 */
int32_t pagesave(webpage_t *pagep, int32_t id, char *dirname) {
    if (pagep == NULL || dirname == NULL || webpage_getURL(pagep) == NULL ||
        webpage_getHTML(pagep) == NULL) {
        return 1;
    }

    if (id <= 0) {
        return 1;
    }

    struct stat st;
    if (stat(dirname, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return 1;
    }

    // 12 bytes = "2147483647" + '/' + '\0'
    if (strlen(dirname) + 12 >= MAX_FILENAME_BYTES) {
        return 1;
    }

    char filename[MAX_FILENAME_BYTES];
    sprintf(filename, "%s/%d", dirname, id);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        return 1;
    }

    // getNextURL compresses the html and does not update the internal
    // html_len field, so webpage_getHTMLlen will return the wrong value
    // need to measure the shortened string instead
    int html_len_bytes = (int)strlen(webpage_getHTML(pagep));

    fprintf(fp, "%s\n%d\n%d\n", webpage_getURL(pagep), webpage_getDepth(pagep),
            html_len_bytes);

    fwrite(webpage_getHTML(pagep), sizeof(char), html_len_bytes, fp);

    if (ferror(fp) || fclose(fp) == EOF) {
        return 1;
    }

    return 0;
}

/* 
 * pageload -- loads the numbered filename <id> in directory <dirnm>
 * into a new webpage
 *
 * returns: non-NULL for success; NULL otherwise
 */
webpage_t *pageload(int id, char *dirname) {
	if (id <= 0 || dirname == NULL) {
		return NULL;
	}

	struct stat st;
  if (stat(dirname, &st) != 0 || !S_ISDIR(st.st_mode)) {
    return NULL;
  }

	// 12 bytes = "2147483647" + '/' + '\0'
  if (strlen(dirname) + 12 >= MAX_FILENAME_BYTES) {
    return NULL;
  }

  char filename[MAX_FILENAME_BYTES];
  sprintf(filename, "%s/%d", dirname, id);

	FILE *fp = fopen(filename, "r");

	if (fp == NULL) {
		return NULL;
	}

	char url[512];
	char depth_s[10];
	char len_s[10];
	
	// get first three lines
	if (fgets(url, sizeof(url), fp) == NULL || fgets(depth_s, sizeof(depth_s), fp) == NULL || fgets(len_s, sizeof(len_s), fp) == NULL) {
		return NULL;
	}

	long depth_l = strtol(depth_s);
	long len_l = strtol(len_s);

	//pray it worked
	
	int32_t depth = (int32_t) depth_l;
	int32_t len = (int32_t) len;
	
	char *html = calloc(1, len + 1);

	if (fread(html, 1, len, fp) != len) {
		return NULL;
	}

	return webpage_new(url, depth, html);
}
