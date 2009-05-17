#ifndef DNSTOOL_MX_EXCHANGE_H
#define DNSTOOL_MX_EXCHANGE_H

#define DNSTOOL_DEBUG				0
#define DNSTOOL_ERRSTR				0

#define DNSTool_MSGSIZE				10000
#define DNSTool_NServer				"198.41.0.4"
#define DNSTool_Port				53

struct mx_exchange
{
	STRPTR domain[9];
	
	int internal_matching_answers; // used as counter for domain[]
	
	int matching_answers;
	int nameserver_referrals;
	
#if DNSTOOL_ERRSTR
	BOOL error;
	char *error_string;
#else
	int error_code;		// use it just to make my code smaller
#endif
};

/** ERROR CODES:
 **
 ** 10:	NULL address
 ** 20: NULL mxe_dest
 ** 30: NULL SocketBase
 ** 40: <reserved>
 ** 50: Invalid Name Server
 ** 60: couldn't create socket
 ** 70: connect() failed
 ** 80: send() failed
 ** 90: recv() failed
 ** 99: ...
 **/

GLOBAL VOID 
GetMXDomain( char *address, struct mx_exchange *mxe_dest, char *NameServer);


#if DNSTOOL_DEBUG
# define KP( args... )	kprintf( args )
#else
# define KP( args...)	((void)0)
#endif

#endif /* DNSTOOL_MX_EXCHANGE_H */
