
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

PRIVATE	u8 mode = 0; //记录输入模式 0 -> 正常输入 ; 1-> 按了一次ESC 输入要查找的字符串
					//             2 -> 结束输入要查找的字符串，改变匹配的字符串颜色
					//			   3 -> TAB 
					//			   4 -> 查询输入的TAB
PRIVATE u8 clean_flag = 0; //清屏标志
PRIVATE u32 ESC_times = 0; //记录输入了多少次ESC

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	u32 start = get_ticks();
	u32 end;

	ESC_times = 0;
	mode = 0;

	select_console(0);
	while (1) {
		// 	To-DO  20s刷新
		end = get_ticks();
		if(end - start > 20000 && (mode != 1) && (mode != 2) ){
			clean_flag = 1;
			start = end;
		}

		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};
        if (!(key & FLAG_EXT)) {	//如果是可打印字符
			if(mode != 2){
				if(mode == 3){mode=0;}
				if(mode == 4){mode=1;}
				put_key(p_tty, key);
        	}
		}
        else {
            int raw_code = key & MASK_RAW;
            switch(raw_code) {
				case TAB:
					for(int i=0;i<4;i++){
						put_key(p_tty, ' ');//认为就是加了四个空格
					}
					if(mode == 1){	mode = 4;	}
					else{	mode = 3;	}
					break;

				case ENTER:
					if(mode != 2){
						put_key(p_tty, '\n');
						if(ESC_times%2 == 1){
							mode = 2;
						}
						else{
							mode = 0;
						}
					}
					break;
            	case BACKSPACE:
					if(mode != 2){
						if(mode == 3|| mode == 0){ mode = 0; }
						else { mode = 1; }
						put_key(p_tty, '\b');
					}
					break;
        		
				case ESC:
					// To-DO 查询操作
					if( ESC_times % 2 == 0){
						mode = 1;
					}
					else{
						mode = 0;
					}
					put_key(p_tty,27);
					ESC_times++;
					break;

				case UP:
                	if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_DN);
                	}
					
					break;
				case DOWN:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_UP);
					}
					
					break;

				case F1:
				case F2:
				case F3:
				case F4:
				case F5:
				case F6:
				case F7:
				case F8:
				case F9:
				case F10:
				case F11:
				case F12:

					// /* Alt + F1~F12 */
					// if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
					// 	select_console(raw_code - F1);
					// }
					// break;
					
				default:
					
                	break;
            }
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if(clean_flag){
		clean_screen(p_tty->p_console);
		clean_flag = 0;
	}

	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch, mode);
	}
}


