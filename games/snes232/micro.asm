;
; Copyright (C) 2002 Philip Blundell <philb@gnu.org>
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version
; 2 of the License, or (at your option) any later version.
;
; port B.0  UART tx
;        1  controller latch
;        2  controller clock
;        3  controller 1 data
;        4  controller 2 data
;
; 115200 baud bit time = 32 cycles @ 3.6864MHz
; 60Hz polling         = 61140 cycles
; 12us bit time        ~ 44 cycles 
;
; data format:
;
;    7     6     5     4     3     2     1     0
; | U/D |  C  |  0  |  0  | B:3 | B:2 | B:1 | B:0 |
;
; U/D:   0 = button down, 1 = button up
; C      0 = player 1, 1 = player 2
; B[3:0] button number
; 

.equ SREG	= $3f

.equ TIMSK	= $39

.equ MCUCR	= $35
.equ TCCR0	= $33
.equ TCNT0	= $32

.equ WDTCR	= $21

.equ PORTB	= $18
.equ DDRB	= $17
.equ PINB	= $16

.equ PORTD	= $12
.equ DDRD	= $11

.equ ACSR	= $8

.equ TIMERVAL   = 195

.equ TX_BIT	= 0
.equ LATCH_BIT	= 1
.equ CLK_BIT	= 2
.equ DATA1_BIT	= 3
.equ DATA2_BIT	= 4

.def old1lo	= r8
.def old1hi	= r9
.def old2lo	= r10
.def old2hi	= r11

.def new1lo	= r17
.def new1hi	= r4
.def new2lo	= r18
.def new2hi	= r5

.def diff1lo	= r0
.def diff1hi	= r1
.def diff2lo	= r2
.def diff2hi	= r3

.def this_bit	= r19

.def tmp	= r16

.def bits	= r21

.macro wait12
	nop
	nop
	nop
	nop
	
	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop

	nop
	nop
	nop
	nop
	
	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
.endm

; nominal 22 cycles for 6us.
.macro wait6
	nop
	nop
	nop
	nop
	
	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop
.endm

; wait 23 cycles
.macro wait23
	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
	nop

	nop
	nop
	nop
.endm

	.org	0
	rjmp	reset_vec
	rjmp	int0_vec
	rjmp	timer_vec
	rjmp	comp_vec

reset_vec:
	ldi	tmp, $e7		; set all but 3,4 outputs, high
	out	DDRB, tmp
	ldi	tmp, $ff		; enable pullups on 3, 4
	out	PORTB, tmp

	ldi	tmp, $ff		; set all bits outputs, high
	out	DDRD, tmp
	out	PORTD, tmp

	ldi	tmp, $5			; set prescale by 1024; counter is 3600Hz
	out	TCCR0, tmp

	ldi	tmp, TIMERVAL
	out	TCNT0, tmp

	ldi	this_bit, $50
	rcall	xmit

	wdr
	ldi	tmp, 12			; enable watchdog, 0.25s timeout
	out	WDTCR, tmp

	sbi	ACSR, 7			; power down ADC

	clr	old1lo			; old button status
	clr	old1hi
	clr	old2lo
	clr	old2hi

	ldi	tmp, $80
	out	SREG, tmp		; global interrupt enable
	ldi	tmp, $2
	out	TIMSK, tmp		; enable timer interrupt

	ldi	tmp, $20		; enable sleep
	out	MCUCR, tmp	

main_loop:
	sleep
	wdr
	rjmp	main_loop

int0_vec:
comp_vec:
	rjmp	reset_vec

timer_vec:
	ldi	tmp, TIMERVAL		; reload timer
	out	TCNT0, tmp

	clr	new1lo
	clr	new1hi
	clr	new2lo
	clr	new2hi
	ldi	bits, 16

	sbi	PORTB, LATCH_BIT	; latch high
	wait12
	cbi	PORTB, LATCH_BIT	; latch low
	wait6

read_bit:
	lsl	new1lo
	rol	new1hi
	lsl	new2lo
	rol	new2hi
	wait6
	cbi	PORTB, CLK_BIT		; clock low
	in	tmp, PINB
	sbrc	tmp, DATA1_BIT
	sbr	new1lo, 1
	sbic	PINB, DATA2_BIT
	sbr	new2lo, 1
	wait6
	sbi	PORTB, CLK_BIT		; clock high
	dec	bits
	brne	read_bit

	mov	diff1lo, new1lo		; compute mask of bits that changed
	mov	diff2lo, new2lo
	mov	diff1hi, new1hi
	mov	diff2hi, new2hi

	eor	diff1lo, old1lo
	eor	diff2lo, old2lo
	eor	diff1hi, old1hi
	eor	diff2hi, old2hi

	mov	old1lo, new1lo		; save these bits for next time
	mov	old2lo, new2lo
	mov	old1hi,	new1hi
	mov	old2hi, new2hi

	ldi	bits, 15
check_bit:
	clr	this_bit
	rol	new1lo
	rol	new1hi
	ror	this_bit
	rol	diff1lo
	rol	diff1hi
	brcc	skip1
	or	this_bit, bits
	rcall	xmit
skip1:
	clr	this_bit
	rol	new2lo
	rol	new2hi
	ror	this_bit
	rol	diff2lo
	rol	diff2hi
	brcc	skip2
	or	this_bit, bits
	ori	this_bit, 64
	rcall	xmit
skip2:
	dec	bits
	brvc	check_bit
	reti

	; char to send is in "this_bit"
xmit:
	; send start bit
	cbi	PORTB, TX_BIT

	wait23				
	nop
	nop
	nop
	nop

	ldi	tmp, 8
loop:
	ror	this_bit
	brcc	send_0
	nop
	sbi	PORTB, TX_BIT
	rjmp	send_x
send_0:
	cbi	PORTB, TX_BIT
	nop
	nop
send_x:
	wait23

	dec	tmp
	brne	loop

	nop
	nop
	nop
	nop

	; send stop bit
	sbi	PORTB, TX_BIT

	wait23
	nop

	nop
	nop
	nop
	nop

	nop
	nop

	ret
