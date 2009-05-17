
#include "globals.h"

/*#define _STAT_H 1
#include <bsdsocket.h>
#include <netdb.h>
*/


#include "mx_exchange.h"
//#include <proto/intuition.h> // DisplayBeep() for test, remove me!

#define inet_ntoa(x)		Inet_NtoA( (x) ## .s_addr)

static struct Library *mx_SocketBase;
# define SocketBase mx_SocketBase


/*pszTarget: pointer to store the readable name in
**pDomain:   domain, may use compression
**pMsgStart: pointer to the beginning of the message
**           (starting with the id field)
**returnvalue: points behind the domain-data at pDomain*/
static char *DNSTool_DomainToName(char *pszTarget,char *pDomain,void *pMsgStart)
 {
 int iLabel=0;
 while (0!=*pDomain)
  {
  if (0!=iLabel)
   *(pszTarget++)='.';
  iLabel++;
  if (0xC0==(0xC0&*pDomain))
   {
   int iOffs=0;
   iOffs+=(int)(0x003f&((int)*pDomain))<<8;
   iOffs+=(int)0x00ff&((int)*(pDomain+1));
   DNSTool_DomainToName(pszTarget,((char*)pMsgStart)+iOffs,pMsgStart);
   return(pDomain+2);
   }
  strncpy(pszTarget,pDomain+1,*pDomain);
  pszTarget+=*pDomain;
  pDomain+=*pDomain+1;
  }
 *pszTarget=0;
 return(pDomain+1);
 }

#if DNSTOOL_DEBUG
static const char *DNSTool_ClassAsText(int iQClass)
 {
 const char *pcszRet;
 pcszRet=(1==iQClass)?"IN"
        :(2==iQClass)?"CS"
        :(3==iQClass)?"CH"
        :(4==iQClass)?"HS"
        :(255==iQClass)?"*"
        :"<unknown>";
 return(pcszRet);
 }
#endif


#if DNSTOOL_DEBUG
static const char *DNSTool_TypeAsText(int iQType)
 {
 const char *pcszRet;
 pcszRet=(1==iQType)?"A"
        :(2==iQType)?"NS"
        :(3==iQType)?"MD"
        :(4==iQType)?"MF"
        :(5==iQType)?"CNAME"
        :(6==iQType)?"SOA"
        :(7==iQType)?"MB"
        :(8==iQType)?"MG"
        :(9==iQType)?"MR"
        :(10==iQType)?"NULL"
        :(11==iQType)?"WKS"
        :(12==iQType)?"PTR"
        :(13==iQType)?"HINFO"
        :(14==iQType)?"MINFO"
        :(15==iQType)?"MX"
        :(16==iQType)?"TXT"
        :(252==iQType)?"AXFR"
        :(253==iQType)?"MAILB"
        :(254==iQType)?"MAILA"
        :(255==iQType)?"*"
        :"<unknown>";
 return(pcszRet);
 }
#endif


/*pStart:    pointer to resource-record
**pMsgStart: pointer to the beginning of the message
**           (starting with the id field)
**returnvalue: points behind the resource-record*/
static char *DNSTool_PrintRecord(char *pStart,void *pMsgStart, struct mx_exchange *mxe_dest)
 {
 char szTmp[256];
 int iQType=0;
 int iQClass=0;
 long lTTL=0;
 int iRDLength=0;
 pStart=DNSTool_DomainToName(szTmp,pStart,pMsgStart);
 KP("NAME='%s' ",szTmp);

 iQType=(int)(0x00FF&(int)*pStart)<<8;
 iQType+=(int)(0x00FF&(int)*(pStart+1));
 pStart+=2;

#if DNSTOOL_DEBUG
 KP("QTYPE=%s ", DNSTool_TypeAsText(iQType));
#endif

 iQClass=(int)(0x00FF&(int)*pStart)<<8;
 iQClass+=(int)(0x00FF&(int)*(pStart+1));
 pStart+=2;
	//	 KP("QCLASS=%s ", DNSTool_ClassAsText(iQType));
 
 lTTL=(long)(0x00FF&(long)*pStart)<<24;
 lTTL+=(long)(0x00FF&(long)*(pStart+1))<<16; 
 lTTL+=(long)(0x00FF&(long)*(pStart+2))<<8;
 lTTL+=(long)(0x00FF&(long)*(pStart+3));
 pStart+=4;
 KP("TTL=%ld ",lTTL);

 iRDLength=(int)(0x00FF&(int)*pStart)<<8;
 iRDLength+=(int)(0x00FF&(int)*(pStart+1));
 pStart+=2;
 KP("RDLENGTH=%ld\n",iRDLength);

 if (15==iQType) /*MX*/
  {
  int iPref=0;
  iPref=(int)(0x00FF&(int)*pStart)<<8;
  iPref+=(int)(0x00FF&(int)*(pStart+1));
  KP("(MX)PREFERENCE=%ld\n",iPref);
  DNSTool_DomainToName(szTmp,pStart+2,pMsgStart);
  KP("(MX)EXCHANGE='%s'",szTmp);
  
  if((mxe_dest->domain[mxe_dest->internal_matching_answers] = strdup(szTmp)))
   {
  	mxe_dest->domain[++mxe_dest->internal_matching_answers] = NULL;
   }
  }
  else
  {
  	KP("*************** UNSUPPORTED iQType %ld\n", iQType);
  }
 
 KP("\n--------------\n");
 return(pStart+iRDLength);
 }


#if 1
/*pStart:    pointer to question-record
**pMsgStart: pointer to the beginning of the message
**           (starting with the id field)
**returnvalue: points behind the question-record*/
static char *DNSTool_PrintQuestion(char *pStart,void *pMsgStart)
 {
 char szTmp[256];
 int iQType=0;
 int iQClass=0;
 pStart=DNSTool_DomainToName(szTmp,pStart,pMsgStart);
 
 iQType=(int)(0x00FF&(int)*pStart)<<8;
 iQType+=(int)(0x00FF&(int)*(pStart+1));
 pStart+=2;

 iQClass=(int)(0x00FF&(int)*pStart)<<8;
 iQClass+=(int)(0x00FF&(int)*(pStart+1));
 pStart+=2;

 KP("QNAME='%s'\n"
        "QTYPE=%s\n"
        "QCLASS=%s\n"
        "--------------\n"
       ,szTmp
       ,DNSTool_TypeAsText(iQType)
       ,DNSTool_ClassAsText(iQClass)
    );

 return(pStart);
 }
#endif


/*pTarget: pointer to store domain in
**pszName: domain-name in readable format
*/
static void DNSTool_NameToDomain(char *pTarget,char *pszName)
 {
 while (0!=*pszName)
  {
  int iCnt=0;
  while (('.'!=pszName[iCnt])&&(0!=pszName[iCnt]))
   {
   pTarget[iCnt+1]=pszName[iCnt];
   iCnt++;
   }
  *pTarget=(char)iCnt;
  pTarget+=1+iCnt;
  pszName+=iCnt;
  if ('.'==*pszName)
   pszName++;
  }
 *pTarget=0;
 return;
 }




#if DNSTOOL_ERRSTR
#define SETMX_ERROR(code, msg)						\
({									\
	mxe_dest->error = TRUE;						\
	/*if(mxe_dest->error_string)					\
	{								\
		DisplayBeep(0L); free( mxe_dest->error_string );	\
	}	*/							\
	mxe_dest->error_string = strdup( msg );				\
})
#else
# define SETMX_ERROR(code, msg)	\
	mxe_dest->error_code = code
#endif

#undef SocketBase

VOID GetMXDomain( char *address, struct mx_exchange *mxe_dest, char *NameServer)
{
 char *pszAuthoritativeNS=NULL;

 if (!address || !mxe_dest /*|| !SocketBase*/)
  {
#if DNSTOOL_ERRSTR
  	mxe_dest->error = TRUE;
  	
  	mxe_dest->error_string = strdup(
  	((!address && !mxe_dest) ? "Invalid address && mxe_dest!" :
  	(!address ? "Invalid address" : "Invalid mxe_dest")));
#else
	
	if(!address)	SETMX_ERROR(10,"");
	if(!mxe_dest)	SETMX_ERROR(20,"");
//	if(!SocketBase)	SETMX_ERROR(30,"");
	
#endif
  }
 else
  {
  char *pszNameServ;
  int iAnswerCount;
  int iNSReferral=-1;
  struct hostent *pNameserv;
  struct Library *SocketBase;
  
  mx_SocketBase		= (SocketBase = OpenLibrary(__bsdsocketname, 4));
  
  if(!SocketBase)
  {
  	SETMX_ERROR(88,""); return;
  }
  
  if(!(pszNameServ = NameServer))
  	pszNameServ = DNSTool_NServer;
  
TryOtherServer: 

  iNSReferral++;
  iAnswerCount=0;

  pNameserv=gethostbyname(pszNameServ);
  if (NULL==pNameserv)
   SETMX_ERROR(50, "Invalid name server");
  else
   {
   struct in_addr NsAddr=*((struct in_addr*)pNameserv->h_addr_list[0]);
   int nSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP/*protocol*/);
   
   KP("used nameserver: '%s' (%s)\n",pNameserv->h_name,inet_ntoa(NsAddr));

   /*clear the authoritative ns not to retry using the same ns again*/
   if (NULL!=pszAuthoritativeNS)
    {
    free(pszAuthoritativeNS);
    pszAuthoritativeNS=NULL;
    pszNameServ=NULL;
    }

   if (-1==nSocket)
    SETMX_ERROR(60, "Could not create socket");
   else
    {
    struct sockaddr_in ConAddr;
    memset(&ConAddr,0,sizeof(ConAddr));
    ConAddr.sin_len=sizeof(ConAddr);
    ConAddr.sin_family=AF_INET;
    ConAddr.sin_port=DNSTool_Port;
    ConAddr.sin_addr=NsAddr;

    if (0!=connect(nSocket,(struct sockaddr*)&ConAddr,ConAddr.sin_len))
     {
     	KP("Error: connect() failed (errno=%ld)\n",Errno());
     	
     	SETMX_ERROR(70, "connect() failed");
     }
    else
     {
     char *puchBuf=(char*)malloc(DNSTool_MSGSIZE);
     if (NULL!=puchBuf)
      {
      char *puchTmp=puchBuf;
      *(puchTmp++)=0; //skip overall length-field
      *(puchTmp++)=0; //skip overall length-field
      /*Header section*/
      *(puchTmp++)=0; //no id
      *(puchTmp++)=0;
      *(puchTmp++)=0; //opcode
      *(puchTmp++)=0; //rcode
      *(puchTmp++)=0; //QDCOUNT
      *(puchTmp++)=1;
      *(puchTmp++)=0; //ANCOUNT
      *(puchTmp++)=0;
      *(puchTmp++)=0; //NSCOUNT
      *(puchTmp++)=0;
      *(puchTmp++)=0; //ARCOUNT
      *(puchTmp++)=0;
      /*Question section*/
      DNSTool_NameToDomain(puchTmp, address);
      puchTmp+=strlen(puchTmp)+1;
      *(puchTmp++)=0; //QTYPE
      *(puchTmp++)=15;		// MX QUERY
      *(puchTmp++)=0; //QCLASS
      *(puchTmp++)=1; //1=IN
      
      /*set overall length*/
      *(puchBuf)=0xFF&(htons(puchTmp-puchBuf-2)>>8);
      *(puchBuf+1)=0xFF&htons(puchTmp-puchBuf-2);
      if (-1==send(nSocket,puchBuf,puchTmp-puchBuf,0/*flags*/))
       SETMX_ERROR(80, "send() failed\n");
      else
       {
       int irecv=0;
       KP("query has been sent, now waiting for reply...\n");
       memset(puchBuf,0,DNSTool_MSGSIZE);
       irecv=recv(nSocket,puchBuf,DNSTool_MSGSIZE,0/*flags*/);
       if (-1==irecv)
        SETMX_ERROR(90, "recv() failed");
       else
        {
        int iRecLen=0;
        int iQDCount=0;
        int iANCount=0;
        int iNSCount=0;
        int iARCount=0;
        puchTmp=puchBuf;
        iRecLen=(int)((int)(*puchBuf))<<8;
        iRecLen+=0xff&((int)*(puchBuf+1));
        puchTmp+=2; //skip length field
        puchTmp+=2; //skip id field
        
        KP("recv returned %ld; returned length=%ld\n",irecv,iRecLen);
        KP("RCODE=%ld (",0x0f&(*(puchTmp+1)));
        
        switch (0x0f&(*(puchTmp+1)))
         {
         case 0:  KP("no error");        break;
         case 1:  KP("format error");    break;
         case 2:  KP("server failure");  break;
         case 3:  KP("name error");      break;
         case 4:  KP("not implemented"); break;
         case 5:  KP("refused");         break;
         default: KP("unknown RCODE");   break;
         }
        KP(")\n");
        
        
        iQDCount=(int)(*(puchTmp+2))<<8;
        iQDCount+=0xff&((int)(*(puchTmp+3)));
        iANCount=(int)(*(puchTmp+4))<<8;
        iANCount+=0xff&((int)(*(puchTmp+5)));
        iNSCount=(int)(*(puchTmp+6))<<8;
        iNSCount+=0xff&((int)(*(puchTmp+7)));
        iARCount=(int)(*(puchTmp+8))<<8;
        iARCount+=0xff&((int)(*(puchTmp+9)));
        puchTmp+=10;
        
        KP("QDCOUNT=%ld\n",iQDCount);
        KP("ANCOUNT=%ld\n",iANCount);
        KP("NSCOUNT=%ld\n",iNSCount);
        KP("ARCOUNT=%ld\n",iARCount);
#if 1
        /*print question section*/
        KP("************************************************************\n");
        KP("Question Section:\n");
        KP("************************************************************\n");
        while (0!=iQDCount)
         {
         puchTmp=DNSTool_PrintQuestion(puchTmp,puchBuf+2);
         iQDCount--;
         }
#endif
        /*print answer section*/
        KP("************************************************************\n");
        KP("Answer Section:\n");
        KP("************************************************************\n");
        while (0!=iANCount)
         {
         char szTmp[256];
         int iQType=0;
         char *pPP=puchTmp;
         pPP=DNSTool_DomainToName(szTmp,pPP,puchBuf+2);
         iQType=(int)(0x00FF&(int)*pPP)<<8;
         iQType+=(int)(0x00FF&(int)*(pPP+1));
         /*is it a record of the requested type?*/
#if 0 // original
         if((0==strcmpi(argv[2],DNSTool_TypeAsText(iQType)))
          ||(0==strcmpi("*",argv[2])))
#else
	if( iQType == 15 ) // "MX"
#endif
          iAnswerCount++;
         puchTmp=DNSTool_PrintRecord(puchTmp,puchBuf+2, mxe_dest);
         iANCount--;
         }
         
        /*print name server section*/
        KP("************************************************************\n");
        KP("Name server Section:\n");
        KP("************************************************************\n");
        while (0!=iNSCount)
         {
         char szTmp[256];
         int iQType=0;
         char *pPP=puchTmp;
         pPP=DNSTool_DomainToName(szTmp,pPP,puchBuf+2);
         iQType=(int)(0x00FF&(int)*pPP)<<8;
         iQType+=(int)(0x00FF&(int)*(pPP+1));
         pPP+=2+2+4+2;
         pPP=DNSTool_DomainToName(szTmp,pPP,puchBuf+2);
         if((2==iQType) /*NS*/
          &&(NULL==pszAuthoritativeNS))
          {
          /*record authoritative ns for possible retry later*/
          pszAuthoritativeNS=(char*)malloc(sizeof(szTmp));
          if (NULL!=pszAuthoritativeNS)
           strcpy(pszAuthoritativeNS,szTmp);
          }
         puchTmp=DNSTool_PrintRecord(puchTmp,puchBuf+2, mxe_dest);
         iNSCount--;
         }
        /*print additional records section*/
        KP("************************************************************\n");
        KP("Additional records Section:\n");
        KP("************************************************************\n");
        while (0!=iARCount)
         {
         puchTmp=DNSTool_PrintRecord(puchTmp,puchBuf+2, mxe_dest);
         iARCount--;
         }

        } /*if recv()*/
       } /*if send()*/

      free(puchBuf);
      }
     } /*if connect()*/
    CloseSocket(nSocket);
    }
   } /*if gethostbyname()*/

  /*if necessary,retry with other nameserver*/
  if((NULL!=pszAuthoritativeNS)
   &&(0==iAnswerCount))
   {
   pszNameServ=pszAuthoritativeNS;
   KP(">>\n>>no answer matched the query; retrying using '%s'\n>>\n"
         ,pszNameServ);
   goto TryOtherServer;
   }
  
  mxe_dest->matching_answers	= iAnswerCount;
  mxe_dest->nameserver_referrals= iNSReferral;
  
  KP(">>\n>>received %ld matching answers after %ld nameserver referrals\n>>\n"
        ,iAnswerCount
        ,iNSReferral);
	
	CloseLibrary( SocketBase );
	
  } /*if argc*/


 if (NULL!=pszAuthoritativeNS)
  {
  free(pszAuthoritativeNS);
  pszAuthoritativeNS=NULL;
  }
}
