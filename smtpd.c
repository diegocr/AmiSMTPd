
/**
 * SMTPD - Simple Mail Transfer Protocol (AND POP3) Daemon.
 * (c)2005 Diego Casorran - All right reserved.
 * 
 * 
 * ChangeLog:
 * 	0.1, 20050629: First Version
 * 	0.2, 20060415: Added support for "MAIL FROM:<>"
 * 
**/

#include "globals.h"
#include "config.h"

GLOBAL LONG process_incoming_connection(LONG sock);

STATIC LONG BindPRO(LONG port, LONG EventMask, LONG IoctlAttr, struct Library *SocketBase)
{
	long sockfd;
	
	if ((sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_IP)) != -1)
	{
		struct sockaddr_in server;
		int len;
		
		memset (&server, 0, sizeof (server));
		server.sin_family      = AF_INET;
		server.sin_addr.s_addr = htonl (INADDR_ANY);
		server.sin_port        = htons (port);
		
		/* set something non-zero */
		len = sizeof (server);
		
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &len, sizeof(len));
		
		if(bind(sockfd, (struct sockaddr *)&server, sizeof (struct sockaddr)) != -1)
		{
			listen(sockfd, 5);
			
			if(setsockopt( sockfd, SOL_SOCKET, SO_EVENTMASK, &EventMask, sizeof(LONG)) != -1)
			{
				long int u = 1;
				if(IoctlSocket ( sockfd, IoctlAttr, (char*)&u) != -1)
				{
					// everything OK!
					
					return sockfd;
				}
			}
		}
		
		CloseSocket(sockfd);
		sockfd = -1;
	}
	
	return sockfd;
}


int smtpd(void)
{
	long sattr;
	ULONG WaitSig = 0, NetSigMask = 0;
	
	nfo("");
	nfo("Started");
	
	NetSigMask = (1L << G.NetSig);
	
	sattr = SocketBaseTags( SBTM_SETVAL(SBTC_SIGEVENTMASK), NetSigMask, SBTM_SETVAL(SBTC_DTABLESIZE), 4096, TAG_DONE);
	
	if( sattr != 0 )
	{
		err("SocketBaseTags() error #%ld\n", sattr );
		return 99;
	}
	
	deb("Binding port(s)...");
	
	if((SMTP_SOCK = BindPRO( G.port, FD_ACCEPT | FD_ERROR, FIOASYNC, SocketBase)) == -1)
	{
		err("Bind error for port %ld: %s", G.port, (ULONG)l_enostr());
		return 99;
	}
	if((POP3_SOCK = BindPRO( 110, FD_ACCEPT | FD_ERROR, FIOASYNC, SocketBase)) == -1)
	{
		err("Bind error for port %ld: %s", 110, (ULONG)l_enostr());
#if 0
		return 99;
#else
		/* reeived emails will be processed correctly, ofcourse */
		nfo("-=- POP3 Service unavailable");
#endif
	}
	
	nfo("Listening on port %ld.", (ULONG)G.port);
	
	if(InitConfig()==FALSE)
	{
		nfo("cannot load config file, error %ld", IoErr());
		return 99;
	}
	
	#define _SIGNALS	NetSigMask | th_sigmask | SIGBREAKF_CTRL_C
	
	deb("Waiting for Signals %08lx", (long)_SIGNALS);
	
	do {
		WaitSig = Wait(WaitSig | _SIGNALS );
		
		if(WaitSig & SIGBREAKF_CTRL_C)
		{
			deb("Interrupt Signal Received");	break;
		}
		if(WaitSig & th_sigmask)
		{
			deb("Geting MsgPorts's");	th_poll();
		}
		else if(WaitSig & NetSigMask)
		{
			long mysock; ULONG emask;
			
			deb("Got NetSignal...");
			
			while((mysock = GetSocketEvents((ULONG *)&emask)) != -1)
			{
				if(emask & FD_ACCEPT)
				{
					if(process_incoming_connection(mysock) == -1)
					{
						err("Error processing incoming connection.");
					}
				}
				if(emask & FD_ERROR)
				{
					err("NetError On %s Service: %s", 
						(ULONG)((mysock == SMTP_SOCK) ? "SMTPD":"POP3"), (ULONG)strerror(errno));
				}
			}
		}
	} while(1);
	
	ClearConfig();
	
	return 0;
}

// to be avalable (without conflics) between main task and threads
void _vsyslog( long level, const char *fmt, LONG *args)
{
	vsyslog( level, fmt, args);
}
char *l_enostr(void)
{
#ifdef _enostr
	char *res;
	if(!(res = strchr(_enostr(errno), ' ')))
		res = strchr(_enostr(h_errno), ' ');
	
	return res ? res+1 : "'errno undefined !?'";
#else
	return strerror(errno);
#endif
}

__inline STRPTR LocalSMTPHost( void )
{
//	return (STRPTR) "externalmx-1.sourceforge.net";
//	return (STRPTR) "amiga.sf.net" ;
	return (STRPTR) config->ServerVirtualHostname;
}

