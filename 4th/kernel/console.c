
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

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

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);
PRIVATE u32 ESC_counter;
PRIVATE u32 word_length = 0;
PRIVATE u8 *start_index = 0;
PRIVATE u32 origin_cursor = 0;

// 颜色常量： BLUE是换行符添的空格、 RED是查询模式、WHITE是TAB的空格
PRIVATE u8 colors[7] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	ESC_counter = 0;

	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		out_char(p_tty->p_console, nr_tty + '0', 0);
		out_char(p_tty->p_console, '#', 0);
	}

	set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch, u8 mode)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	u8 *p_vmem_start;
	u32 same_bit_counter;

	switch (ch)
	{
	case '\n':
		p_vmem_start = (u8 *)(V_MEM_BASE);
		if (mode == 2)
		{
			//ESC 后按的第一个回车  代表查找
			while (p_vmem > p_vmem_start)
			{
				same_bit_counter = 0;
				for (int i = 0; i < 2 * word_length; i += 2)
				{
					if ((*(p_vmem_start + i) == *(start_index + i)) && ((*(p_vmem_start + i + 1) == DEFAULT_CHAR_COLOR && *(start_index + i + 1) == RED) ||
																		(*(p_vmem_start + i + 1) == L_RED && *(start_index + i + 1) == L_RED)))
					{
						same_bit_counter++;
					}
					else
					{
						break;
					}
				}
				if (same_bit_counter == word_length)
				{
					for (int i = 0; i < word_length * 2; i += 2)
					{
						if (*(p_vmem_start + i + 1) == DEFAULT_CHAR_COLOR)
						{
							*(p_vmem_start + i + 1) = RED;
						}
					}
				}
				p_vmem_start++;
			}
		}

		else if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH)
		{
			u32 next_line_head = p_con->original_addr + SCREEN_WIDTH *
															((p_con->cursor - p_con->original_addr) /
																 SCREEN_WIDTH +
															 1);
			while (p_con->cursor < next_line_head)
			{
				p_con->cursor++;
				*p_vmem++ = ' ';
				*p_vmem++ = BLUE;
			}
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			int count = 0;
			if (*(p_vmem - 1) == BLUE)
			{
				while (*(p_vmem - 1) == BLUE && count < SCREEN_WIDTH)
				{
					p_con->cursor--;
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					p_vmem -= 2;
					count++;
				}
			}
			else if (*(p_vmem - 1) == L_RED)
			{
				for (int i = 0; i < 4; i++)
				{
					p_con->cursor--;
					if (mode == 0 || mode == 1)
					{
						*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					}
					else
					{
						word_length--;
					}
					*(p_vmem - 2) = ' ';
					p_vmem -= 2;
				}
			}

			else
			{
				p_con->cursor--;
				*(p_vmem - 2) = ' ';
				*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
			}
		}
		break;

	case 27:
		if (ESC_counter % 2 == 0 && mode == 1)
		{
			start_index = p_vmem++;
			word_length = 0;
			origin_cursor = p_con->cursor;
		}

		else if (mode == 0 && ESC_counter % 2 == 1)
		{
			p_vmem_start = (u8 *)(V_MEM_BASE);
			while (p_vmem >= p_vmem_start)
			{
				if (*(p_vmem_start + 1) == RED)
				{
					*(p_vmem_start + 1) = DEFAULT_CHAR_COLOR;
				}
				p_vmem_start += 2;
			}
			for (int i = 0; i < 2 * word_length; i = i + 2)
			{
				*(start_index + i) = ' ';
			}
			p_con->cursor = origin_cursor; // 还原完成
		}
		ESC_counter++;
		break;

	default:
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1)
		{

			*p_vmem++ = ch;
			if (mode == 3)
			{
				*p_vmem++ = L_RED;
			}
			else if (mode == 4)
			{
				*p_vmem++ = L_RED;
				word_length++;
			}
			else if (mode == 1)
			{
				word_length++;
				*p_vmem++ = RED;
			}
			else
			{
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}

			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           print_color_char
*======================================================================*/

PUBLIC void print_color_char(CONSOLE *p_con, char ch, u8 color)
{

	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	switch (ch)
	{
	case '\n':
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH)
		{
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
		}
		break;

	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			*p_vmem++ = colors[color];
			//*p_vmem++ = DEFAULT_CHAR_COLOR;
			// if(color==0){
			// *p_vmem++ =WHITE;
			// }else if(color==1){
			// 	*p_vmem++ =RED;
			// }else if(color==2){
			// 	*p_vmem++ =BLUE;
			// }else if(color==3){
			// 	*p_vmem++ = GREEN;
			// }else if(color==4){
			// 	*p_vmem++ = NEW_COLOR1;
			// }else if(color==5){
			// 	*p_vmem++ = NEW_COLOR2;
			// }else if(color==6){
			// 	*p_vmem++ = WHITE;
			// }

			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

PUBLIC void clean_screen(CONSOLE *p_console)
{
	u8 *start = (u8 *)(V_MEM_BASE);
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_console->cursor * 2);

	while (start < p_vmem)
	{
		p_vmem--;
		*p_vmem = DEFAULT_CHAR_COLOR;
		p_vmem--;
		*p_vmem = ' ';
	}

	p_console->cursor = p_console->original_addr;
	p_console->current_start_addr = p_console->original_addr;

	flush(p_console);
}