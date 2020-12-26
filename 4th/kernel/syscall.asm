
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_sys_p			equ 1
_NR_sys_v			equ 2
_NR_sys_sleep		equ 3
_NR_sys_my_print	equ 4
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	do_p
global	do_v
global	do_sleep
global	do_my_print

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	push ebx
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              sys_P
;							   P操作
; ====================================================================
do_p:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_sys_p
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              sys_V
;							   V操作
; ====================================================================
do_v:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_sys_v
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              sys_sleep
;							   进程休眠
; ====================================================================
do_sleep:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_sys_sleep
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================
;                              sys_my_print
;							   封装的打印系统调用
; ====================================================================
do_my_print:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_sys_my_print
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret



