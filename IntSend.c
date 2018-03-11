/*
 * IntSend.c
 *
 * Created: 2017/06/28 23:19:56
 *  Author: Keiji
 */ 
// USART.H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "controlLine.h"
#include "Managetone.h"



void write_burst(void);
void flush_spi_buff(void);


//volatile uint8_t spi_sendData[256];    // spiで送信するデータ。ring buffer
//volatile uint8_t spi_send_write=0;        // 現在のwrite位置(spi_sendDataのindex)
//volatile uint8_t spi_send_read=0;         // 現在のread位置(spi_sendDataのindex)
//volatile uint8_t send_buf_byte = 0;
extern volatile uint8_t send_buf_byte;
extern char tone_reg[480];
extern char career_no[8][4];
extern char career_val[8];

/*
// SPI送信完了割り込み
ISR(SPI_STC_vect)
{	
	
	cli();
	//send_buf_byte--;
	if ((--send_buf_byte) != 0){
		
		if(send_buf_byte & 0x01){
			// second byte of single write 
			SPDR = spi_sendData[spi_send_read++];
	
		}else{
			// first byte of single write (adr,dat)
			wr_hi();
			wr_lo();
			SPDR = spi_sendData[spi_send_read++];
		}
	}else{
			// end of buffer 
		wr_hi();		
	}
	
	sei();	
}
*/
/*
// single write 割り込み用にバッファに積む

void if_s_write2(uint8_t adr,uint8_t dat)
{
	cli();
	
	if(send_buf_byte== 0){
										
			spi_sendData[spi_send_write++] = dat;
			send_buf_byte = 2;

			wr_lo();
			//spi_stat = 1;
			SPDR = adr;		
		
	}else{
		
			spi_sendData[spi_send_write++] = adr;
			spi_sendData[spi_send_write++] = dat;
			send_buf_byte += 2;
		
	}
	
	if(send_buf_byte > 4){
		PORTC |= 0x80;
	}else{
		PORTC &= 0x7f;
	}
	sei();	

}

*/

//-------------------上のコードはアセンブラで書き直した------------
void send_atmega(char c){
	
	SPDR = c;
	while( !(SPSR & (1<< SPIF)));
	c =  SPDR;

}

void write_burst(){
	int voice_top_addr;
	char k,l,m,alg;
	int i;
	char *tone;
	tone = tone_reg;
	

	if_s_write(0x08,0x16);

	if_s_write(0x08,0x00);
	flush_spi_buff();
	//while(send_buf_byte>0);
	
	wr_lo();
	cli();
	send_atmega(0x07);
	send_atmega(0x90);	

	for(i = 0;i<480;i++){
		
		send_atmega(*tone);
		tone++;

	}
	send_atmega(0x80);
	send_atmega(0x03);
	send_atmega(0x81);
	send_atmega(0x80);

	wr_hi();
	sei();
	voice_top_addr = 0;
	for(i = 0;i < 16;i++){
		alg = tone_reg[voice_top_addr + 1] & 0x07;	//get algorithm no
		l = career_val[alg];
	
		k = 255;
		while(l != 0){
			l--;
			m = career_no[alg][l];
			m = tone_reg[voice_top_addr + m * 7 + 3]  & 0xf0; // get release late * 16
			if(m<k)
			k = m;
		
		}
		rel_optval[i] = ((240-k)>>6);
		voice_top_addr += 30;
	}


}

