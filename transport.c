/* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2009 Diego Casorran
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
#include "transport.h"
#include "config.h"

BOOL write_spool(struct EMail *e);
BOOL write_spool_transport(struct EMail *e, STRPTR *arry);
BOOL pop3_check_user( STRPTR user, STRPTR pass);
LONG pop3_check_mailbox( void );

//#define SEND(txt)	send( sockfd, (txt), strlen((txt)), 0)

#define SEND(code, txt)	sendResponse( sockfd, code, (ULONG)txt)

#define OK_CODE(code)	((code < 300))

extern struct Task *spooler_task;
extern BOOL got_new_mail;

STATIC BOOL smtpd_connection = FALSE;

static BOOL sendResponse(long sockfd, int code, ULONG str)
{
	static char ToSend[1024];	long len;
	
	char *fmt = (smtpd_connection ? ("%ld %s\r\n"):("%s %s\r\n"));
	
	ULONG CoDe = (smtpd_connection ? (ULONG)code:
		(OK_CODE(code) ? (ULONG)"+OK":(ULONG)"-ERR"));
	
	_snprintf( ToSend, sizeof(ToSend)-1, fmt, CoDe, str);
	
	len = strlen(ToSend);		deb( ToSend );
	
	return
		((send( sockfd, ToSend, len, 0) == len) ? TRUE : FALSE);
}



static char *recvLn( long sockfd, BOOL INCLUDE_CRLF )
{
	static char buffer[RECVLN_BUFLEN];
	register char *b = (char *) buffer;
	
	UBYTE recv_char;
	
	fd_set readfds;
	
	//struct timeval timeout = { {RECVLN_TIMEOUT}, {0} };
	struct timeval timeout = { RECVLN_TIMEOUT, 0 };
	
	long select_val, rErr, maxlen = sizeof( buffer )-1;
	
	FD_ZERO( (APTR)&readfds );
	FD_SET(sockfd, &readfds );
	
	
	if((select_val = WaitSelect( sockfd + 1, &readfds, NULL,NULL, &timeout, NULL)) < 0)
	{
		SEND( 221, "Internal error, please warn the system admin!");
		
		say("SELECT ERROR: %s", (ULONG)l_enostr());	return NULL;
	}
	
	if(!select_val)
	{
		SEND( 550 /*221*/, "RECV TIMED-OUT!."); return NULL;
	}
	
	do {
		rErr = recv( sockfd, &recv_char, 1, 0);
		
		if(rErr <= 0) break;
		
		if(!INCLUDE_CRLF)
		{
			if(recv_char == '\r') continue;
			if(recv_char == '\n') break;
		}
		
		*b++ = recv_char;
		
		if(recv_char == '\n') break;
		
	} while( --maxlen );	*b = '\0';
	
	if(rErr <= 0)
	{
		say("ERROR RECEIVING (%ld)", (ULONG)errno); return NULL;
	}
	
	if(!maxlen)
	{
		say("****** RECVLN BUFFER OVERFLOW ******\"%s\"********", (long) buffer);
		
		SEND( 500, "Line too long");	return NULL;
	}
	
	deb( buffer );
	
	return (char *) buffer;
}


INLINE char *email_user( char *email )
{
	char user[USER_LENGTH+DOMAIN_LENGTH+4], *u = user;
	
	if(!email)
		return NULL;
	
	while(*email == ' ' || *email == '<') email++;
	
	for(; *email && *email != '@'; *u++ = *email++); *u = 0;
	
	return strdup( user );
}



static BOOL too_many = FALSE;
static STRPTR crossposting_addresses[999];
static int crossposting_addresses_counter = 0;

static void crossposting_free( void )
{
	int x;
	
	for(x=0; x <= crossposting_addresses_counter; x++) {
		fMem(crossposting_addresses[x]);
	}
	
	crossposting_addresses_counter = 0;
}

static void free_pop3data( void )
{
	int x;
	
	fMem(P.user); fMem(P.pass);
	
	for(x=0; x <= P.queue; x++)
	{
		fMem(P.mbox.files[x]);
	}
	
	bzero((APTR)&P, sizeof(struct pop3));
}

static void RSETdata( struct EMail *em, BOOL purge )
{
    if( em == NULL )
    	return;
    
    fMem(em->helo); fMem(em->from); fMem(em->to); fMem(em->spool);
    
    if(em->email_data)
    {
    	_string_free(em->email_data);	em->email_data = (String *) NULL;
    }
    
    crossposting_free();	free_pop3data();
    
    em->xtra_header = NULL;
    fMem( em->relay_user );
    
    if( purge ) {
    	
    	fMem(em->ip); fMem(em->host);	Free( em );
    	
    } else {
    	//em->email_data = _string_new (NULL, 0, 0, TRUE);
    	em->email_data = _string_new ( 4096 );
    }
}


#define MSG_MEMORY_ERROR	"Internal error 12" //"Internal error allocating memory"


/** TODO: launch this from a thread to accept multiple connections (?)
 **/
long process_incoming_connection(long sock)
{
    struct sockaddr_in sa;
    struct hostent *he;
    long sockfd, len, rc=-1;
    BOOL request_succed = FALSE;
    register struct EMail *em = NULL;
    UBYTE *lastHelo = NULL;
    
    deb("procesing incoming conection...");
    
    len = sizeof (sa);
    sockfd = accept ( sock, (struct sockaddr *) &sa, &len);
    
    if(sockfd < 0)
    {
    	say("Accept error: %s", (ULONG)l_enostr());
    	return rc;
    }
    
    //if(!((em = AllocVec(sizeof(struct EMail), MEMF_PUBLIC|MEMF_CLEAR)) 
	//&& (em->email_data = _string_new (NULL, 0, 0, TRUE))))
    if(!((em = Malloc(sizeof(struct EMail))) 
	&& (em->email_data = _string_new ( 4096 ))))
    {
    	SEND(550, MSG_MEMORY_ERROR);
    	
    	goto exit;
    }
    
    Forbid();
    if(too_many)
    {
    	Permit();
    	SEND( 421, "Too many concurrent connections; please try again later.");
    	
    	goto exit;
    }
    too_many = TRUE;
    Permit();
    
    smtpd_connection = (sock == SMTP_SOCK);
    
    crossposting_addresses[0]=NULL;
    crossposting_addresses_counter = 0;
    
    em->ip = strdup((char *) Inet_NtoA(sa.sin_addr.s_addr));
    
    if(!(he=gethostbyname( em->ip )) || !(he=gethostbyaddr(he->h_addr, he->h_length, AF_INET)))
    	he = NULL;
    
    em->host = strdup((char *) (he ? he->h_name:"unknown host"));
    
    say("%s connection from %s", 
    		(ULONG)(smtpd_connection ? "SMTP":"POP3"), (ULONG)em->host);
    
    //str = sf1("220 Welcome %s\r\n", em->host);	SEND(str);
#ifndef RELEASE
    SEND(220, "Welcome to our world...");
#else
    SEND(220, _PROGRAM_NAME " " _PROGRAM_VERZ " " _PROGRAM_DATE " " _PROGRAM_COPY );
#endif
    
    do
    {
      char *line;
      
      if(!(em->email_data))
      {
      	SEND(550, MSG_MEMORY_ERROR);
      	goto exit;
      }
      
      if((line = recvLn( sockfd, FALSE )) == NULL)
      {
      	goto exit;
      }
      
      if(!(*line))
      {
      	SEND(500, "NULL Command");
      }
      else
      
      /*** ****	P O P 3   C O M M A N D S	**** ***/
      
      if(!strnicmp( line, "USER", 4))
      {
      	P.user = strdup( &line[5] );
      	
      	SEND( 250, "let me know your password");
      }
      else
      if(!strnicmp( line, "PASS", 4))
      {
      	if(!P.user) {
      		SEND( 500, "Need a USERname first..."); continue;
      	}
      	
      	P.pass = strdup( &line[5] );
      	
      	if(!pop3_check_user( P.user, P.pass)) {
      		SEND( 500, "Invalid user and/or password"); continue;
      	}
      	
      	if(pop3_check_mailbox() < 0) {
      		SEND( 500, "Couldn't lock your mailbox!"); continue;
      	}
      	
      	SEND( 250, "everything nice with you :]");
      }
      else
      if(!strnicmp( line, "TOP", 3))
      {
      	char *email;
      	
      	long msg_number = _MyAtoi( &line[5] )-1;
      	
      	if(msg_number == -1) {
      		SEND( 500, "No message number specified");
      	}
      	if(!P.mbox.files[msg_number]) {
      		SEND( 500, "Invalid message number");
      	}
      	else
      	if((email = _FileToMem( P.mbox.files[msg_number])))
      	{
      		char *f=email, *head; int lines;
      		
      		head = _strsep( &f, "\r\n\r\n");
      		
      		SEND( 250, " ");
      		
      		send( sockfd, head, strlen(head), 0); // HEY, no check!...
      		send( sockfd, "\r\n\r\n", 4, 0);
      		
      		for(line += 5; *line != ' '; line++); lines = _MyAtoi(line);
      		
      		while(lines-- && (head = _strsep( &f, "\r\n")))
      		{
      			send( sockfd, head, strlen(head), 0);
      			send( sockfd, "\r\n", 2, 0 );
      		}
      		
      		send( sockfd, ".\r\n", 3,0);
      		
      		fMem( email );
      	}
      	else SEND( 500, "Internal error loading email.");
      }
      else
      if(!strnicmp( line, "STAT", 4))
      {
      	static char str[64];
      	
      	sprintf( str, "%ld %ld", P.queue, P.size);
      	
      	SEND( 250, str);
      }
      else
      if(!strnicmp( line, "RETR", 4))
      {
      	static char r_ln[128], *email;
      	
      	long msg_number = _MyAtoi( &line[5] )-1;
      	
      	if(msg_number == -1) {
      		SEND( 500, "No message number specified");
      	}
      	if(!P.mbox.files[msg_number]) {
      		SEND( 500, "Invalid message number");
      	}
      	else
      	if((email = _FileToMem( P.mbox.files[msg_number])))
      	{
      		sprintf( r_ln, "%ld octets", P.mbox.sizes[msg_number]);
      		
      		SEND( 250, r_ln);
      		
      		if(send( sockfd, email, P.mbox.sizes[msg_number], 0) != P.mbox.sizes[msg_number])
      		{
      			say("ERROR sending %s (%ld)", (ULONG)P.mbox.files[msg_number], P.mbox.sizes[msg_number]);
      		}
      		else
      			send( sockfd, "\r\n.\r\n", 5, 0);
      		
      		fMem( email );
      	}
      	else SEND( 500, "Internal error loading email.");
      }
      else
      if(!strnicmp( line, "DELE", 4))
      {
      	long msg_number = _MyAtoi( &line[5] )-1;
      	
      	if(msg_number == -1) {
      		SEND( 500, "No message number specified");
      	}
      	else
      	if(!P.mbox.files[msg_number]) {
      		SEND( 500, "Invalid message number");
      	}
      	else
      	if(P.mbox.deleted[msg_number]) {
      		SEND( 500, "message already deleted");
      	}
      	else
      	if(!DeleteFile( P.mbox.files[msg_number] ))
      		SEND( 500, "Internal error deleting email");
      	else	{
      		SEND( 250, "message deleted");
      		P.mbox.deleted[msg_number] = TRUE;
      	}
      }
      else
      if(!strnicmp( line, "LIST", 4) || !strnicmp( line, "UIDL", 4))
      {
      	char nln[128];
      	//long msg_number = *((char *)&line[4]) != 0x20 ? -1:_MyAtoi( &line[5] );
      	long msg_number = _MyAtoi( &line[5] )-1;
      	
      	BOOL uidl = (line[0] == 'U');
      	deb(">>> %s for #%ld",(ULONG)(uidl?(char *)"UIDL":(char *)"LIST"), msg_number);
      	
      	if(msg_number <= 0) /* list all messges */
      	{
      		long x;		nln[0] = 0x20; nln[1] = '\0';
      		
      		if(!uidl)
      			sprintf( nln, "%ld messages (%ld octets)", P.queue, P.size);
      		
      		SEND( 250, nln);
      		
      		for(x=0; x < P.queue; x++) if(uidl || !P.mbox.deleted[x]) {
      			
      			sprintf( nln, "%ld %lu\r\n", x+1,
      				(uidl ? (ULONG)P.mbox.uidl[x] : P.mbox.sizes[x]));
      			
      			send( sockfd, nln, strlen(nln), 0);
      			
      			deb( nln );
      		}
      		
      		send( sockfd, ".\r\n", 3, 0);	deb(".\r\n");
      	}
      	else
      	{
      		if(!P.mbox.files[msg_number]) {
      			SEND( 500, "Invalid message number");
      		}
      		else {
      			sprintf( nln, "%ld %lu", msg_number+1,
      				(uidl ? P.mbox.uidl[msg_number] : P.mbox.sizes[msg_number]));
      			
      			SEND( 250, nln);
      		}
      	}
      }
      else
      
      
      
      
      /*** ****	S M T P D   C O M M A N D S	**** ***/
      
      
      if(!strnicmp( line, "HELO", 4))
      {
      	if(!strchr( line, '.'))
      	{
      		SEND( 550, "Don't like your HELO/EHLO. Hostname must contain a dot."); continue;
      	}
      	SEND(250, "Hello man, procced...");
      	
	em->helo = strdup( &line[5] );	lastHelo = strdup( em->helo );
      }
      else
      if(!strnicmp( line, "MAIL FROM:", 10))
      {
      	if(!em->helo)
      	{
      		if(lastHelo)
      			em->helo = strdup( lastHelo );
      		else {
      			SEND(500, "Use HELO first.");	continue;
      		}
      	}
      	
      	line += 10; while(*line && *line == ' ') line++;
      	
      	if(*line == '<' && *(line+1) == '>')
      	{
      		/* http://www.rfc-ignorant.org/policy-dsn.php */
      		
      		em->from = strdup("mailer-daemon");
      		SEND(250, "OK");
      		
      		say(" *** %s ATTEMPTING TO SEND A BOUNCE ***\n", (ULONG) em->ip );
      		continue;
      	}
      	
      	if(!strchr( line, '@'))
      	{
      		SEND(550, "Invalid address!");	continue;
      	}
      	SEND(250, "OK");
      	
      /*_strtrim( (line += 10) ); */	em->from = strdup( line );
      	
      	deb("%s is %s", (ULONG)em->ip, (ULONG)em->from);
      }
      else
      if(!strnicmp( line, "RCPT TO:", 8))
      {
      	if(!em->from)
      	{
      		SEND(503, "Use MAIL first.");	continue;
      	}
#if 0
      	if(em->to)
      	{
      		SEND(550, "SORRY, crossposting NOT allowed!");	continue;
      	}
#endif
      	if(!strchr( line, '@'))
      	{
      		SEND(550, "Invalid address!");	continue;
      	}
      	
      	em->relay = FALSE;
      	if(!_CheckMatch( em->ip, RELAY_FROM_HOSTS))
      	{
      		if(!_CheckMatch( line, RELAY_FOR_USERS))
      		{
      			say("denyed RELAY from %s,%s TO %s", (ULONG)em->host, (ULONG)em->from, (ULONG)&line[8]);
      			
      			SEND( 550, "Relay not allowed."); break;
      		}
      		em->relay = TRUE;
      		
      		if(!(em->relay_user = email_user( &line[8] ))) {
      			SEND(550, MSG_MEMORY_ERROR);
      			goto exit;
      		}
      		
      		deb("******* %s RELAYING TO %s", (ULONG)em->ip, (ULONG)em->relay_user);
      	}
      	
#if __FORWARD__
#warning forwarding mode activated...
      	if(em->relay)
      	{
      		SEND( 250, "OK - Forwarding message to " SYSADMIN_MBOX "...");
      		
      		line = SYSADMIN_EMAIL;
      	}
      	else
#endif
      	{
      		SEND(250, "OK");
      		
      		_strtrim( (line += 8) );
      	}
      	
      	if(!em->to) {
      		if(!(em->to = strdup( line ))) {
      			SEND(550, MSG_MEMORY_ERROR);
      			goto exit;
      		}
      	} else {
      		deb("cross-posting, to %s", (long) line );
      		
      		if(!(crossposting_addresses[crossposting_addresses_counter++] = strdup( line )))
      		{	SEND(550, MSG_MEMORY_ERROR);	goto exit;	}
      	}
      	
      	deb("%s emailing to %s", (ULONG)em->ip, (ULONG)line);
      }
      else
      if(!strnicmp( line, "DATA", 4))
      {
//      	unsigned char oic[6] = {0};
      	
      	if(!em->to)
      	{
      		SEND(500, "Use RCPT first.");	continue;
      	}
      	SEND(354, "Enter message, ending with \".\" on a line by itself");
      	
      	while((line = recvLn( sockfd, TRUE )))
      	{
      		//if(!strcmp( line, ".\r\n"))
      		if(*line == '.' && *(line+1) == '\r' && *(line+2) == '\n' && !*(line+3))
      			break;
      		
      		_string_append( em->email_data, line);
      	}
      	
      	if(line == NULL)
      	{
      		SEND(550, "ERROR RECEIVING DATA");
      	}
      	else if(crossposting_addresses[0] && (em->email_data->len > 480000))
      	{
      		SEND(500, "CROSS POSTING NOT ALLOWED FOR TOO LONG EMAILS !..."); /* lala.. */
      	}
      	else
      	{
      		STATIC UBYTE tempSpool[1024];
      		BOOL done;
      		char *user = em->relay_user;
      		
#if !__FORWARD__
		if(em->relay)
		{
			BPTR lock;
			
			//snp2(tempSpool, sizeof(tempSpool)-1, "%s%s", MAILBOX, user);
			*tempSpool = 0;
			AddPart( tempSpool, MAILBOX, sizeof(tempSpool)-1);
			AddPart( tempSpool, user, sizeof(tempSpool)-1);
			
			deb("Checking MBOX \"%s\"", (long) tempSpool);
			
			if((lock = Lock( tempSpool, SHARED_LOCK))) {
				UnLock( lock );
			}
			else {
				say("MAILBOX for \"%s\" doenst exists, creating it...", (ULONG)user);
				
				if((lock = CreateDir( tempSpool ))) {
					UnLock( lock );
				}
				else {
					say("FATAL ERROR CREATING NEW MAILBOX FOR \"%s\", IoErr(%ld)", (ULONG)user, IoErr());
					say("USING SYSADMIN MAILBOX (%s)", (ULONG)SYSADMIN_MBOX);
					
					user = SYSADMIN_MBOX;
				}
			}
		}
#endif
      		do {
#if !__FORWARD__
	if(em->relay)
      			snp4( tempSpool, sizeof(tempSpool)-1, 
      				"%s%s/%ld.%ld", MAILBOX, user, _elapsed_time(0), _uniqueId(0));
	else
#endif
      			snp3( tempSpool, sizeof(tempSpool)-1, "%s%ld.%ld", SPOOLDIR, _elapsed_time(0), _uniqueId(0));
      		
      		} while(_exists( tempSpool ));
      		
      		em->spool = strdup( tempSpool );
      		
      		deb("Using spool %s", (long) tempSpool);
      		
      		crossposting_addresses[crossposting_addresses_counter] = NULL;
      		
      		if(crossposting_addresses[0])
      			done = write_spool_transport( em, crossposting_addresses);
      		else
      			done = write_spool( em );
      		
		if(done)
		{
			if(!em->relay)
			{
				FORBIDIF( spooler_task, got_new_mail, TRUE);
			}
      			SEND(250, "Message Accepted.");
      			
      			request_succed = TRUE;
      		}
      		else	{
      			SEND(550, "Error writing Spool...");
      			say( "********** ERROR WRITING SPOOL *******\"%s\"******", (long)tempSpool );
      		}
      	}
      	RSETdata( em, FALSE );
      }
      else
      if(!strnicmp( line, "QUIT", 4))
      {
      	SEND(221, "Closing connection..."); break;
      }
      else
      if(!strnicmp( line, "NOOP", 4))
      {
      	SEND( 250, "OK");
      }
      else
      if(!strnicmp( line, "RSET", 4))
      {
      	RSETdata( em, FALSE );	SEND( 250, "OK");
      }
      else
      if(!strnicmp( line, "HELP", 4))
      {
      	SEND( 250, "This is a private server...");
      }
      else
      if(!strnicmp( line, "RSPOOL", 6))
      {
      	request_succed = TRUE;	break;
      }
      else
      if(!strnicmp( line, "VRFY", 4))
      {
      	say( "VRFY request from %s for %s", (ULONG) em->ip, (ULONG) &line[5]);
      	
      	if(_CheckMatch( line, RELAY_FOR_USERS))
      	{
      		SEND( 250, "OK, we know that user...");
      	}
      	else	SEND( 550, "dunno whois that user.");
      }
      else
      if(!strnicmp( line, "EXPN", 4))
      {
      	SEND( 550, "we do not manage mailing-list.");
      }
/*      else
      if(!strnicmp( line, "", ))
      {
      }
  */    else SEND( 500, "Syntax error, command unrecognized.");
    }
    while(1);
    
    rc = 1;
    
exit:
    CloseSocket( sockfd );	sockfd = -1;
    
    say("connection from %s closed%s.", (ULONG)(em ? em->host:""), (long)
    	((rc == 1) ? "":(request_succed ? " due errors after accepted email":" due errors")));
    
    Forbid();
    too_many = FALSE;
    Permit();
    
    if(request_succed && !em->relay)
    {
    	deb("Running spooler...");
    	
    	FORBIDCALL( !spooler_task, SPOOLER_TASK );
    }
    
    RSETdata( em, TRUE );
    
    return rc;
}
