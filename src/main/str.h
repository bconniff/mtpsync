/**
 * @file str.h
 * Reusable functions for working with strings.
 */

#ifndef _STR_H_
#define _STR_H_

/**
 * Checks if a string starts with a specified prefix.
 * @param str     string to check
 * @param prefix  prefix to look for
 * @return        non-zero if str begins with the prefix, zero otherwise
 */
int str_starts_with(char* str, char* prefix);

/**
 * Checks if a string ends with a specified suffix.
 * @param str     string to check
 * @param suffix  suffix to look for
 * @return        non-zero if str ends with the suffix, zero otherwise
 */
int str_ends_with(char* str, char* suffix);

/**
 * Joins multiple strings together.
 * Allocates the result on the heap, free it when done.
 * @param argc  number of arguments to join
 * @param ...   strings to be joined
 * @return      joined result, or NULL if an error occurred
 */
char* str_join(size_t argc, ...);

/**
 * Converts a string to lowercase. Does not mutate the original string.
 * Allocates the result on the heap, free it when done.
 * @param str  to convert to lowercase
 * @return     joined result, or NULL if an error occurred
 */
char* str_lower(char* str);

/**
 * Converts a string to uppercase. Does not mutate the original string.
 * Allocates the result on the heap, free it when done.
 * @param str  to convert to uppercase
 * @return     joined result, or NULL if an error occurred
 */
char* str_upper(char* str);

#endif
