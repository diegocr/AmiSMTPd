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

#ifndef SMTPD_TRANSPORT_H
#define SMTPD_TRANSPORT_H

/** if(__FORWARD__ == 1) RELAY_FOR_USERS goto SYSADMIN_EMAIL,
 ** else store them on SYSADMIN_MBOX.			***/
#define __FORWARD__			0

#define RECVLN_BUFLEN			(1024 * 16) /* Check NOSTDLIB_STACK! */
#define RECVLN_TIMEOUT			20	/* seconds */


// STRING LENGTHS, rfc821
# define	USER_LENGTH		64
# define	DOMAIN_LENGTH		64
# define	PATH_LENGTH		256
# define	CMDLN_LENGTH		512
# define	REPLYLN_LENGTH		512
# define	TXTLN_LENGTH		1000


#endif /* SMTPD_TRANSPORT_H */
