/* strcmp function */
#include <string.h>
_C_LIB_DECL

int (strcmp)(const char *s1, const char *s2)
	{	/* compare unsigned char s1[], s2[] */
	for (; *s1 == *s2; ++s1, ++s2)
		if (*s1 == '\0')
			return (0);
	return (*(unsigned char *)s1 < *(unsigned char *)s2
		? -1 : +1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
