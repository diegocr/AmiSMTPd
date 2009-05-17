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

#ifndef SMTPD_GLOBALS_H
#define SMTPD_GLOBALS_H

#define _PROGRAM_NAME		"AmiSmtpD"
#define _PROGRAM_VERZ		"1.0"
#define _PROGRAM_DATE		"30.01.2007"
#define _PROGRAM_COPY		"Copyright (C) 2007 Diego Casorran - http://amiga.sf.net/"

#define PROGROOTDIR		_PROGRAM_NAME ":"
#define SPOOLDIR		PROGROOTDIR "spool/"
#define MAILBOX_PATHPART	"mailbox"
#define MAILBOX			PROGROOTDIR MAILBOX_PATHPART "/"
#define MAILBOX_AUTHS		MAILBOX ".auth" 		/* pop3.c */

#define SMTP_LOGGER_FILENAME	PROGROOTDIR ".log"		/* spooler.c */
#define MXDNS_CACHE_FILE	PROGROOTDIR ".dns.cache"	/* spooler.c */
#define FADDY_FILE		PROGROOTDIR ".faddy.txt"	/* spool_write.c */

#define STACK_SIZE		65535 /* program's stack size */

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <proto/utility.h>

#include <SDI_compiler.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <amitcp/socketbasetags.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "thread.h"
#include "casolib.h"
#include "utils.h"

THREAD_DECL(spooler);
__inline STRPTR LocalSMTPHost(void);

void _vsyslog( long level, const char *fmt, LONG *args);
char *l_enostr(void);

#define stricmp Stricmp
#define strnicmp Strnicmp

extern int h_errno, errno;

#define SMTP_SOCK	G._iSock[0]
#define POP3_SOCK	G._iSock[1]

struct Globals
{
//	char *host;
	ULONG port;
	LONG NetSig, _iSock[2];
	
	struct DosLibrary	*DOSBase;
	struct Library		*SocketBase;
};
struct EMail
{
	char *ip, *host;
	char *helo;
	char *from;
	char *to;
	String *email_data;
	char *spool;
	
	BOOL relay;
	char *relay_user;
	
	STRPTR xtra_header;
};
struct pop3
{
	STRPTR user, pass;
	
	long queue; /* number of email for that user */
	long size; /* full size of all emails */
	
	struct {
		STRPTR files[999];
		LONG sizes[999];
		BOOL deleted[999];
		ULONG uidl[999];
		/* #warning fixme, use linked list */
		
	} mbox;
};
extern struct Globals G;
extern struct pop3 P;

#undef syslog
#define syslog( level, fmt, args...)					\
	({								\
		ULONG _tags[] = { args }; _vsyslog( level, fmt, _tags);	\
	})
#define nfo(args...)	syslog(  LOG_INFO	, args)
#define warn(args...)	syslog(  LOG_WARN	, args)
#define err(args...)	syslog(  LOG_ERR	, args)
#define say(args...)	syslog(  LOG_NOTICE	, args)
#define deb(args...)	syslog(  LOG_DEBUG	, args)

#define SPOOLER_TASK	th_spawn(NULL, "smtpd spooler", &spooler, 1, &G )

#define FORBID( var, val)		\
	Forbid();	var = val;	Permit()

#define FORBIDIF( exp, var, val)	\
	Forbid();	if(exp)	var = val;	Permit()

#define FORBIDCALL( exp, func)		\
	Forbid();	if((exp))	{Permit(); func;}else Permit()


GLOBAL BOOL InitMallocSystem( VOID );
GLOBAL VOID ClearMallocSystem( VOID );
GLOBAL APTR Malloc( ULONG s );
GLOBAL APTR Realloc( APTR old, ULONG ns );
GLOBAL VOID Free( APTR m );
GLOBAL char * StrnDup( char * src, unsigned long size );
GLOBAL char * StrDup( char * src );

extern char __dosname[], __bsdsocketname[];

#endif /* SMTPD_GLOBALS_H */
