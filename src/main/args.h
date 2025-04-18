/**
 * @file args.h
 * Simple command-line argument parser.
 */

#ifndef _ARG_H_
#define _ARG_H_

/**
 * Status codes for arg parse results.
 */
typedef enum {
    ARG_STATUS_OK,       ///< Arg parsing succeeded
    ARG_STATUS_ESYNTAX,  ///< Parsing failed due to invalid syntax
    ARG_STATUS_ENOMEM,   ///< Failed due to allocation error
} ArgStatusCode;

/**
 * Callback function for handling arguments. Arguments are checked in order of
 * position, and when a matching ArgDefinition is found, the ArgHandlerFn is
 * called. The current index is passes as a reference. If the parameter expects
 * to consume an additional argument, the ArgHandlerFn should move the pointer
 * after the consumed arguments.
 * @param argc  total number of args in the argv array
 * @param argv  array of argument values
 * @param i     reference to the current index in the unparsed list of arguments
 * @param data  passed arugument data
 */
typedef ArgStatusCode (*ArgHandlerFn)(int argc, char** argv, int* i, void* data);

/**
 * Argument definitions
 */
typedef struct {
    char arg_short;       ///< Short version of the arg; use 0 to disable
    char* arg_long;       ///< Long version of the arg; use NULL to disable
    ArgHandlerFn arg_fn;  ///< Handler to call when the argument is matched
} ArgDefinition;

/**
 * Result of parsing arguments. This will contain the new set of positional
 * arguments, with all long and short arguments removed. When status is
 * #ARG_STATUS_OK, the argv member is allocated on the heap and should be freed
 * when done.
 */
typedef struct {
    ArgStatusCode status; ///< Status of the arg_parse() operation
    int argc;             ///< New count of positional arguments
    char** argv;          ///< New positional arguments, free it when done
} ArgParseResult;

/**
 * Parse command-line arguments. This will check each argument in the argv list
 * against the provided "defs" array of ArgDefinition structs. If the parameter
 * matches one of the provided definitions, the definition's associated
 * ArgHandlerFn will be called. Free the argv in the returned ArgParseResult
 * when done.
 * @param argc  total number of arguments in the argv array
 * @param argv  array of argument values
 * @param defc  total number of definitions in the defv array
 * @param defv  definitions for allowed command-line arguments
 * @param data  data that will be passed to argument handler function.
 * @return      parsed argument data
 */
ArgParseResult arg_parse(int argc, char** argv, size_t defc, ArgDefinition defv[], void* data);

#endif
