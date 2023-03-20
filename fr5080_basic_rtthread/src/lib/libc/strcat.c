/* strcat function */
#include <string.h>
_C_LIB_DECL

char *(strcat)(char *_Restrict s1, const char *_Restrict s2)
	{	/* copy char s2[] to end of s1[] */
	char *s;

	for (s = s1; *s != '\0'; ++s)
		;			/* find end of s1[] */
	for (; (*s = *s2) != '\0'; ++s, ++s2)
		;			/* copy s2[] to end */
	return (s1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
