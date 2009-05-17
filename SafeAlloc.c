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


#include <proto/exec.h>

STATIC APTR memPool = NULL;
STATIC struct SignalSemaphore * memSemaphore = NULL;

STATIC struct SignalSemaphore * __create_semaphore(void)
{
	struct SignalSemaphore * semaphore;

	#if defined(__amigaos4__)
	{
		semaphore = AllocSysObject(ASOT_SEMAPHORE,NULL);
	}
	#else
	{
		semaphore = AllocVec(sizeof(*semaphore),MEMF_ANY|MEMF_PUBLIC);
		if(semaphore != NULL)
			InitSemaphore(semaphore);
	}
	#endif /* __amigaos4 */

	return(semaphore);
}

STATIC void __delete_semaphore(struct SignalSemaphore * semaphore)
{
	if(semaphore != NULL)
	{
		#if defined(__amigaos4__)
		{
			FreeSysObject(ASOT_SEMAPHORE,semaphore);
		}
		#else
		{
			//assert( semaphore->ss_Owner == NULL );

			#if defined(DEBUG)
			{
				/* Just in case somebody tries to reuse this data
				   structure; this should produce an alert if
				   attempted. */
				memset(semaphore,0,sizeof(*semaphore));
			}
			#endif /* DEBUG */

			FreeVec(semaphore);
		}
		#endif /* __amigaos4 */
	}
}

BOOL InitMallocSystem( VOID )
{
	if((memSemaphore = __create_semaphore ( )) != NULL)
	{
		if((memPool = CreatePool( MEMF_PUBLIC|MEMF_CLEAR, 4096, 512 )))
		{
			return TRUE;
		}
		
		__delete_semaphore( memSemaphore );
	}
	
	return FALSE;
}

VOID ClearMallocSystem( VOID )
{
	__delete_semaphore( memSemaphore );
	memSemaphore = NULL;
	
	if(memPool != NULL)
	{
		DeletePool( memPool );
		memPool = NULL;
	}
}

APTR Malloc( ULONG s )
{
	ULONG *mem;
	LONG size = s;

	if( size <= 0 )
		return NULL;
	
	size += sizeof(ULONG) + MEM_BLOCKMASK;
	size &= ~MEM_BLOCKMASK;
	
	ObtainSemaphore(memSemaphore);
	
	if((mem = AllocPooled(memPool,size)))
	{
		*mem++=size;
	}
	ReleaseSemaphore(memSemaphore);
	
	return mem;
}

VOID Free( APTR m )
{
	ULONG *mem = (ULONG *) m;
	
	if(mem && memPool)
	{
		ULONG size = (ULONG) (*(--mem));
		
		ObtainSemaphore(memSemaphore);
		
		if(((long)size) > 0) {
			FreePooled( memPool, mem, size);
		}
		ReleaseSemaphore(memSemaphore);
	}
}

APTR Realloc( APTR old, ULONG ns )
{
	APTR new;
	LONG osize, *o = old;
	LONG nsize = ns;

	if (!old)
		return Malloc( nsize );
	
	osize = (*(o-1)) - sizeof(ULONG);
	if(nsize <= osize)
		return old;
	
	if((new = Malloc( nsize )))
	{
		ULONG *n = (ULONG *) new;
		
		osize >>= 2;
		while(osize--)
		{
			*n++ = *o++;
		}
		
		Free( old );
	}
	
	return new;
}



char * StrnDup( const char * src, unsigned long size )
{
	char * dst = NULL;
	
	if((((long)size) > 0) && (src != NULL))
	{
		if((dst = Malloc( size+2 )))
		{
			CopyMem((APTR) src, dst, size );
			dst[size] = '\0';
		}
	}
	
	return dst;
}

extern int strlen(const char *str);

char * StrDup( const char * src )
{
	return (src != NULL) ? StrnDup(src, strlen(src)+1) : NULL;
}


