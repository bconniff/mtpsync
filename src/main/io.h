/**
 * @file io.h
 * Reusable input/output functions.
 */

#ifndef _IO_H_
#define _IO_H_

/**
 * Prompt user for a y/n confirmation.
 * @param fmt   printf style format string for the confirmation prompt
 * @param ...   format arguments for the confirmation prompt
 * @return      non-zero when the user replies affirmatively, zero otherwise
 */
int io_confirm(const char* fmt, ...);

#endif
