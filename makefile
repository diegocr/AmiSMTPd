
APP	= smtpd
SYS	= 
CPU	= -m68060 -m68881
PACKAGE	= $(APP)$(SYS)
CC	= gcc
XFLAGS	= $(CPU) -O3 -noixemul -nostdlib -msmall-code -funroll-loops
CFLAGS	= $(XFLAGS) -c -g -Wall -DNDEBUG
LFLAGS	= -s
LIBS	= -ldebug -lamiga

COMPILE		= $(CC) $(CFLAGS)
LINK		= $(CC) $(XFLAGS)

DEPDIR		= .deps
OBJDIR		= .objs

CCDEPMODE	= depmode=$(CC)
depcomp		= $(SHELL) /bin/depcomp

OBJS =	\
	$(OBJDIR)/startup$(SYS).o	\
	$(OBJDIR)/mx_exchange$(SYS).o	\
	$(OBJDIR)/smtpd$(SYS).o		\
	$(OBJDIR)/spool_write$(SYS).o	\
	$(OBJDIR)/spooler$(SYS).o	\
	$(OBJDIR)/thread$(SYS).o	\
	$(OBJDIR)/casolib$(SYS).o	\
	$(OBJDIR)/config$(SYS).o	\
	$(OBJDIR)/pop3$(SYS).o		\
	$(OBJDIR)/strerror$(SYS).o	\
	$(OBJDIR)/swapstack$(SYS).o	\
	$(OBJDIR)/SafeAlloc$(SYS).o	\
	$(OBJDIR)/utils$(SYS).o		\
	$(OBJDIR)/transport$(SYS).o

default:	$(PACKAGE)

$(PACKAGE): $(OBJS)
	if test -f $@; then if test -f $@~; then /bin/rm -f $@~; fi; cp -f $@ $@~; /bin/rm -f $@; fi;
	@echo Linking Debug version...
	$(LINK) -o $@.deb $(OBJS) $(LIBS)
	@echo Linking pure...
	$(LINK) $(LFLAGS) -o $@ $(OBJS) $(LIBS) -Wl,-Map,$@.map,--cref

all:
	$(MAKE) SYS='_020' CPU='-m68020-60'
	$(MAKE) SYS='_020f' CPU='-m68020 -m68881'
	$(MAKE) SYS='_040' CPU='-m68040'
	$(MAKE) SYS='_040f' CPU='-m68040 -m68881'
	$(MAKE) SYS='_060' CPU='-m68060 -m68881'


include ./$(DEPDIR)/mx_exchange$(SYS).Po
include ./$(DEPDIR)/smtpd$(SYS).Po
include ./$(DEPDIR)/spool_write$(SYS).Po
include ./$(DEPDIR)/spooler$(SYS).Po
include ./$(DEPDIR)/startup$(SYS).Po
include ./$(DEPDIR)/thread$(SYS).Po
include ./$(DEPDIR)/pop3$(SYS).Po
include ./$(DEPDIR)/transport$(SYS).Po

$(OBJDIR)/%$(SYS).o: %.c
	@echo Making $@...
	source='$<' object='$@' libtool=no \
	depfile='$(DEPDIR)/$*$(SYS).Po' tmpdepfile='$(DEPDIR)/$*$(SYS).TPo' \
	$(CCDEPMODE) $(depcomp) \
	$(COMPILE) -o $@ -c ./$<

clean:
	rm -f $(OBJDIR)/*$(SYS).o


makefile:
	/bin/makef.sh "smtpd" "-nostdlib -msmall-code -I/gg/netinclude" "" "" "" ""


.SILENT:
