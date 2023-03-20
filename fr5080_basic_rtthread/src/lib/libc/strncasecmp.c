/* strncasecmp function */
#include <ctype.h>
#include <string.h>
_C_LIB_DECL

int (strncasecmp)(const char *s1_arg, const char *s2_arg, size_t n)
	{	/* compare case-insensitive s1_arg[max n], s2_arg[max n] */
	const unsigned char *s1 = (const unsigned char *)s1_arg;
	const unsigned char *s2 = (const unsigned char *)s2_arg;

	for (; 0 < n; ++s1, ++s2, --n)
		if (tolower(*s1) != tolower(*s2))
			return ((unsigned char)tolower(*s1)
				< (unsigned char)tolower(*s2) ? -1 : +1);
		else if (*s1 == '\0')
			return (0);
	return (0);
	}
_END_C_LIB_DECL

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
