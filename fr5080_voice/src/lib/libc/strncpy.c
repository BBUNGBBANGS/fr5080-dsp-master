/* strncpy function */
#include <string.h>
_C_LIB_DECL

char *(strncpy)(char *_Restrict s1, const char *_Restrict s2,
	size_t n)
	{	/* copy char s2[max n] to s1[n] */
	char *s;

	for (s = s1; 0 < n && *s2 != '\0'; --n)
		*s++ = *s2++;	/* copy at most n chars from s2[] */
	for (; 0 < n; --n)
		*s++ = '\0';
	return (s1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
