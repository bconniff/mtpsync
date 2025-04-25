#ifndef _ARRAY_H_
#define _ARRAY_H_

#define ARRAY_LEN(a) (sizeof(a) ? (sizeof(a) / sizeof((a)[0])) : 0)

#endif
