
#ifndef INLINE
# define INLINE	static __inline
#endif

/** standards inlined:
 **********************************************************************/

#define strlen		__strlen
#define strstr		__strstr

INLINE size_t strlen(const char *string)
{ const char *s;
#if 1
  if(!(string && *string))
  	return 0;
#endif
  s=string;
  do;while(*s++); return ~(string-s);
}

INLINE char *strstr(const char *s1,const char *s2)
{ const char *c1,*c2;

  do {
    c1 = s1; c2 = s2;
    while(*c1 && *c1==*c2) {
      c1++; c2++;
    }
    if (!*c2)
      return (char *)s1;
  } while(*s1++);
  return (char *)0;
}
/**********************************************************************/


#ifdef NEED_SEPSET
static unsigned char string_set[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

INLINE char *string_sep_set_func (char *string, const char *charset)
{
	unsigned char *str_ptr;
	unsigned char *ptr;

	for (ptr = (unsigned char *)charset; *ptr; ptr++)
		string_set[(int)*ptr] = 1;

	for (str_ptr = string; *str_ptr; str_ptr++)
	{
		if (string_set[(int)*str_ptr])
			break;
	}

	for (ptr = (unsigned char *)charset; *ptr; ptr++)
		string_set[(int)*ptr] = 0;

	if (!str_ptr[0])
		str_ptr = NULL;

	return str_ptr;
}

INLINE char *string_sep_set (char **string, const char *charset)
{
	char *iter, *str;

	if (!string || !*string || !**string)
		return NULL;

	str = *string;

	if((iter = string_sep_set_func(str, charset)))
	{
		*iter = 0;
		iter += sizeof (char);
	}

	*string = iter;

	return str;
}

#endif /* NEED_SEPSET */

//#ifdef NEED_STRTRIM
INLINE char *string_move(char *dst, const char *src)
{
	if (!dst || !src)
		return dst;

	return memmove (dst, src, strlen (src) + 1);
}

INLINE char *string_trim (char *string)
{
	char *ptr;

	if (!string || !string[0])
		return string;

	/* skip leading whitespace */
	for (ptr = string; (*ptr) ==  0x20; ptr++);

	/* shift downward */
	if (ptr != string)
		string_move (string, ptr);

	if (!string[0])
		return string;

	/* look backwards */
	ptr = string + strlen (string) - 1;

	if((*ptr)==' ')
	{
		while(ptr >= string && ((*ptr) == 0x20))
			ptr--;

		ptr[1] = 0;
	}

	return string;
}
//#endif

INLINE char *string_sep (char **string, const char *delim)
{
	char *iter, *str;
	
	if (!string || !*string || !**string)
		return NULL;

	str = *string;

	if((iter = strstr(str, delim)))
	{
		*iter = 0;
		iter += strlen(delim);
	}

	*string = iter;

	return str;

}

#define _strsep		string_sep
#define _stesep_set	string_sep_set
#define _strtrim	string_trim
#define _strmove	string_move

