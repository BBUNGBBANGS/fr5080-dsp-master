/* strcpy function */
#include <string.h>
_C_LIB_DECL

char *(strcpy)(char *_Restrict s1, const char *_Restrict s2)
	{	/* copy char s2[] to s1[] */
	char *s;

	for (s = s1; (*s++ = *s2++) != '\0'; )
		;
	return (s1);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
