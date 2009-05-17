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


#include <errno.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <amitcp/socketbasetags.h>

extern char __bsdsocketname[];

/****** net.lib/strerror *****************************************************

    NAME
        strerror -- return the text for given error number

    SYNOPSIS
        string = strerror(error);

        char * strerror(int);

    FUNCTION
        This function returns pointer to the (English) string describing the
        error code given as argument. The error strings are defined for the
        error codes defined in <sys/errno.h>.

    NOTES
        The string pointed to by the return value should not be modified by
        the program, but may be overwritten by a subsequent call to this
        function.

    BUGS
        The strerror() prototype should be 
	const char *strerror(unsigned int); 
	However, the SAS C includes define it differently.

    SEE ALSO
        <netinclude:sys/errno.h>, perror(), PrintNetFault()
*****************************************************************************
*/

#ifdef notyet
const char *
strerror(unsigned int error)
#else
char *
strerror(int error)
#endif
{
  struct Library * SocketBase;
  ULONG taglist[3];
  
  if(!(SocketBase = OpenLibrary(__bsdsocketname,4)))
  	return "";

  taglist[0] = SBTM_GETVAL(SBTC_ERRNOSTRPTR);
  taglist[1] = error;
  taglist[2] = TAG_END;

  SocketBaseTagList((struct TagItem *)taglist);
  CloseLibrary(SocketBase);
  return (char *)taglist[1];
}
