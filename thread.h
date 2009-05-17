#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>
#include <exec/ports.h>
#include <exec/tasks.h>

extern u_long th_sigmask;

struct thread_struct;

typedef void (*thcb)(struct thread_struct *t, int com, APTR data);

typedef struct thmsg_struct
{
	struct Message header;
	struct thread_struct *sender;
	int com;
	APTR data;
	int isreply;
} *thmsg, _thmsg;

typedef struct thread_struct
{
	struct thread_struct *next;
	struct Task *task;
	struct MsgPort *port;
	struct MsgPort *mother;
	struct Task *mother_task;
	thcb handler;
	APTR data;
	_thmsg sm;
	_thmsg em;
} *thread, _thread;

enum thread_state {
	THC_STARTUP = -1,
	THC_EXIT = -2,
	THC_FINISH = -3
};


extern int th_init(void);
extern void th_exit(void);

extern thread th_spawn(thcb handler, char *name, void (*func)(void), int pri, APTR data);
extern void th_message(thread t, int com, APTR data);
extern void th_poll(void);

/* following functions are called from threads */
extern thread thr_init(void);
extern void thr_exit(thread t, int reason);
extern void thr_message(thread t, int com, APTR data);


#ifdef __MORPHOS__
#define THREAD_DECL(x) struct EmulLibEntry x
#define THREAD(x) \
	static void x ## _gate(void); \
	struct EmulLibEntry x = { \
	TRAP_LIB, 0, (void (*)(void)) & x ## _gate }; \
	static void x ## _gate(void)
#else
#define THREAD_DECL(x) void ASM SAVEDS x (void)
#define THREAD(x) void ASM SAVEDS x (void)
#endif


#endif	/* THREAD_H */
