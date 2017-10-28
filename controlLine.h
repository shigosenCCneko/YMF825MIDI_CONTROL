/*
 * controlLine.h
 *
 * Created: 2017/07/06 22:14:14
 *  Author: Keiji
 */ 





#define rd_hi() (PORTD |= 0x40)
#define rd_lo() (PORTD &= ~0x40)
#define wr_hi() (PORTB |= 0x01)
#define wr_lo() (PORTB &= ~(0x01))


#define rs_hi() (PORTC |= 0x40)	//reset YMF262
#define rs_lo() (PORTC &= ~0x40)

#define rs_atmghi() (PORTC |= 0x80)	//reset Atmega328
#define rs_atmglo() (PORTC &= ~0x80)

#define a0_hi sbi _SFR_IO_ADDR(PORTD) ,0
#define a0_lo cbi _SFR_IO_ADDR(PORTD),0

#define a1_hi sbi _SFR_IO_ADDR(PORTD),1
#define a1_lo cbi _SFR_IO_ADDR(PORTD),1

#define cs_hi sbi _SFR_IO_ADDR(PORTD),4
#define cs_lo cbi _SFR_IO_ADDR(PORTD),4










