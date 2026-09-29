/* 
 * style.c --- style sheet for C programmers
 * 
 * Author: Stephen Taylor
 * Created: 10-16-2020
 * Version: 1.0
 * 
 * Description: This little sheet shows examples of how to lay out
 * code. Be sure to READ THE COMMENTS and USE THE TAB key in emacs to
 * ensure code alligns structure for readability and maintainability.
 *
 * Do not be clever or verbose -- keep your code simple, readable, and
 * concise. If you are forced into something obscure, comment it well.
 *
 * To insert and align comments use: ESC-; 
 * To justify comment paragraphs use: ESC-q
 *
 * This header is inserted when you open a new .c or .h file; on an
 * existing file position cursor at the top of the file using Esc-<
 * and execute: Esc-x make-header
 * 
 */
#include <style.h>		/* include your module header file first; it
												 includes everything needed to use the module
												 and carefully documents the module interface
												 -- access it through the include path (-I)
												 specified in your makefile */

/* Next define your constatnts */
#define MAVAL1 100							/* use ALL UPPERCASE for CONSTANTS */
#define MAVAL2 200							
#define XVAL1 1
#define XVAL2 2
#define XVAL3 3

/* Then your type definitions */
typedef struct foo {						/* note the name of the structure */
	double y;
	char *xp; /* end pointer names with p, associate the * with the variable not the type */
	int x;
} foo_t;		/* note use of _t to designate a user defined type */

/* 
 * Next declare global variables, try to AVOID using global variables,
 * if you must use them always mark as static and never share across
 * modules to control their scope
 */
static int x;	
static double foo[MAXVAL1]; 		/* declare arrays using defined constants */

/* 
 * Carefully mark all functions that are not listed in the module
 * interface header file as static
 */
static int32_t foo_func1(char *sp, int x, double y) { /* also ok to put brace on separate line */
	int i;										/* declare variables at top of function, in lowercase */
	int *xp,*yp,xval;					/* group related variables, end pointer names with p */
	double *dp;								/* do not use long names for simple indexes and ideas */

	/*
	 * Example loops -- AVOID use of continue and break in loops 
	 */
	for(i=0; i<MAXVAL1; i++)			/* note use of spaces and use of constants in loops*/
		do_something(sp,foo[i]);    /* one statement in block so no braces {}  */

	for(i=0; i<7; i++) {					/* note use of spaces and brace for separation */
		do_something(sp,i);					/* multiple statements in block use braces {}  */
		do_something_else(sp,i,y);	/* note tab alignment of code */
	}															/* note positiion of closing brace under for */

	/* 
	 * Example if/case statements 
	 */
	if(x>7)
		do_something();							/* single statement in if - no braces */
	else 													/* else aligned with associated if */
		do_something_else();				/* single statement else - no braces */

	
	if(x>7) {											/* note position of braces */
		do_something1();						/* multi-statement in if */
		do_something2();					
		do_something3();					
	}
	else {												/* note position of else under if and braces */
		do_something1();						/* multi-statement else */
		do_something2();						/* tab align code */
		do_something3();					
	}

	/* Example Switch statement */
	switch(xval) {
	case XVAL1:										/* no braces in switch entries */
		dosomthing();
		break;											/* typically use break on every clause  */
	case XVAL2:
		dosomething();							/* align multiple statements in each entry */
		dosomethingmore();
		break;
	case XVAL3:
		dosomething();
		dosomethingeelse();
		dosomethingmore();
		break;
	default:											/* include a default */
		dosomedefaultthing();
		break;											/* be consistent with break */
	}
			
	/* 
	 * Return statements -- if function has more than 20 lines of code,
	 * use ONLY ONE return as LAST statement at last line in function.
	 */
	return 0;	 /* Generally use 0 to mean success when returning error codes */
}




/* ------ style.h - SEPARATE FILE BUT SHOWN HERE FOR COMPLETENESS------------- */
#pragma once					/* FIRST LINE: ensure you can include file multiple times */

#include <stdio.h>		/* include any needed standard header files to use module */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>							/* include stdxxx files first, then ancillary files */

/* DO NOT INCLUDE THIS .h FILE */

/*
 * prototypes for functions exported by module NOTE: carefully
 * document each prototype inputs, errors, and result values so that
 * anyone ussing the module can look at the interface to understand
 * the functions it provides
 */
void funct1(int x, double y, char *sp);
int funct2(int x, double y, char *sp);
double funct3(int x, double y, char *sp);




/* -----MAIN PROGRAM -- SEPARATE FILE ---------------------------------------- */

/* 
 * Generally, place your main function is a separate file from 
 */

#include <stdio.h>							/* include any needed standard header files */
#include <stdlib.h>							/* include stdlib.h for exit() */
#include <style.h>							/* include your module header files from include path */

int main(int argc, char *argv[]) {

	/* process all arguments first */
	/* if you get a bad argument, print usage statement and exit(EXIT_FAILURE) */
	
	if(somethingbadhappens())				
		exit(EXIT_FAILURE);					/* ONLY use exit() in main function */
	
	exit(EXIT_SUCCESS);						/* default fallthrough action - exit() with success */
}


/* ----------------------------------- */
