/* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2007 Diego Casorran
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Contributor(s):
 *   Diego Casorran <dcasorran@gmail.com> (Original Author)
 * 
 * ***** END LICENSE BLOCK ***** */


#include "globals.h"
#undef strlen

/****************************************************************************/

struct RawDoFmtStream {
	
	STRPTR Buffer;
	LONG Size;
};

static void RawDoFmtChar( REG(d0,UBYTE c), REG(a3,struct RawDoFmtStream *s))
{
	if(s->Size > 0)
	{
		*(s->Buffer)++ = c;
		 (s->Size)--;
		
		if(s->Size == 1)
		{
			*(s->Buffer)	= '\0';
			  s->Size	= 0;
		}
	}
}

LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args)
{
	long rc = 0;
	
	if((size > (long)sizeof(char)) && (outbuf != NULL))
	{
		struct RawDoFmtStream s;
		
		s.Buffer = outbuf;
		s.Size	 = size;
		
		RawDoFmt( fmt, (APTR)args, (void (*)())RawDoFmtChar, (APTR)&s);
		
		if((rc = ( size - s.Size )) != size)
			--rc;
	}
	
	return(rc);
}

LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... )
{
	va_list args;
	long rc;
	
	va_start (args, fmt);
	rc = VSNPrintf( outbuf, size, fmt, args );
	va_end(args);
	
	return(rc);
}

LONG SPrintf( STRPTR outbuf, CONST_STRPTR fmt, ... )
{
	va_list args;
	long rc;
	
	va_start (args, fmt);
	rc = VSNPrintf( outbuf, 2147483647, fmt, args );
	va_end(args);
	#warning 2147483647? hmmm....
	
	return(rc);
}

/****************************************************************************/


/****************************************************************************/


/****************************************************************************/

void bcopy(const void *src,void *dest,size_t len)
{
	CopyMem((APTR) src, dest, len );
}

/****************************************************************************/

void bzero( void *data, size_t fsize )
{
	register unsigned long * uptr = (unsigned long *) data;
	register unsigned char * sptr;
	long size = (long) fsize;
	
	// first blocks of 32 bits
	while(size >= (long)sizeof(ULONG))
	{
		*uptr++ = 0;
		size -= sizeof(ULONG);
	}
	
	sptr = (unsigned char *) uptr;
	
	// now any pending bytes
	while(size-- > 0)
		*sptr++ = 0;
}

/****************************************************************************/

void *memset(void *ptr, int val, unsigned long len)
{
	void * result = ptr;
	register unsigned char * m = ptr;
	
	if( val == 0 )
	{
		bzero( ptr, len );
	}
	else
	{
		while(len-- > 0)
			(*m++) = val;
	}
	return(result);
}

/****************************************************************************/

size_t strlen(const char *string)
{ const char *s=string;
  
  if(!(string && *string))
  	return 0;
  
  do;while(*s++); return ~(string-s);
}

/****************************************************************************/

char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!*s++)
      { s = (char *)0; break; }
  return (char *)s;
}

/****************************************************************************/
/*
 * Portable ISO 'C' (1994) runtime library for the Amiga computer
 * Copyright (c) 2002-2006 by Olaf Barthel <olsen (at) sourcery.han.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Neither the name of Olaf Barthel nor the names of contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Address is neither aligned to a word or long word boundary. */
#define IS_UNALIGNED(a) ((((unsigned long)(a)) & 1) != 0)

/* Address is aligned to a word boundary, but not to a long
   word boundary. */
#define IS_SHORT_ALIGNED(a) ((((unsigned long)(a)) & 3) == 2)

/* Address is aligned to a long word boundary. For an 68030 and beyond the
   alignment does not matter. */
#if defined(M68020) || defined(_M68020) || defined(mc68020) || defined(__mc68020)
#define IS_LONG_ALIGNED(a) (1)
#else
#define IS_LONG_ALIGNED(a) ((((unsigned long)(a)) & 3) == 0)
#endif /* M68020 */

INLINE VOID __memmove(unsigned char * to,unsigned char * from,size_t len)
{
	if(from < to && to < from + len)
	{
		to		+= len;
		from	+= len;

		/* The setup below is intended to speed up copying larger
		 * memory blocks. This can be very elaborate and should not be
		 * done unless a payoff can be expected.
		 */
		if(len > 4 * sizeof(long))
		{
			size_t distance;

			/* Try to align both source and destination to an even address. */
			if(IS_UNALIGNED(to) && IS_UNALIGNED(from))
			{
				(*--to) = (*--from);
				len--;
			}

			/* Try to align both source and destination to addresses which are
			 * multiples of four.
			 */
			if(len >= sizeof(short) && IS_SHORT_ALIGNED(to) && IS_SHORT_ALIGNED(from))
			{
				(*--to) = (*--from);
				(*--to) = (*--from);

				len -= sizeof(short);
			}

			/* Check the distance between source and destination. If it's shorter
			 * than a long word, don't dive into the copying routine below since
			 * the overlapping copying may clobber data.
			 */
			distance = (size_t)(to - from);

			/* If both source and destination are aligned to addresses which are
			 * multiples of four and there is still enough data left to be copied,
			 * try to move it in larger chunks.
			 */
			if(distance >= sizeof(long) && len >= sizeof(long) && IS_LONG_ALIGNED(to) && IS_LONG_ALIGNED(from))
			{
				unsigned long * _to		= (unsigned long *)to;
				unsigned long * _from	= (unsigned long *)from;

				/* An unrolled transfer loop, which shifts 32 bytes per iteration. */
				while(len >= 8 * sizeof(long))
				{
					/* The following should translate into load/store
					   opcodes which encode the access offsets (-1..-8)
					   into the respective displacement values. This
					   should help the PowerPC by avoiding pipeline
					   stalls (removing the predecrement on the address
					   will do that) but has no noticeable impact on the
					   68k platform (I checked). */

					_to[-1] = _from[-1];
					_to[-2] = _from[-2];
					_to[-3] = _from[-3];
					_to[-4] = _from[-4];
					_to[-5] = _from[-5];
					_to[-6] = _from[-6];
					_to[-7] = _from[-7];
					_to[-8] = _from[-8];

					_to		-= 8;
					_from	-= 8;

					/*
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					(*--_to) = (*--_from);
					*/

					len -= 8 * sizeof(long);
				}

				/* Try to mop up any small amounts of data still in need of
				 * copying...
				 */
				while(len >= sizeof(long))
				{
					(*--_to) = (*--_from);

					len -= sizeof(long);
				}		

				to		= (char *)_to;
				from	= (char *)_from;
			}
		}

		/* If there's anything left, copy the rest. */
		while(len-- > 0)
			(*--to) = (*--from);
	}
	else
	{
		/* The setup below is intended to speed up copying larger
		 * memory blocks. This can be very elaborate and should not be
		 * done unless a payoff can be expected.
		 */
		if(len > 4 * sizeof(long))
		{
			size_t distance;

			/* Try to align both source and destination to an even address. */
			if(IS_UNALIGNED(to) && IS_UNALIGNED(from))
			{
				(*to++) = (*from++);
				len--;
			}

			/* Try to align both source and destination to addresses which are
			 * multiples of four.
			 */
			if(len >= sizeof(short) && IS_SHORT_ALIGNED(to) && IS_SHORT_ALIGNED(from))
			{
				(*to++) = (*from++);
				(*to++) = (*from++);

				len -= sizeof(short);
			}

			/* Check the distance between source and destination. If it's shorter
			 * than a long word, don't dive into the copying routine below since
			 * the overlapping copying may clobber data.
			 */
			if(to >= from)
				distance = (size_t)(to - from);
			else
				distance = (size_t)(from - to);

			/* If both source and destination are aligned to addresses which are
			 * multiples of four and there is still enough data left to be copied,
			 * try to move it in larger chunks.
			 */
			if(distance >= sizeof(long) && len >= sizeof(long) && IS_LONG_ALIGNED(to) && IS_LONG_ALIGNED(from))
			{
				unsigned long * _to		= (unsigned long *)to;
				unsigned long * _from	= (unsigned long *)from;

				/* An unrolled transfer loop, which shifts 32 bytes per iteration. */
				while(len >= 8 * sizeof(long))
				{
					/* The following should translate into load/store
					   opcodes which encode the access offsets (0..7)
					   into the respective displacement values. This
					   should help the PowerPC by avoiding pipeline
					   stalls (removing the postincrement on the address
					   will do that) but has no noticeable impact on the
					   68k platform (I checked). */

					_to[0] = _from[0];
					_to[1] = _from[1];
					_to[2] = _from[2];
					_to[3] = _from[3];
					_to[4] = _from[4];
					_to[5] = _from[5];
					_to[6] = _from[6];
					_to[7] = _from[7];

					_to		+= 8;
					_from	+= 8;

					/*
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					(*_to++) = (*_from++);
					*/

					len -= 8 * sizeof(long);
				}

				/* Try to mop up any small amounts of data still in need of
				 * copying...
				 */
				while(len >= sizeof(long))
				{
					(*_to++) = (*_from++);

					len -= sizeof(long);
				}		

				to		= (char *)_to;
				from	= (char *)_from;
			}
		}

		/* If there's anything left, copy the rest. */
		while(len-- > 0)
			(*to++) = (*from++);
	}
}

/****************************************************************************/

void * memmove(void *dest, const void * src, size_t len)
{
	void * result = dest;

	if(!( (len == 0) || (dest != NULL && src != NULL && (int)len > 0) )) {
	//	errno = EFAULT;
		goto out;
	}

	if(len > 0 && dest != src)
	{
		char * to = dest;
		const char * from = src;

		#if 0
		{
			if(from < to && to < from + len)
			{
				to		+= len;
				from	+= len;

				while(len-- > 0)
					(*--to) = (*--from);
			}
			else
			{
				while(len-- > 0)
					(*to++) = (*from++);
			}
		}
		#else
		{
			__memmove((unsigned char *)to,(unsigned char *)from,len);
		}
		#endif
	}

 out:

	return(result);
}

/****************************************************************************/

int strncmp(const char *s1,const char *s2,size_t n)
{ unsigned char *p1=(unsigned char *)s1,*p2=(unsigned char *)s2;
  unsigned long r,c;

  if ((r=n))
    do;while(r=*p1++,c=*p2++,!(r-=c) && (char)c && --n);
  return r;
}

int strcmp(const char *s1, const char * s2)
{
	if((s1 != NULL) && (s2 != NULL) && (s1 != s2))
	{
		while (*s1 == *s2++)
			if (*s1++ == 0)
				return (0);
		return(*(const unsigned char *)s1 - *(const unsigned char *)--s2);
	}
	
	return (0);
}

/****************************************************************************/

size_t strncpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

size_t strcpy(char *dst, const char *src)
{
	return strncpy( dst, src, strlen(src));
}

/****************************************************************************/

int
memcmp(s1, s2, n)
	const void *s1, *s2;
	size_t n;
{
//	_DIAGASSERT(s1 != 0);
//	_DIAGASSERT(s2 != 0);

	if (n != 0) {
		const unsigned char *p1 = s1, *p2 = s2;

		do {
			if (*p1++ != *p2++)
				return (*--p1 - *--p2);
		} while (--n != 0);
	}
	return (0);
}

/****************************************************************************/

/****************************************************************************/

