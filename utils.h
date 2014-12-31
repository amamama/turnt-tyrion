#ifndef __UTILS_H
#define __UTILS_H

extern char const utf8_skip_data[256];

char **alloc_strcat(char **dest, char const *src);

char *utf8_next_char(char const * const p);
int utf8_strlen(char const *p);
char *utf8_offset_to_pointer(char const *str, int offset);
int utf8_pointer_to_offset(char const *str, char const * const pos);

#endif
