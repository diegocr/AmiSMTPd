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


#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>

#define __NOLIBBASE__

#include <proto/dos.h>
#include <dos/dostags.h>

#include "thread.h"
#include "globals.h"
#include "mx_exchange.h"
#include "spool_write.h"
#include <casorran/debug.h>

/* this macro lets us long-align structures on the stack */
#define D_S(type,name)	\
	char a_##name[ sizeof( type ) + 3 ];	\
	type *name = ( type * ) ( ( LONG ) ( a_##name + 3 ) & ~3 );

struct Task *spooler_task = NULL;
BOOL got_new_mail = FALSE;

STATIC BPTR log_fh = 0;
STATIC int fError = 0, seRetrys = 0;
STATIC struct Library * SocketBase = NULL; // private to this thread, ofcourse
STATIC struct Library * DOSBase = NULL;

STATIC BOOL OpenThreadLibs( VOID )
{
	if(!(DOSBase = OpenLibrary(__dosname,0)))
		return FALSE;
	
	if(!(SocketBase = OpenLibrary(__bsdsocketname,4)))
		return FALSE;
	
	return TRUE;
}

STATIC VOID CloseThreadLibs( VOID )
{
	if(DOSBase != NULL) {
		CloseLibrary(DOSBase);
		DOSBase = NULL;
	}
	if(SocketBase != NULL) {
		CloseLibrary(SocketBase);
		SocketBase = NULL;
	}
}

//------------------------------------------------------------------------------

#define _vsyslog	__thread_vsyslog

#if 0
# define cdeb(fmt, args...)	deb("$%08lX: " ## fmt, (ULONG)t->task, args)
#else
# define cdeb( fmt, args...)						\
({									\
	ULONG _tags[] = {args};						\
									\
	deb( fmt, args);						\
									\
	logger( fmt , (APTR)_tags);					\
})
#endif

STATIC void _vsyslog( long level, const char *fmt, LONG *args)
{
	if( SocketBase != NULL )
		vsyslog( level, fmt, args);
}

//------------------------------------------------------------------------------

static void logger( const char *fmt, APTR args)
{
	static char line[2048];
	
	if(!log_fh || !DOSBase)
		return;
	
	CopyMem(_clockstr(NULL), line, 10);   line[10] = ' ';
	_vsnprintf( &line[11], sizeof(line) - 12, fmt, args);
	
	Seek( log_fh, 0, OFFSET_END);
	
	DBL( line ) DBL("\n")
	
	Write(log_fh, line, strlen(line));
	FPutC(log_fh, '\n');
}

//------------------------------------------------------------------------------

static void CloseSockFD( LONG *sock )
{
	long sockfd = *sock;
	
	shutdown( sockfd, 2 );
	CloseSocket( sockfd );
}

//------------------------------------------------------------------------------

static BOOL __send_email( char *smtpserv, struct EMail *e )
{
	struct message_data
	{
		long code;	char *str;	char *cmd;
	}
	 _md[] = 
	{
		{ 220,	LocalSMTPHost() /*e->helo*/,	"HELO "		},
		{ 250,	e->from,	"MAIL FROM: "	},
		{ 250,	e->to,		"RCPT TO: "	},
		{ 250,	"",		"DATA"		},
		{ 354,	NULL,		NULL		},
		{ 250,	"",		"QUIT"		},
		{ 221,	NULL,		NULL		},
		{   0,	NULL,		NULL		},
	};
	struct message_data *md;	BOOL send_error;
	
	long sockfd;
	
	int i;
	struct hostent *hostaddr;
	struct sockaddr_in saddr;
	BOOL got220=FALSE;
	
	DB("Entering..., %s\n", smtpserv)
	nfo("Sending EMail to %s\n", (ULONG)e->to);
	
	if(!(hostaddr = gethostbyname((STRPTR)smtpserv )))
	{
		cdeb("********** gethostbyname() FAILED (%ld)", Errno()); return FALSE;
	}
   	
   	#include "debug_hostaddr.h"
   	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cdeb("****** socket() creation FAILED",0); return FALSE;
	}
	
   	memset( &saddr, 0, sizeof(saddr));
   	
	saddr.sin_len		= sizeof(saddr);
	saddr.sin_family	= AF_INET;
	saddr.sin_port		= htons( 25 );
	
	DB("Trying Aliases.\n")
	
	for(i=0; hostaddr->h_addr_list[i]; i++)
	{
		memcpy((APTR)&saddr.sin_addr, hostaddr->h_addr_list[i], (size_t)hostaddr->h_length);
		
		if(connect( sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) != -1)
			break;
	}
	
//	cdeb("ERRNO VARS: h_errno = %ld, errno = %ld, Errno() = %ld", h_errno, errno, Errno());
	
	if(sockfd == -1)
	{
		CloseSockFD( &sockfd ); // ummh...
		cdeb("******* Connection to %s refused", (ULONG)smtpserv); return FALSE;
	}
	
	if(Errno() == 60)
	{
		cdeb("***** CONNECTION TIMEOUT", 0);
		CloseSockFD( &sockfd );	 return FALSE;
	}
	
	DB("Starting communication:\n")
	
	for( md = _md; md->code; md++)
	{
		char line[512], *ln = (char *) line, *str;
		
		long Err, max = sizeof(line) - 1, rCode;
		
		UBYTE c;
		
		DB("recveiving line...\n")
		
		do
		{
			Err = recv( sockfd, &c, 1,0);
			
			if(Err <= 0) break;
			if(c == '\n') break;
			if(c == '\r') continue;
			*ln++ = c;
		}
		while( --max );		*ln = 0;
		
		DB("GOT line with %ld bytes, which its: \n\t\"%s\"\n", sizeof(line) - max, line)
		
		if(Err <= 0)
		{
			cdeb("******* ERROR RECEIVING! (%s)", (ULONG)_enostr(Errno()));
			CloseSockFD( &sockfd );	 return FALSE;
		}
		
		if(!max)
		{
			cdeb("******* BUFFER OVERFLOW *********, got: \"%s\"", (ULONG)line);
			CloseSockFD( &sockfd );	 return FALSE;
		}
		
		rCode = _MyAtoi(line);
		
		if((rCode == 220) && got220)
		{
			md--;
			
			/** si el servidor envia lineas multiples, como un 
			 ** mensage de bienvenida largo, saltalo.
			 **/
			
			continue;
		}
		
		if(rCode != md->code)
		{
			cdeb("*** ERROR: %s", (ULONG)line);
			
			err( line );	CloseSockFD( &sockfd );
			
			if((rCode == 421) && (seRetrys < 8))
			{
				nfo("delaying to retry..."); Delay( 300 );
				seRetrys++;
				
				return __send_email( smtpserv, e);
			}
			
			if((rCode == 554) || (rCode == 550))
			{
				/* crude method to disallow retrying when 'Access Denied'.. */
				fError = -1;
			}
			
			return FALSE;
		}
		
		if(rCode == 220)
			got220 = TRUE;
		
		if(md->code == 354)
		{
			str = e->email_data->str;
			
			DB("Sending DATA, %ld bytes\n", e->email_data->len)
			
			send_error = 
				((send( sockfd, e->email_data->str, e->email_data->len, 0)
					!= e->email_data->len) ? TRUE : FALSE);
		}
		else
		{
			str = sf2("%s%s\r\n", md->cmd, md->str);
			
			DB("Sending command: \"%s\"\n", str)
			
			send_error = !_send( sockfd, str, SocketBase);
		}
		
		if(send_error)
		{
			cdeb("ERROR sending!",0);
			CloseSockFD( &sockfd );	 return FALSE;
		}
	}
	
	DB("Finishing connection\n")
	
	CloseSockFD( &sockfd );
	
	return TRUE;
}

//------------------------------------------------------------------------------

INLINE void __save_mxdns_cache( char *email_domain, struct mx_exchange *mxe )
{
	static char data[4096];	int x;
	
	sp1( data, "%s:", email_domain);
	
	for( x = 0; x <= mxe->matching_answers; x++)
	{
		if(!mxe->domain[x])
			continue;
		
		sprintf( &data[strlen(data)], mxe->domain[x]);
		
		if(mxe->domain[x+1])
			sprintf( &data[strlen(data)], ",");
	}
	
	sprintf( &data[strlen(data)], "\n");
	
	if(!_append( MXDNS_CACHE_FILE, data))
		say("ERROR writing to CACHE...");
}

//------------------------------------------------------------------------------

static void __free_mxDomains( struct mx_exchange *mxe )
{
	int x; for( x = 0; x <= mxe->matching_answers; x++)
	{
		if(mxe->domain[x])
		{
			free( mxe->domain[x] ); mxe->domain[x] = NULL;
		}
	}
}

//------------------------------------------------------------------------------

static BOOL __mx_exchange_send( char *email_domain, struct mx_exchange *mxe, struct EMail *e)
{
	int x;	BOOL done = FALSE;
	
	if(!mxe)
	{
		say ("mxe error");	return FALSE;
	}
	
	for( x = 0; x <= mxe->matching_answers && !done; x++)
	{
		if(!mxe->domain[x])
			continue;
		
		cdeb("Trying using SMTP \"%s\"", (ULONG)mxe->domain[x]);
		
		if(__send_email( mxe->domain[x], e))
		{
			cdeb("MESSAGE SENT USING ABOVE (MX)SMTP Server!", 0);
			
			done = TRUE;
		}
	}
	
	if(done)
	{
		// if not cached already...
		if(mxe->matching_answers == mxe->internal_matching_answers)
		{
			__save_mxdns_cache( email_domain, mxe );
		}
	}
	
	__free_mxDomains( mxe );
	
	return done;
}

//------------------------------------------------------------------------------

INLINE BOOL __get_mxDomain_fromCache( char *dom, struct EMail *e)
{
	char *file;
	
	if(!_exists( MXDNS_CACHE_FILE )) /* env-handler is assumed */
		return FALSE;
	
	if((file = _FileToMem( MXDNS_CACHE_FILE )))
	{
		char *f = file, *ln;
		
		while((ln = _strsep( &f, "\n")))
		{
			if(!strncmp( dom, ln, strlen(dom)))
			{
				struct mx_exchange mxe;
				char *addr;
				
				cdeb("Found CACHED MX domain(s) for \"%s\"...", (ULONG)dom);
				
				mxe.matching_answers		=  0;
				mxe.internal_matching_answers	= -1;
				
				for(_strsep( &ln, ":"); (addr = _strsep( &ln, ",")); mxe.matching_answers++)
				{
					if(!(mxe.domain[mxe.matching_answers] = strdup( addr )))
					{
						say("error parsing cache...");
						
						__free_mxDomains( &mxe );	break;
					}
				}
				
				mxe.domain[mxe.matching_answers] = NULL;
				
				if(mxe.domain[0])
				{
					if(!__mx_exchange_send( dom, &mxe, e ))
					{
						say("failed using cached MX Domain(s) for \"%s\"!", (ULONG)e->to);
						return FALSE;
					}
					return TRUE;
				}
				break;
			}
		}
	}
	else	say("error loading %s", (ULONG)MXDNS_CACHE_FILE);
	
	return FALSE;
}

//------------------------------------------------------------------------------

INLINE BOOL __get_mxDomain( char *dom, struct EMail *e)
{
	struct mx_exchange mxe;		BOOL done = FALSE;
	
	STRPTR NameServers[] = 
	{
		"198.41.0.4",	/* a.root-servers.net, NON-authoritative, DNSTool default */
		"195.30.6.140",	/* ns1.engelschall.com, authoritative */
		"66.98.244.52",	/* a.consumer.net, NON-authoritative */
		NULL
	}, **p;
	
	for(p = (STRPTR **)NameServers; (*p) && !done; p++)
	{
		memset( &mxe, 0, sizeof(struct mx_exchange));
		
		cdeb("Querying MX Domain(s) for \"%s\", NS(%s)...", (ULONG)e->to, (ULONG)((char *)(*p)));
		
		GetMXDomain( dom, &mxe, ((char *)(*p)) /*, CasorranBase*/);
		
		if(mxe.error_code != 0)
		{
			cdeb("error retreiving mx domains!, #%ld", mxe.error_code);
		}
		else
		{
			if(mxe.matching_answers != mxe.internal_matching_answers)
			{
				cdeb("Internal error, mismatch matching answers! (%ld != %ld)",
					mxe.matching_answers, mxe.internal_matching_answers);
			}
		
			cdeb("Found %ld addresses, after %ld NameServer referrals.",
				mxe.matching_answers, mxe.nameserver_referrals);
			
			if(!mxe.matching_answers)
				continue;
			
			done = __mx_exchange_send( dom, &mxe, e );
		}
	}
	
	if(!(*p) && !done && (fError != -1))
	{
		cdeb("**** WARNING: out of nameservers !!!!!!", 0);
	}
	
	return done;
}

//------------------------------------------------------------------------------

INLINE BOOL __email_transport( struct EMail *e)
{
	char recipient_domain[256], *rd = (char *) recipient_domain, *to = e->to;
	
	int rdlen = sizeof(recipient_domain) - 1;
	
	cdeb("Sending message to \"%s\"", (ULONG)e->to);
	
	while(*to && *to != '@') to++;
	
	if(!to)
	{ // this should never happend...
		cdeb("ERROR getting recipient domain!", 0);	return FALSE;
	}
	
	for(to++; *to && *to != '>' && rdlen--; *rd++ = *to++); *rd = '\0';
	
	if(!rdlen)
	{
		cdeb("****** RECIPIENT BUFFER OVERFLOW *******", 0); return FALSE;
	}
	
	if(__get_mxDomain_fromCache( recipient_domain, e))
	{
		return TRUE;
	}
	
	cdeb("Trying using SMTP \"%s\"...", (ULONG)recipient_domain);
	
	if(__send_email( recipient_domain, e))
	{
		cdeb("Message sent succesfully using email domain.", 0); return TRUE;
	}
	
	return __get_mxDomain(recipient_domain, e);
}

//------------------------------------------------------------------------------

static void free_emaildata( struct EMail *e)
{	
	fMem(e->ip); fMem(e->helo); fMem(e->from); fMem(e->to);
	
	if(e->email_data)
	{
		_string_free( e->email_data );	e->email_data = NULL;
	}
}

//------------------------------------------------------------------------------

INLINE VOID ProcessSpoolFile(STRPTR filename)
{
	char * file;
	
	if((file = _FileToMem( filename )))
	{
		char *f = file, *ln;	size_t data_length;
		struct EMail EM;
		struct email_passport
		{
			char **str;
		} _ep[] =
		{
			{ &EM.ip }, { &EM.helo }, { &EM.from }, { &EM.to }, {NULL}
		};
		struct email_passport *ep;
		
		EM.ip = NULL; EM.from = NULL; EM.to = NULL; EM.helo = NULL;
		
		// check that it is a spool file
		ln = _strsep( &f, "\n");
		
		if(!ln || *ln++ != magic_id[0] || *ln != magic_id[1])
		{
			err("*-+-*--+ \"%s\" ISN'T A SPOOL FILE", (ULONG)filename);
			cdeb("*-+-*--+ \"%s\" ISN'T A SPOOL FILE", (ULONG)filename);
			
			fMem(file); return;
		}
		
		//cdeb("line %ld point", __LINE__);
		
		for(ep = _ep; !EM.to && (ln = _strsep( &f, "\n")); ep++)
		{
			_strtrim(ln);
			
			if(!(*ep->str = strdup(ln)))
				break;
		}
		
		if(!EM.ip || !EM.helo || !EM.from || !EM.to)
		{
			cdeb("***** ERROR READING SPOOL, out of memory !?", 0);
			free_emaildata(&EM); fMem(file);
			return;
		}
		
		DB("GOT: ip = %s, helo = %s, from = %s, to = %s\n", EM.ip, EM.helo, EM.from, EM.to)
		
		//EM.email_data = _string_new (NULL, 0, 0, TRUE);
		EM.email_data = _string_new ( 2048 );
		
		if(!EM.email_data)
		{
			cdeb("FATAL EROR Allocating Memory", 0);
			free_emaildata(&EM); fMem(file);
			return;
		}
		
		data_length = ( size_t ) strlen( f );
		
		DB("Parsing DATA, %ld bytes\n", data_length)
		
		_string_appendu( EM.email_data, f, data_length);
		
		// ensure that all data is stored correctly...
		if(data_length != EM.email_data->len)
		{
			cdeb("*********** FATAL ERROR STORING MAILDATA **********");
			err("*********** FATAL ERROR STORING MAILDATA **********");
			free_emaildata(&EM); fMem(file);
			return;
		}
		_string_appendu( EM.email_data, "\r\n.\r\n", 5);
		
		if(__email_transport( &EM ))
		{
			if(!DeleteFile(filename))
			{
				cdeb("ERROR Deleting spool file \"%s\"", (ULONG)filename);
			}
		} else {
			// TODO: forward the email (?)... however thats a private mail server
			cdeb("******** FAILED SENDING MESSAGE **********", 0);
			
			err("failed sending email to \"%s\"", (ULONG)EM.to);
			
			fError++;
		}
		
		free_emaildata(&EM);
		
		fMem(file);
	}
	else if(!fError) cdeb("Error loading spool file \"%s\"", (ULONG)filename);
}

//------------------------------------------------------------------------------

INLINE VOID HandleSpoolerFiles( VOID )
{
	D_S(struct FileInfoBlock,fib);
	BPTR lock;
	
	if((lock = Lock( SPOOLDIR, SHARED_LOCK)))
	{
		if(Examine(lock,fib)!=DOSFALSE)
		{
			STATIC UBYTE filename[128];
			
			while(ExNext(lock,fib)!=DOSFALSE)
			{
				filename[0] = '\0';
				AddPart( filename, SPOOLDIR, sizeof(filename)-1);
				AddPart( filename, fib->fib_FileName, sizeof(filename)-1);
				
				DB("Loading \"%s\"\n", filename)
				
				ProcessSpoolFile( filename );
			}
		}
		UnLock(lock);
	}
}

//------------------------------------------------------------------------------

INLINE VOID SpoolerHandler(struct Globals *g)
{
	if(OpenThreadLibs())
	{
		fError   = 0;
		seRetrys = 0;
		log_fh   = 0;
		nfo("spooler running.");
		
		if((log_fh = Open( SMTP_LOGGER_FILENAME, MODE_READWRITE )))
		{
			static char start_log[256];
			
			snp1( start_log, sizeof(start_log) -1,
			"\n\r\n\n********** SPOOLER STARTED On %s\t--------\n", _Fecha("%A, %e %B %Y %X"));
			
			Seek( log_fh, 0, OFFSET_END);
			
			Write( log_fh, start_log, strlen(start_log));
		}
		else say("************** COULDN'T WRITE TO %s !!", (ULONG)SMTP_LOGGER_FILENAME);
		
restart:
		HandleSpoolerFiles ( ) ;
		
		if( fError )
		{
			if(fError > 6)
			{
				err("too many fatal errors...");
			}
			else if(fError == -1)
			{
				err("exiting (with no retrys) due previous errors.");
			}
			else
			{
				goto restart;
			}
		}
		else if(got_new_mail)
		{
			FORBID( got_new_mail, FALSE);
			
			goto restart;
		}
		
		if(log_fh)
		{
			logger("********** SPOOLER STOPPED\t----------------", NULL);
			
			Close(log_fh);
			log_fh = 0;
		}
		
		nfo("spooler stoped.");
		
		CloseThreadLibs();
	}
}

//------------------------------------------------------------------------------

THREAD(spooler)
{
	thread t;
	
	if((t = thr_init()))
	{
		FORBID( spooler_task, t->task);
		
		SpoolerHandler((struct Globals *) t->data );
		
		FORBID( spooler_task, NULL);
		
		thr_exit(t, 0);
	}
}

