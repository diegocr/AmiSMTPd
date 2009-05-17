/*
** Threading functions
** Copyright © 1999-2000 by Gürer Özen
**
*/

#include "globals.h"
#include <dos/dostags.h>

struct MsgPort *th_mother=NULL;
thread th_list=NULL;
u_long th_sigmask=0;

void th_handlemsg(thmsg m);
void th_cleanup(thread t);


int th_init(void)
{
	th_mother = CreateMsgPort();
	if (!th_mother) return(0);
	th_sigmask = 1L << (th_mother->mp_SigBit);
	return(1);
}


void th_exit(void)
{
	thread t;

	t = th_list;
	while (t) {
		th_message(t, THC_EXIT, 0);
		t = t->next;
	}

	while (th_list) {
		thmsg m;
		WaitPort(th_mother);
		m = (thmsg)GetMsg(th_mother);
		th_handlemsg(m);
	}

	DeleteMsgPort(th_mother);
	th_sigmask = 0;
}


thread th_spawn(thcb handler, char *name, void (*func)(void), int pri, APTR data)
{
	thread t;

	t = malloc(sizeof(_thread));
	if (!t) return(NULL);
	memset(t, 0, sizeof(_thread));

	t->task = (struct Task *)CreateNewProcTags(
		NP_Entry, (ULONG)func,
		NP_Name, (ULONG)name,
		NP_StackSize, STACK_SIZE *2,
		NP_Priority, pri,
		TAG_DONE);

	if (!t->task) {
		free(t);
		return(NULL);
	}

	t->handler = handler;
	t->mother = th_mother;
	t->data = data;

	t->sm.header.mn_ReplyPort = th_mother;
	t->sm.header.mn_Length = sizeof(_thmsg);
	t->sm.com = THC_STARTUP;
	t->sm.data = t;

	/* Begin new stuff */
	t->task->tc_UserData = &t->sm;
	t->mother_task = FindTask(NULL);
	SetSignal(0L, SIGF_SINGLE);
	Signal(t->task, SIGF_SINGLE);
	Wait(SIGF_SINGLE);
	/* End new stuff */

	/* Old stuff
	PutMsg(&((struct Process *)t->task)->pr_MsgPort,(struct Message *)&t->sm);
	   End old stuff */

	/* Begin new stuff */
	if (handler)	(handler)(t, THC_STARTUP, data);
	/* End new stuff */

	t->next = th_list;
	th_list = t;
	return(t);
}


void th_message(thread t, int com, APTR data)
{
	thmsg m;

	if (!t) return;

	if (!t->port) return;

	m = malloc(sizeof(_thmsg));
	if (!m) return;
	memset(m,0,sizeof(_thmsg));

	m->header.mn_ReplyPort = th_mother;
	m->header.mn_Length = sizeof(_thmsg);
	m->com = com;
	m->data = data;

	PutMsg(t->port,(struct Message *)m);
}


void th_poll(void)
{
	thread t;
	thmsg m;

	while(1) {
		m = (thmsg)GetMsg(th_mother);
		if(!m) break;

		if(m->com != THC_FINISH && (!m->isreply)) {
			t = m->sender;
			if(t && t->handler)
				(t->handler)(t,m->com,m->data);
		}

		th_handlemsg(m);
	}
}


thread thr_init(void)
{
	thread t;
	thmsg m;
	struct Task *me = FindTask(NULL), *mother;

	/* Begin new stuff */
	Wait(SIGF_SINGLE);
	m = (thmsg)me->tc_UserData;
	t = (thread)m->data;
	mother = t->mother_task;

	t->port = CreateMsgPort();
	if (!t->port) {
		me->tc_UserData = (APTR)FALSE;
		t = NULL;
	}
	else {
		me->tc_UserData = (APTR)TRUE;
		m->sender = t;
	}
	Signal(mother, SIGF_SINGLE);
	/* End new stuff */

	/* Begin old stuff
	WaitPort(&((struct Process *)me)->pr_MsgPort);
	m = (thmsg)GetMsg(&((struct Process *)me)->pr_MsgPort);
	t = m->data;

	t->port = CreateMsgPort();
	if (!t->port) return(NULL);

	m->sender = t;
	ReplyMsg((struct Message *)m);
	   End old stuff */

	return(t);
}


void thr_exit(thread t, int reason)
/* Inside the thread */
{
	thmsg m=NULL;
	BOOL waiting = TRUE;

	t->em.header.mn_ReplyPort = t->port;
	t->em.header.mn_Length = sizeof(_thmsg);
	t->em.com = THC_EXIT;
	t->em.data = (APTR)reason;
	t->em.sender = t;
	PutMsg(t->mother, (struct Message *)&t->em);

	while (waiting) {
		WaitPort(t->port);
		while((m = (thmsg)GetMsg(t->port)))
		{
			if (&t->em == m) {
				/*
				if(m->header.mn_ReplyPort != t->port)
				{
					ReplyMsg((struct Message *)m);
				}
				*/
				waiting = FALSE;
			}
			m->isreply = 1;
			/*
			if(m->header.mn_ReplyPort != t->port)
			{
				ReplyMsg((struct Message *)m);
			}
			*/
		}
	}

	t->em.header.mn_ReplyPort = NULL;
	t->em.sender = t;
	t->em.com = THC_FINISH;
	PutMsg(t->mother, (struct Message *)&t->em);

	if (t->port)
	{
		Forbid();
		DeleteMsgPort(t->port);
		Permit();
	}
}


void thr_message(thread t, int com, APTR data)
{
	thmsg m;

	m = malloc(sizeof(_thmsg));
	if (!m) return;
	memset(m, 0, sizeof(_thmsg));

	m->header.mn_ReplyPort = t->port;
	m->header.mn_Length = sizeof(_thmsg);
	m->sender = t;
	m->com = com;
	m->data = data;

	PutMsg(t->mother, (struct Message *)m);
}


/* private functions */

void th_handlemsg(thmsg m)
{
	if (m->com == THC_STARTUP) return;

	if (m->com == THC_FINISH) {
		th_cleanup(m->sender);
	}
	else {
		if (m->isreply) {
			free(m);
		}
		else {
			m->isreply = 1;
			ReplyMsg((struct Message *)m);
		}
	}
}


void th_cleanup(thread t)
{
	if (th_list==t) {
		th_list = t->next;
	}
	else {
		thread tmp = th_list;
		while (tmp) {
			if (tmp->next==t) {
				tmp->next=t->next;
				break;
			}
			tmp = tmp->next;
		}
	}
	free(t);
}
