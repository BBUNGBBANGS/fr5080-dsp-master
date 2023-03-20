/* memmove function */
#include <string.h>
_C_LIB_DECL

void *(memmove)(void *s1, const void *s2, size_t n)
	{	/* copy char s2[n] to s1[n] safely */
	char *sc1 = (char *)s1;
	const char *sc2 = (const char *)s2;

	if (sc2 < sc1 && sc1 < sc2 + n)
		for (sc1 += n, sc2 += n; 0 < n; --n)
			*--sc1 = *--sc2;	/* copy backwards */
	else
		for (; 0 < n; --n)
			*sc1++ = *sc2++;	/* copy forwards */
	return (s1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
