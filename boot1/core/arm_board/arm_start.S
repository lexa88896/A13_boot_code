/*
**********************************************************************************************************************
*											        eGon
*						           the Embedded GO-ON Bootloader System
*									       eGON arm boot sub-system
*
*						  Copyright(C), 2006-2010, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : arm_start
*
* By      : Jerry
*
* Version : V2.00
*
* Date	   :  2020-9-19
*
* Descript:
*
**********************************************************************************************************************
*/

	#include "arm_a8.h"
/*********************************BOOT ϵͳ��ʼ������********************************/
/*
@start
@; ARM ������ ..........
@; 00 - Reset
@; 04 - Undefined instructions
@; 08 - SWI instructions
@; 0C - prefetch abort
@; 10 - Data abort
@; 14 - Reserved
@; 18 - IRQ interrupts
@; 1C - FIQ interrupts
@;֧�ֳ���ת
*/
     ldr     pc,_undefined_instruction
     ldr     pc,_undefined_instruction
     ldr     pc,_software_interrupt
     ldr     pc,_undefined_instruction
     ldr     pc,_undefined_instruction
     ldr     pc,_undefined_instruction
     ldr     pc,_irq
     ldr     pc,_undefined_instruction

_undefined_instruction:	.word undefined_instruction
_software_interrupt:     .word software_interrupt
_irq:					.word irq_interrupt

#define     INTC_REG_VCTR    0x1c20400
/*---------------------------------------------------------------------------------------*/

irq_interrupt:
	sub     lr, lr, #4                                      @; ���淵�ص�ַ
	stmfd   sp!, {r0-r12, lr}@; save context       	        @; �Ĵ���ѹջ
	mrs     r3, spsr                                        @; ��ȡSPSR
	stmfd   sp!, {r3}	                                    @; ѹջ

    msr     cpsr_c, #(ARMV7_FIQ_MASK | ARMV7_IRQ_MASK | ARMV7_SYSTEM_MODE)     @; �л���SYSTEMģʽ
    stmfd   sp!, {r0-r12, lr}                                @; ����lr_usr�������õ��ļĴ���

    ldr	    r0,  =INTC_REG_VCTR
    ldr		r1,  [r0]
    ldr		r0,  =eGon2_IRQVectorTable
    add		r1,  r0, r1, lsl #1
    ldr		r0,  [r1]					@;/* arg of isr		*/
    mov     lr,  pc
    ldr     pc,  [r1, #4]               @;/* r1���溯��������r1+4���溯����ַ. r1���ݸ���r0��ʵ������r0�����жϺ������� */

    ldmfd   sp!, {r0-r12, lr}                           @; �ָ�SYSTEMģʽ�Ĵ���
    msr     cpsr_c, #(ARMV7_FIQ_MASK | ARMV7_IRQ_MASK | ARMV7_IRQ_MODE)     @; �л���IRQģʽ
    ldmfd   sp!, {r3} 	                                    @; ���ݳ�ջ
    msr     spsr_cxsf, r3                                   @; ��ԭspsr

	ldmfd   sp!, {r0-r12, pc}^    @;���쳣ģʽ����; unknown event ignore


undefined_instruction:
   b  undefined_instruction

software_interrupt:
   b  eGon2_swi_handler
/*------------------------------------------end------------------------------------------*/




