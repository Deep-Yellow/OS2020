
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

SEMAPHORE writeblock;
SEMAPHORE mutex;
SEMAPHORE readblock;
SEMAPHORE writer_firsrt;
SEMAPHORE *wb;
SEMAPHORE *mt;
SEMAPHORE *rb;
int read = 0;
int write = 0;
int readcount = 0;
int writecount = 0;

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;

	for (p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		if (p->sleep_time > 0)
		{
			p->sleep_time--; //减掉睡眠的时间
		}
	}
	while (!greatest_ticks)
	{
		for (p = proc_table; p < proc_table + NR_TASKS; p++)
		{
			if (p->wait > 0 || p->sleep_time > 0)
			{
				continue; //若在等待状态或有睡眠时间，就不分配时间片
			}
			if (p->ticks > greatest_ticks)
			{
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		if (!greatest_ticks)
		{

			for (p = proc_table; p < proc_table + NR_TASKS; p++)
			{
				if (p->wait > 0 || p->sleep_time > 0)
				{
					continue; //若在等待状态或有睡眠时间，就不分配时间片
				}
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC int sys_p(SEMAPHORE *t)
{
	t->count--;
	if (t->count < 0)
	{
		p_proc_ready->wait = 1;
		t->proc_deque[t->wait_num] = p_proc_ready;
		t->wait_num++; //进入等待进程队列
		schedule();
	}
}

PUBLIC int sys_v(SEMAPHORE *t)
{
	t->count++;
	if (t->count <= 0)
	{
		t->proc_deque[0]->wait = 0;
		for (int i = 0; i < t->wait_num; i++)
		{
			t->proc_deque[i] = t->proc_deque[i + 1];
		}
		t->wait_num--;
	}
}

PUBLIC int sys_sleep(int k)
{
	p_proc_ready->sleep_time = k;
	schedule();
	return 0;
}

PUBLIC int sys_my_print(char *str)
{
	TTY *p_tty = tty_table;
	int i = 0;
	int color = p_proc_ready - proc_table;
	while (str[i] != '\0')
	{
		print_color_char(p_tty->p_console, str[i], color);
		i++;
	}
}

/**--------------------------------------------------------------------------------
 * 						writer & reader
 ----------------------------------------------------------------------------------*/

//reader first
// PUBLIC void reader(char *name, int t, int inter)
// {
// 	do_my_print(name);
// 	do_my_print(" want to read\n");

// 	do_p(mt);
// 	read++;
// 	if (read == 1)
// 	{
// 		do_p(wb);
// 	}

// 	do_p(rb);
// 	do_my_print(name);
// 	do_my_print(" is reading\n");
// 	readcount++;
// 	do_v(mt);

// 	do_sleep(t);

// 	do_my_print(name);
// 	do_my_print(" finished reading\n");
// 	do_v(rb);

// 	do_p(mt);
// 	readcount--;
// 	do_v(mt);

// 	do_p(mt);
// 	read--;
// 	if (read == 0)
// 	{
// 		do_v(wb);
// 	}
// 	do_v(mt);
// 	do_sleep(inter);
// }

// PUBLIC void writer(char *name, int t, int inter)
// {
// 	do_my_print(name);
// 	do_my_print(" want to write\n");

// 	do_p(wb);
// 	do_my_print(name);
// 	do_my_print(" is writing\n");
// 	writecount = 1;
// 	do_sleep(t);
// 	do_my_print(name);
// 	do_my_print(" finished writing\n");
// 	writecount = 0;
// 	do_v(wb);

// 	do_sleep(inter);
// }

//writer first

PUBLIC void reader(char *name, int t, int inter)
{
	do_my_print(name);
	do_my_print(" want to read\n");

	do_p(&writer_firsrt);
	do_p(mt);
	read++;
	if (read == 1)
	{
		do_p(wb);
	}

	do_p(rb);
	do_my_print(name);
	do_my_print(" is reading\n");
	readcount++;
	do_v(mt);
	do_v(&writer_firsrt);

	do_sleep(t);

	do_my_print(name);
	do_my_print(" finished reading\n");
	do_v(rb);

	do_p(mt);
	readcount--;
	do_v(mt);

	do_p(mt);
	read--;
	if (read == 0)
	{
		do_v(wb);
	}
	do_v(mt);
	do_sleep(inter);
}

PUBLIC void writer(char *name, int t, int inter)
{
	do_my_print(name);
	do_my_print(" want to write\n");

	do_p(&writer_firsrt);
	do_p(wb);
	do_my_print(name);
	do_my_print(" is writing\n");
	writecount = 1;
	do_sleep(t);
	do_my_print(name);
	do_my_print(" finished writing\n");
	writecount = 0;
	do_v(wb);
	do_v(&writer_firsrt);

	do_sleep(inter);
}

PUBLIC void report()
{
	if (readcount > 0)
	{
		if (readcount == 1)
		{
			do_my_print("ONE IS READING\n");
		}
		else if (readcount == 2)
		{
			do_my_print("TWO IS READING\n");
		}
		else
		{
			do_my_print("THREE IS READING\n");
		}
	}
	else
	{
		if (writecount == 1)
		{
			do_my_print("ONE IS WRITING\n");
		}
		else
		{
			do_my_print("NOTING!\n");
		}
	}
}

PUBLIC void initSemaphore()
{
	writer_firsrt.count = 1;
	writeblock.count = 1;
	mutex.count = 1;
	readblock.count = 3; //reader nums
	wb = &writeblock;
	mt = &mutex;
	rb = &readblock;
	read = 0;
	write = 0;
	readcount = 0;
	writecount = 0;
}