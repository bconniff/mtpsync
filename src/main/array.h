/**
 * @file array.h
 * Array helper macros
 */

#ifndef _ARRAY_H_
#define _ARRAY_H_

/**
 * Determine the number of elements in an array.
 * @param a  array to calculate size of
 * @return   number of elements
 */
#define ARRAY_LEN(a) (sizeof(a) ? (sizeof(a) / sizeof((a)[0])) : 0)

#endif
