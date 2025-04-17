#ifndef _STR_H_
#define _STR_H_

#define STR_BUF_SIZE 128

int str_starts_with(char* str, char* prefix);
int str_ends_with(char* str, char* suffix);
char* str_join(size_t argc, ...);
char* str_lower(char*);
char* str_upper(char*);

#endif
