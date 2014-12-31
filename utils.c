#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char **alloc_strcat(char **dest, char const *src) {
	#ifdef DEBUG
	puts(__func__);
	#endif

	int destlen = 0, srclen = 0,wasdestnull = 0;
	destlen = (*dest)?strlen(*dest):0;
	srclen = strlen(src);
	wasdestnull = !(*dest);
	*dest = (char*)realloc(*dest, (destlen + srclen + 1)*sizeof(char));
	if (dest) {
		if (wasdestnull) {
			memset(*dest, 0, strlen(src) + 1);
		}
		strncat(*dest, src, strlen(src));
	} else {
		fprintf(stderr,"realloc failed\n");
	}
	return dest;
}

char const utf8_skip_data[256] = {
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

char *utf8_next_char(char const * const p) {
	return (char *)((p) + utf8_skip_data[*(unsigned char *)(p)]);
}

int utf8_strlen(char const *p) {
	int i;
	if (!p) return 0;
	for (i = 0; *p; i++) {
		p = utf8_next_char(p);
	}
	return i;
}

char *utf8_offset_to_pointer(char const *str, int offset) {
	if (offset < 0) {
		return NULL;
	}
	for(;offset--;) {
		str = utf8_next_char(str);
	}
	return (char *)str;
}

int utf8_pointer_to_offset(char const *str, char const * const pos) {
	int ret = 0;
	if (pos < str) {
		ret = - utf8_pointer_to_offset (pos, str);
	} else {
		for(; str < pos; ret++) {
			str = utf8_next_char(str);
		}
	}
	return ret;
}
