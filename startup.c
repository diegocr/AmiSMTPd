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


asm(".globl __start\njbsr __start\nrts");

#include "globals.h"

STATIC CONST UBYTE __version[] USED_VAR = 
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	"$VER:" _PROGRAM_NAME " " _PROGRAM_VERZ " " _PROGRAM_DATE " " _PROGRAM_COPY 
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
;
struct ExecBase		*SysBase	= NULL;
struct DosLibrary	*DOSBase	= NULL;
struct Library		*SocketBase	= NULL;
struct Library		*LocaleBase	= NULL;
struct UtilityBase	*UtilityBase	= NULL;

int h_errno = 0, errno = 0;
struct Globals G;
struct pop3 P;

static struct RDArgs *args = NULL;
static struct MsgPort *myPort = NULL;

GLOBAL int smtpd(void);
GLOBAL int __swap_stack_and_call(struct StackSwapStruct * stk,APTR function);
static int _exit(int r);
static int _main( void );

char __dosname[] = "dos.library", __bsdsocketname[] = "bsdsocket.library",
	__utilityname[] = "utility.library", __localename[] = "locale.library";

INLINE BOOL OpenLibs(void)
{
	if(!(DOSBase=(struct DosLibrary*)OpenLibrary(__dosname,36)))
		return FALSE;
	
	if(!(LocaleBase = OpenLibrary(__localename,0)))
		return FALSE;
	
	if(!(UtilityBase = (struct UtilityBase *)OpenLibrary(__utilityname,0)))
		return FALSE;
	
	if(!(SocketBase = OpenLibrary(__bsdsocketname, 4)))
		return FALSE;
	
	return TRUE;
}

INLINE int stack_passport( void )
{
	APTR new_stack;
	int return_code = 10;
	unsigned int stack_size, __stack_size = STACK_SIZE ;
	struct StackSwapStruct *stk;
	struct Task *this_task = FindTask(NULL);
	
	__stack_size += ((ULONG)this_task->tc_SPUpper-(ULONG)this_task->tc_SPLower);
	
	/* Make the stack size a multiple of 32 bytes. */
	stack_size = 32 + ((__stack_size + 31UL) & ~31UL);
	
	/* Allocate the stack swapping data structure
	   and the stack space separately. */
	stk = (struct StackSwapStruct *) AllocVec( sizeof(*stk), MEMF_PUBLIC|MEMF_ANY );
	if(stk != NULL)
	{
		new_stack = AllocMem(stack_size,MEMF_PUBLIC|MEMF_ANY);
		if(new_stack != NULL)
		{
			/* Fill in the lower and upper bounds, then
			   take care of the stack pointer itself. */
			
			stk->stk_Lower	= new_stack;
			stk->stk_Upper	= (ULONG)(new_stack)+stack_size;
			stk->stk_Pointer= (APTR)(stk->stk_Upper - 32);
			
			return_code = __swap_stack_and_call(stk,(APTR)_main);
			
			FreeMem(new_stack, stack_size);
		}
		
		FreeVec(stk);
	}
	
	return return_code;
}

STATIC int _main( void )
{
	LONG arg[2] = {0};
	ULONG sbterr = 0;
	
	G.NetSig = -1;
	G.port = 25;
	
	SMTP_SOCK = -1;
	POP3_SOCK = -1;

	bzero( &P, sizeof(struct pop3));
	
	if(!OpenLibs())
		return _exit(10);
	
	if(!InitMallocSystem ( ))
		return _exit(10);
	
	G.DOSBase = DOSBase;
	G.SocketBase = SocketBase;
	
	sbterr = SocketBaseTags(
		       SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), (ULONG)&errno,
		       SBTM_SETVAL(SBTC_HERRNOLONGPTR), (ULONG)&h_errno,
		       //SBTM_SETVAL(SBTC_LOGTAGPTR),	(ULONG)_PROGRAM_NAME,
		       //SBTM_SETVAL(SBTC_LOGTAGPTR),	(ULONG)FilePart(FindTask(NULL)->tc_Node.ln_Name),
		       SBTM_SETVAL(SBTC_LOGTAGPTR),	(ULONG) "smtpd",
		       SBTM_SETVAL(SBTC_LOGSTAT),	/*LOG_PID |*/ LOG_CONS,
		       SBTM_SETVAL(SBTC_LOGFACILITY),	LOG_DAEMON,
		       SBTM_SETVAL(SBTC_LOGMASK),	LOG_UPTO(LOG_INFO),
		       TAG_END);
	if(sbterr)
	{
		err("BaseTags error on %ld", sbterr);
		return _exit(30);
	}
	
	if((args = ReadArgs( "PORT/N,DEBUG/S", (long*)arg, NULL)))
	{
		if(arg[0])
			G.port = *((ULONG *) arg[0]);
		
		if(((BOOL)arg[1]))
			SocketBaseTags(SBTM_SETVAL(SBTC_LOGMASK), LOG_UPTO(LOG_DEBUG), TAG_END);
	}
	
	deb("Initializing...");
	
	if(!th_init())
	{
		err("Failed Creating MsgPort");
		return _exit(40);
	}
	
	deb("Alloc Signals...");
	
	if((G.NetSig = AllocSignal(-1)) == -1)
	{
		err("Failed Allocating Signals");
		return _exit(50);
	}
	
	return _exit(smtpd());
}

static int _exit(int r)
{
	nfo("");
	nfo("exiting.");
	
	if(G.NetSig != -1)
		FreeSignal(G.NetSig);
	
	if(SMTP_SOCK != -1)
		CloseSocket(SMTP_SOCK);
	if(POP3_SOCK != -1)
		CloseSocket(POP3_SOCK);
	
	if(args)
		FreeArgs( args );
	
	if(r >= 40 || r == 0)
		th_exit();
	
	ClearMallocSystem ( );
	
	if(DOSBase)
		CloseLibrary((struct Library*)DOSBase);
	
	if(LocaleBase)
		CloseLibrary(LocaleBase);
	
	if(UtilityBase)
		CloseLibrary((struct Library *)UtilityBase);
	
	if(SocketBase)
		CloseLibrary(SocketBase);
	
	if(myPort)
		DeletePort(myPort);
	
	return r;
}

int _start( void )
{
	SysBase = *(struct ExecBase **) 4L;	
	
	Forbid();
	myPort = FindPort(_PROGRAM_NAME);
	Permit();
	
	if(myPort)
		return 1024;
	
	if(!(myPort = CreatePort(_PROGRAM_NAME,0)))
		return 1025;
	
//	return _main();
	return stack_passport();
}
