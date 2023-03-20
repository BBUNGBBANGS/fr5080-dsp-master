/* memcpy function */
#include <string.h>
_C_LIB_DECL

void *(memcpy)(void *_Restrict s1, const void *_Restrict s2, size_t n)
	{	/* copy char s2[n] to s1[n] in any order */
	char *su1 = (char *)s1;
	const char *su2 = (const char *)s2;

	for (; 0 < n; ++su1, ++su2, --n)
		*su1 = *su2;
	return (s1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
