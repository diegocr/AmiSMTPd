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
#include "spool_write.h"

#define FAKE_DAY	0

#define HEAD		&header[strlen(header)]
#define HACK(id)	id //(!strncmp( e->ip, "127.", 4) ? id : 1)
//#define FAKE_IP		"66.35.250.206" // users.sf.net
#define FAKE_IP		"82.98.128.62" // casorran.com


#if FAKE_DAY
STATIC CONST STRPTR dia[] =
	{ "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

STATIC CONST STRPTR mes[] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
#endif

INLINE char *fmt_date( void )		/* Fri,  5 Nov 2004 00:00:00*/
{
	static char date[64];
	
#if FAKE_DAY
	long h = (long) _MyAtoi(_Fecha("%q")), x, d, w, m;
	
	d = (long) _MyAtoi(_Fecha("%d"));
	w = (long) _MyAtoi(_Fecha("%w"));
	m = (long) _MyAtoi(_Fecha("%m"));
	
	// FIXME, when they reached 0...
	for(x = 9; x; x--, h--) 
	if(!h) { h = 24; if(!--d) d = 30; if(!--w) w = 7; }
	
	sp5( date, "%s, %2ld %s %s %02ld:", dia[w-1], d, mes[m-1], _Fecha("%Y"), h);
	
	sp1( &date[strlen(date)], "%s -0800 (PST)", _Fecha("%M:%S"));
#else
	_snprintf( date, sizeof(date)-1,_Fecha( "%a, %e %b %Y %X 0000 (GMT)" ));
#endif
	
	return (char *) date;
}

/****************************************************************************/

struct __fake_addy {
	
	STRPTR dest_addy;
	STRPTR fake_addy;
	
	struct __fake_addy * next;
};
STATIC struct __fake_addy * faddy = NULL;

INLINE void new_faddy( STRPTR dest, STRPTR fake )
{
	struct __fake_addy * new;
	
	if((new = gMem(sizeof(struct __fake_addy))))
	{
		new->dest_addy = strdup( dest );
		new->fake_addy = strdup( fake );
		
		if(new->dest_addy && new->fake_addy)
		{
			say("added faddy \"%s\" -> \"%s\"", (ULONG) new->dest_addy, (ULONG)new->fake_addy );
			
			new->next = faddy;
			faddy = new;
		}
		else {
			fMem(new);
		}
	}
	
	if(new == NULL) {
		
		say(" ** out of memory for faddy ** ");
	}
	
	#warning this isnt freed at exit
}

INLINE VOID LoadFAddy( void )
{
	char * file;
	
	if((file = _FileToMem(FADDY_FILE)))
	{
		char *f=file, * line;
		
		while((line = _strsep( &f, "\n")))
		{
			char * left;
			
			if(!*line || *line == ';')
				continue;
			
			left = _strsep( &line, ":");
			
			if(!(left && *left && *(left+1)) || !(line && *line && *(line+1)))
			{
#ifndef RELEASE
				say(" ** PARSER ERROR READING FADDY FILE **");
#endif
				continue;
			}
			
			new_faddy( left, line );
		}
		
		fMem( file );
	}
}


STATIC STRPTR FakeAddy( struct EMail *e)
{
	STATIC BOOL file_loaded = FALSE;
	STATIC UBYTE addy[256];
	struct __fake_addy * ptr;
	
	if(file_loaded == FALSE)
	{
		LoadFAddy ( ) ;
		
		file_loaded = TRUE;
	}
	
	for( ptr = faddy ; ptr ; ptr = ptr->next )
	{
		if(_CheckMatch( e->to, ptr->dest_addy ))
			break;
	}
	
	if(ptr != NULL)
	{
		snp3( addy, sizeof(addy), "%s%s%s\n",
			(*ptr->fake_addy == '<' ? "" : "<"),
			ptr->fake_addy, (*ptr->fake_addy == '<' ? "" : ">"));
	}
	else
	{
		snp3( addy, sizeof(addy), "%s%s%s\n",
			(*e->from == '<' ? "" : "<"), 
			e->from, (*e->from == '<' ? "" : ">"));
	}
	
	return (STRPTR) addy;
}

/****************************************************************************/

BOOL write_spool(struct EMail *e)
{
	char header[4096];	BOOL done;
	struct DateStamp ds;
	
	// check that all is correct
	if(!e->spool || !e->ip || !e->helo || !e->from || !e->to 
		|| !e->email_data || !e->email_data->str)
		{
			say("possible error allocating memory...");
			return FALSE;
	}
	
	DateStamp(&ds);		*header = 0;
	
//	deb("Writing spool \"%s\", relay = %ld", (long) e->spool, (long) e->relay);
	
//#if !__FORWARD__
	if(e->relay == FALSE) {
//#endif
	
	// first line identify itself as a mail file
	sp3( header, "%lc%lc MAIL SPOOL, timestamp = %ld\n",
		magic_id[0], HACK(magic_id[1]), _elapsed_time(0));
	
	// write Sender IP  -  TODO: resolve it?
	sp1( HEAD, "%s\n", e->ip);
	
	// we assume on the next that there can't be buffer overflows...
	sp1( HEAD, "%s\n", e->helo);
	
#if 0
	sp3( HEAD, "%s%s%s\n", 
		(*e->from == '<' ? "" : "<"), e->from, (*e->from == '<' ? "" : "<"));
#else
	_sprintf( HEAD, FakeAddy( e ));
#endif
	
	sp3( HEAD, "%s%s%s\n", 
		(*e->to == '<' ? "" : "<"), e->to, (*e->to == '<' ? "" : "<"));
	
//#if !__FORWARD__
	} else {
//		deb("RELAYING, not adding header");
	}
//#endif
	
	// according to RFC I think FWS == '\t', but YAM dislike it (?)..
	#undef FWS
	#define FWS " "
	#undef CFWS
	#define CFWS "\r\n\t"
	
	sp8( HEAD,
		"Received:" FWS 
		RCVD_FROM FWS "%s [%s (helo=%s)]" CFWS
		RCVD_BY   FWS "%s" FWS RCVD_WITH FWS "esmtp" FWS /*CFWS*/
		RCVD_ID   FWS "%ld-%ld-MC68060" CFWS
		RCVD_FOR  FWS "%s" RCVD_SC FWS "%s" CRLF
#ifndef RELEASE
		"X-Spam-Score: 0.0 (/)\r\n" /* ... :-] */
		, (!strncmp( e->ip, "127.", 4) ? FAKE_IP : e->host)
		, (!strncmp( e->ip, "127.", 4) ? "0.0.0.0" : e->ip)
#else
		, e->host, e->ip
#endif
		, e->helo
		, LocalSMTPHost()
		, ((ds.ds_Days * 86400) + (ds.ds_Minute * 60) + (ds.ds_Tick / TICKS_PER_SECOND))
		, _uniqueId(0)
		, e->to
		, fmt_date());
	
	if(e->xtra_header)
		_sprintf( HEAD, e->xtra_header);
	
	// write final spool file
	if((done = _WriteFile( e->spool, header)))
	{
		if(!(done = (BOOL)_append( e->spool, e->email_data->str)))
			DeleteFile( e->spool );
	}
	
	return done;
}


BOOL write_spool_transport(struct EMail *e, STRPTR *arry)
{
//	char **arry = (*cpa);
	BOOL done;
	int cnt=0;
	
	do {
		STATIC UBYTE tempSpool[128];
		
		if(!(done = write_spool( e ))) {
			nfo("WRITE_SPOOL FAILED ON CROSSPOSTING, SOME EMAIL COULD BE SENT TWICE!");
			
			DeleteFile( e->spool );	break;
		}
		
		fMem(e->to); e->to = *arry;
		
		snp2( tempSpool, sizeof(tempSpool)-1, "%s,%ld", e->spool, ++cnt);
		
		fMem(e->spool);
		
		if(!(e->spool = strdup(tempSpool))) {
			nfo("STRDUP FAILED ON CROSS POSTING, SOME EMAIL COULD BE SENT TWICE!");
			return FALSE;
		}
		
	} while( *arry++ && done );
	
	return done;
}

