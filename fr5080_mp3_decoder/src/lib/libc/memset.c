/* memset function */
#include <string.h>
_C_LIB_DECL

void *(memset)(void *s, int c, size_t n)
	{	/* store c throughout unsigned char s[n] */
	const unsigned char uc = (unsigned char)c;
	unsigned char *su = (unsigned char *)s;

	for (; 0 < n; ++su, --n)
		*su = uc;
	return (s);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
