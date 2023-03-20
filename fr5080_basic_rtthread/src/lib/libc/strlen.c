/* strlen function */
#include <string.h>
_C_LIB_DECL

size_t (strlen)(const char *s)
	{	/* find length of s[] */
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		;
	return (sc - s);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
