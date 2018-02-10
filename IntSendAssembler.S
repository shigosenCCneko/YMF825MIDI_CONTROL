
/*
 * IntSendAssembler.S
 *
 * Created: 2017/10/15 16:12:15
 *  Author: Keiji
 *
 *
 *	アセンブラ版 if_s_write(uint8_t adr,uint8_t dat)
 *
 *	とりあえずバッファに積んで
 *	SPI送信済み割り込み(SPI_STC_vect)で処理	
 *	受信バッファのサイズを256byteにして256byteの境界に配置
 *	アドレス計算を下位8bitだけにしてアドレス計算と
 *	カウンタの溢れ処理を簡略化
 *
 *	burst writeやwaitを入れる時は　flush_spi_buff()を呼び
 *	バッファが空になるまで待つ
 *
 */
 
#include <avr/io.h>
#include <avr/interrupt.h>


#define ZERO  r1
;.extern	send_buf_byte
;.extern	spi_send_read
;.extern	spi_send_write


.global SPI_STC_vect
.global if_s_write	
.global flush_spi_buff

.global spi_sendData
.global send_buf_byte

.section .data

spi_send_write:	.dc.b	0
spi_send_read:	.dc.b	0
send_buf_byte:	.dc.b	0
.extern send_buf_byte

.section .bss
.balign 256	;バッファを256byteの境界域に確保
spi_sendData:	.ds.b	256

.section .text
.func SPI_STC_vect
SPI_STC_vect:
	cli

	push r0
	in r0, _SFR_IO_ADDR(SREG) 
	push r0

	push r24
	push r30
	push r31

	lds r24,send_buf_byte
	subi r24,1
	sts send_buf_byte,r24
	tst r24
	breq NO_DATA

	sbrs r24,0
	rjmp FIRST_BYTE

	lds r30,spi_send_read
	ldi r31,hi8(spi_sendData)
	ld r24,Z
	out _SFR_IO_ADDR(SPDR),r24
	inc r30
	sts spi_send_read,r30

	pop r31
	pop r30
	pop r24
	pop r0
	out _SFR_IO_ADDR(SREG),r0
	pop r0
	sei
	reti

NO_DATA:	
	sbi 0x5,0

	pop r31
	pop r30
	pop r24
	pop r0
	out _SFR_IO_ADDR(SREG),r0
	pop r0
	sei
	reti

FIRST_BYTE:	
	sbi 0x5,0				;SSをHI

	lds r30,spi_send_read
	cbi 0x5,0					;1 SPI clock 以上空けてSSをLOにする
	ldi  r31,hi8(spi_sendData)

	ld r24,Z
	out _SFR_IO_ADDR(SPDR),r24

	inc r30
	sts spi_send_read,r30

	pop r31
	pop r30
	pop r24
	pop r0
	out _SFR_IO_ADDR(SREG),r0
	pop r0
	sei
	reti

.endfunc



//
//  if_s_write(uint8_t adr,uint8_t dat)
//
.global if_s_write	
.func
if_s_write:

	cli
	
	lds r25,send_buf_byte
	lds r30,spi_send_write
	
	cpse r25,ZERO
	rjmp ON_SEND		//送信中

	ldi	r31,hi8(spi_sendData)
	st Z,r22
	inc r30
	sts	spi_send_write,r30
	ldi r25,2
	sts send_buf_byte,r25
	cbi 0x5,0
	out _SFR_IO_ADDR(SPDR),r24
//cbi _SFR_IO_ADDR(PORTC),7

	sei
	ret


ON_SEND:				//送信中、バファに積むだけ
	ldi r31,hi8(spi_sendData)
	st Z,r24
	inc r30
	st Z,r22
	inc r30
	sts spi_send_write,r30

	subi r25,(-2)
	sts  send_buf_byte,r25

/*ldi r24,4
sub r24,r25
ori r24,0x40
out _SFR_IO_ADDR(PORTC),r24*/

	sei
	ret			


.endfunc

/*
 *	void flush_spi_buff(void)
 *
 *	送信バッファが空になるまでループ
 */

.func
flush_spi_buff:

loop1:
	lds r24,send_buf_byte
	tst r24
	brne loop1
	ret
.endfunc
