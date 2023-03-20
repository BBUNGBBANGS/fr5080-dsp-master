/* memcmp function */
#include <string.h>
_C_LIB_DECL

int (memcmp)(const void *s1, const void *s2, size_t n)
	{	/* compare unsigned char s1[n], s2[n] */
	const unsigned char *su1 = (const unsigned char *)s1;
	const unsigned char *su2 = (const unsigned char *)s2;

	for (; 0 < n; ++su1, ++su2, --n)
		if (*su1 != *su2)
			return (*su1 < *su2 ? -1 : +1);
	return (0);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
