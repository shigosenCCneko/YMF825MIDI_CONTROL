/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Dual MIDI demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */
#include <avr/pgmspace.h>
#include <avr/eeprom.h>



#define DATA_LEN_825	30

/* offsets for SBI params */
#define AM_VIB		0
#define KSL_LEVEL	2
#define ATTACK_DECAY	4
#define SUSTAIN_RELEASE	6
#define WAVE_SELECT	8

/* offset for SBI instrument */
#define CONNECTION	10
#define OFFSET_4OP	11
#define MAX_MUL	8


#define FOSC 16000000UL
//#define BAUD FOSC 
//#define UBRR_DAT (FOSC/(16*BAUD)-1)
void usart_spi_init(void);
void usart_spi_send(unsigned int data);






//#define MAX_3MUL 3

#include "wave.h"
#include "divtable.h"
#include "DualMIDI.h"
//#include "sintbl.h"
#include "controlLine.h"
#include "Managetone.h"


char tone_reg[480];


char MAX_3MUL = 8;
int eeprom_no=0;				//Selected channel
int eeprom_p_midi = 0;			//eeprom read pointer;
char channelVal;				// Channel value



//char  modulation_depth[16];	//modulation の振れ幅　[0-63]  TLと計算テーブルを共有するため
//char  modulation_pitch[16];	//modulation の周期
//char  modulation_cnt[16];    //modulation の減算カウンタ
//char  modulation_tblpointer[16]; //sin テーブルのポインタ
uint8_t rpn_msb[16];
uint8_t rpn_lsb[16];

char  sin_pointer[16];
char  sin_pitch[16];
char  sin_tbl_offs[16];


uint8_t play_mode = 1;			// state of channel 1 [mono,mul,hum]
uint8_t hid_res_mode = 0;
uint16_t flash_squ = 0;


//char mul2_top_channel;
char bank = 0;
char sysex_mes[65];
int sysx_mes_pos = 0;

char send_cnt = 0;


void mem_reset(void);
void setChannelDefault();


void if_s_write(uint8_t addr,uint8_t data);

void note_off(char);
void note_on(uint8_t,uint8_t ,uint8_t ,uint8_t ,uint8_t);

void pitch_wheel_change(char ,char  ,char );
void startup_sound(void);



extern uint16_t calc_exp(uint16_t,uint8_t);


void note_off_func(int,char);
void set_timer_intrupt(void);
void reset_ymf825(void);

void check_midimessage();

enum {Atck,Decy,Sus,Rel,Mul,Tlv,Ksl,Wave,Am,Vib,Egt,Ksr,FeedBk,Connect,
	D1R,D2R,D1L,DT1,DT2};
	





/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint          =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};



/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
//static uint8_t PrevHIDReportBuffer[GENERIC_REPORT_SIZE];



/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
//USB_ClassInfo_HID_Device_t Generic_HID_Interface =
	//{
		//.Config =
			//{
				//.InterfaceNumber              = INTERFACE_ID_GenericHID,
				//.ReportINEndpoint             =
					//{
						//.Address              = GENERIC_IN_EPADDR,
						//.Size                 = GENERIC_EPSIZE,
						//.Banks                = 1,
					//},
				//.PrevReportINBuffer           = PrevHIDReportBuffer,
				//.PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
			//},
	//};





/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	
		//CLKPR = 0x80;
		//CLKPR = 0;
		DDRB = 0xf7;
		DDRC = 0xff;
		DDRD = 0xfb;
		DDRF = 0xff;
		char i,j,l; 
		
		char k,m;
		char a,c;
			
		int f;
		int ch;
		int adr;
		char Data[4];
		//int oadr;
	SetupHardware();
	

   
	
	GlobalInterruptEnable();


MIDI_EventPacket_t ReceivedMIDIEvent;

	for (;;)
	{
		

		
		while (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
			
			
			
			
			if((ReceivedMIDIEvent.Event == MIDI_EVENT(1,MIDI_COMMAND_SYSEX_START_3BYTE))){
				//えくるしーぶメッセージ
				// message
				// 0xf0  channel, operator No, Type, value 0xf7
			
				if(ReceivedMIDIEvent.Data1 == 0xf0){
					sysx_mes_pos = 0;
				}
				
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;
				
			}else if((ReceivedMIDIEvent.Event == MIDI_EVENT(1,MIDI_COMMAND_SYSEX_3BYTE))){
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;				
				
									
			}else 	if((ReceivedMIDIEvent.Event == MIDI_EVENT(1,MIDI_COMMAND_SYSEX_END_1BYTE)
			||  ReceivedMIDIEvent.Event == MIDI_EVENT(1,MIDI_COMMAND_SYSEX_END_2BYTE)
			||  ReceivedMIDIEvent.Event == MIDI_EVENT(1,MIDI_COMMAND_SYSEX_END_3BYTE))){
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
				sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;

			Data[0] = ((sysex_mes[1] & 0x0f)<<4) | (sysex_mes[2] & 0x0f);
			Data[1] = ((sysex_mes[3] & 0x0f)<<4) | (sysex_mes[4] & 0x0f);
			Data[2] = ((sysex_mes[5] & 0x0f)<<4) | (sysex_mes[6] & 0x0f);			
			Data[3] = ((sysex_mes[7] & 0x0f)<<4) | (sysex_mes[8] & 0x0f);
			
			if(sysex_mes[0] == 0xf0){
			
				switch(Data[0]){
	
				case 0:  // Reset
			
					switch(Data[3]){
						case 0:
							reset_ymf825();
							bank = 1;
												
							mem_reset();
							break;
						default:
							break;
					}
					break;
			
				case 1:		// Write Data to Register
			
					if(Data[1]==2){
						if_s_write(Data[2],Data[3]);
					}
					break;
				case 2:		// set pori mode midi channel 1 and 4
			
						if(Data[1]== 0){
							play_mode = 0;
					
							setChannelDefault();
							mem_reset();
				
						}else if(Data[1] ==1){
							play_mode = 1;
							for(a = 0;a<16;a++){
								midi_ch[a].voice_no = a;
							
							}
							setChannelDefault();
							mem_reset();				
													
						}else if(Data[1] == 3){
							play_mode = 3;
							MAX_3MUL = 8;
							setChannelDefault();
							mem_reset();
						}
					break;
				case 3: // 4or2 operation select  Data[1]:channel  Data[2]: 2 or 4
					break;
					
				case 4: // algorithm select  num [0-3]4op [0-1]2op
					ch = (int)Data[1];
					c = Data[2];
					break;
			
				case 5: // set tone data  EEPROM YMF262 ;
					ch = (int)Data[1];
					f = (int)Data[2];
					c = Data[3];
					ch = ch << 5;
					f = f + ch;
			
					eeprom_busy_wait();
					eeprom_write_byte((uint8_t *)f,c);
					break;
					
				case 6:	//ready read tone data EEPROM
					send_cnt = 30;
					eeprom_p_midi = Data[1] << 5;
					break;
				
				case 9: // burst write tone data
					write_burst();
					break;	
					
				case 10: //write tone array and write burst
					tone_reg[(Data[1]*30)+Data[2]] = Data[3];
					write_burst();
					break;	
					
				case 11: //write tone array only
					tone_reg[(Data[1]*30+Data[2])] = Data[3];
					break;
		
				default:
					break;
				}

			}
			
		}
			
			
		
			//if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && (ReceivedMIDIEvent.Data3 > 0)){
			if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON))){ 
// ノートオン
			    ch = ReceivedMIDIEvent.Data1 & 0x0f;
			  	i = ReceivedMIDIEvent.Data2;
			  	j = ReceivedMIDIEvent.Data3;
				if(j == 0){
					note_off_func(ch,i);
					//break;
				}else {
				
					
						if(play_mode == 1){
							
							f = get_voice(ch,i,j);
							if(f != NOT_GET){
								note_on(ch,f,i,j,midi_ch[ch].voice_no);
							}
						}else if(play_mode == 3){
							f = get_voice(ch,i,j);
							if(f != NOT_GET){							
								note_on(ch,f,i,j,midi_ch[ch].voice_no);
							}
							//_delay_ms(1);	//可変にしたら面白そう
							//_delay_us(200);
							f = get_voice(ch+8,i,j);
							if(f != NOT_GET){						
								note_on(ch+8,f,i,j,midi_ch[ch+8].voice_no);
							}
						}else{
							note_on(ch,ch,i,j,midi_ch[ch].voice_no);
						}
				}
			}
			
			if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF))){
				//ノートオフ
				ch = ReceivedMIDIEvent.Data1 & 0x0f;
				i = ReceivedMIDIEvent.Data2;
				note_off_func(ch,i);
	
			}
			
	
	
	
	
	
	
				if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_START_3BYTE))){
					//えくるしーぶメッセージ
			
					// 0xf0        0xf7
					
					if(ReceivedMIDIEvent.Data1 == 0xf0){
						sysx_mes_pos = 0;
					}
					
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;
					
					}else if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_3BYTE))){
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;
					
					
				}else 	if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_1BYTE)
				||  ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_2BYTE)
				||  ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_3BYTE))){
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data1;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data2;
					sysex_mes[sysx_mes_pos++] = ReceivedMIDIEvent.Data3;
				
					
					if((sysex_mes[0] == 0xf0)&& (sysex_mes[1] == 0x43) && (sysex_mes[2] == 0x7f) &&
						(sysex_mes[3] == 0x02) && (sysex_mes[4] == 0x00) && (sysex_mes[5]==0x00)){
							
						adr = (sysex_mes[6] & 0x0f) * 30; //tone_reg top adr of tone no
						
						a = sysex_mes[7];	//voice common
						tone_reg[adr+0] = (a & 0x60) >> 5;
						tone_reg[adr+1] = (a & 0x18) << 3;
						tone_reg[adr+1] |= (a & 0x07);
						c = 8;
						for(j =0;j<4;j++){
							f = j * 7 + adr;
							a = sysex_mes[c++];				//key control
							tone_reg[f+8] = (a & 0x70) >> 4;	//8 first
							tone_reg[f+2] = (a & 0x08);		//2 first
							tone_reg[f+2] |= (a & 0x04) >> 2;	
							tone_reg[f+5] = (a & 0x03);		//5 first
							
							a = sysex_mes[c++];				//Attck Rate
							tone_reg[f+4] = (a & 0x0f) << 4;	//4 first
							
							a = sysex_mes[c++];				//Decay Rate
							tone_reg[f+3] = (a & 0x0f);		//3 first
							
							a = sysex_mes[c++];				//Sustain Rate
							tone_reg[f+2] |= (a & 0x0f) <<4;
							
							a =  sysex_mes[c++];			//Release Rate
							tone_reg[f+3] |= (a & 0x0f) <<4;
							
							a =  sysex_mes[c++];			//Sustain Level
							tone_reg[f+4] |= (a & 0x0f);	
							
							a =  sysex_mes[c++];			//Total Level
							tone_reg[f+5] |= (a & 0x3f) << 2;
							
							a =  sysex_mes[c++];			//Modulation
							tone_reg[f+6] = a;				//6 first
							
							a =  sysex_mes[c++];			//Pitch
							tone_reg[f+7] = (a & 0x0f)<<4;	//7 first
							tone_reg[f+7] |= (a & 0x70) >> 4;
							
							a =  sysex_mes[c++];			//Wave Shape
							tone_reg[f+8] |= (a & 0x1f)<<3;
						
							
							
						}
						write_burst();

					}
					
				}
				
				
				
	
	
	
	
	
	
	
	
	
			
			//if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_START_3BYTE))){		
				////えくるしーぶメッセージ
				//// message 
				//// 0xf0  dat1,dat2,dat3,dat4 0xf7
				//i = ReceivedMIDIEvent.Data1;
				//if(i == 0xf0){
					//sysex_mes[0] = i;
					//sysex_mes[1] = ReceivedMIDIEvent.Data2;
					//sysex_mes[2] = ReceivedMIDIEvent.Data3;
				//}
			//}
			//
			//
//
			//if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_1BYTE)
			//||  ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_2BYTE)
			//||  ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_SYSEX_END_3BYTE))){
				//sysex_mes[3] = ReceivedMIDIEvent.Data1;
				//sysex_mes[4] = ReceivedMIDIEvent.Data2;
				//sysex_mes[5] = ReceivedMIDIEvent.Data3;
				//ch = sysex_mes[1];
				//i = sysex_mes[2];
				//j = sysex_mes[3];
				//k = sysex_mes[4];
				//
				//if(sysex_mes[0] == 0xf0){
				//
					//
					//switch(j){ //Type
						//case Atck:
						//
						//break;
						//
						//case Decy:
						//
						//break;
						//
						//case Sus:
											//
						//break;
						//
						//case Rel:
						//
						//break;
						//
						//case Mul:
						//
						//break;
						//
						//case Tlv:
						//
						//break;
						//
						//case Ksl:
						//
						//break;
						//
						 //
					//}
				//}
			//
			//}
			
			
			/* プログラムチェンジ */
			
			
			if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_PROGRAM_CHANGE))){
				ch = ReceivedMIDIEvent.Data1 & 0x0f;
				j = ReceivedMIDIEvent.Data2;	//プログラムナンバー
				//f = j << 5;  //  f = j * 32;
				l = j & 0x0f;
				
				if(bank == 1){
					midi_ch[ch].voice_no = ch;				
					/* Yamaha midi 音源のwavedata読み込み */
					f = j * 30;
					adr = ch * 30;

					for(i = 0;i < 30;i++){
						tone_reg[adr+i] = pgm_read_byte(&(wave825[(int)i+f]));
					}
					write_burst();

				}else if(bank == 2){
					j  = j & 0x1f;
					f = j << 5;		//*32
					adr = ch * 30;
					for(i = 0;i < 30;i++){
						eeprom_busy_wait();
						tone_reg[adr+i] = eeprom_read_byte((uint8_t *)(f+i));
					}
					write_burst();
					
				}else{
					//channelVoiceNo[ch] = l;
					midi_ch[ch].voice_no = l;

				}
								
			}
			
			/* ピッチホイールチェンジ　*/
			
			if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_PITCH_WHEEL_CHANGE))){
				ch = ReceivedMIDIEvent.Data1 & 0x0f;
				j = ReceivedMIDIEvent.Data2;
				i = ReceivedMIDIEvent.Data3;
				

				
				if(play_mode == 1){
						change_pitchbend(ch,i,j);
									
				}else if(play_mode == 3){
						change_pitchbend(ch,i,j);
						change_pitchbend(ch+8,i,j);
				}else{
				pitch_wheel_change(ch, i , j);
				
				}
			}
			

			
			
			/* コントロールチェンジ メッセージ　*/
			if((ReceivedMIDIEvent.Event == MIDI_EVENT(0,MIDI_COMMAND_CONTROL_CHANGE))){
				ch = ReceivedMIDIEvent.Data1 & 0x0f;
				i = ReceivedMIDIEvent.Data2;   //control No
				j = ReceivedMIDIEvent.Data3;	//value
				
				
				if(i == 101){	// RPN MSB
					rpn_msb[ch] = j;
				}
				if(i == 100){	//RPN LSB
					rpn_lsb[ch] = j;
				}
				if(i == 6){
					if((rpn_msb[ch]==0) && (rpn_lsb[ch]==0)){
						midi_ch[ch].pitch_sens = j;
					}
				}
				
	
				
				if(i == 60){
					for(l = 0;l<channelVal;l++){
						sin_pitch[l] = j;
					}
				}
				if(i == 61){
					for(l = 0;l<channelVal;l++){
						sin_tbl_offs[(int)l] = (j<<5) & 0xe0;
						
					}
				}
				if(i == 31){ //modulation sin table pitch
					sin_pitch[ch] = j ;
					if( play_mode == 1   ){
						sin_pitch[ch+1] = j;
						sin_pitch[ch+2] = j;
					}
				}
				
				
				if(i == 32){ //modulation sin table pitch
					sin_tbl_offs[ch] = (j<<5) & 0xe0;
					if( play_mode == 1   ){
						sin_tbl_offs[ch+1] = (j<<5) & 0xe0;
						sin_tbl_offs[ch+2] = (j<<5) & 0xe0;
					}
				}
				
				
				if(i == 121){	//リセットオールコントローラ
				
					setChannelDefault();
					
				}

				if(i == 76){  // モジュレーションpitch
				//j = j >>1;
					//modulation_pitch[ch] = j ;
					//modulation_cnt[ch] = modulation_pitch[ch];
					if( play_mode == 1  ){	
					
							//modulation_pitch[ch+1] = j;
							//modulation_cnt[ch+1] = modulation_pitch[ch+1];
							//modulation_pitch[ch+2] = j;
							//modulation_cnt[ch+2] = modulation_pitch[ch+2];
			
					}
				
				
				}

				if(i == 1){  //モジュレーション　(depth)
					j = j >>4;
					//modulation_depth[ch] = j;

					//modulation_cnt[ch] = modulation_pitch[ch];
					if( play_mode == 1  ){	
						change_modulation(ch,j);	
							
																					
					}else if(play_mode == 3){
						change_modulation(ch,j);
						change_modulation(ch+8,j);			

					}else{
						if_s_write(0x0B,ch);
						if_s_write(0x11,j);			
					}
				}
				if(i == 64){			//hold
					if(j < 64){
						// hold off
						hold_off(ch);
					}else{
						//hold on
						hold_on(ch);
					}
				}
				
				if(i == 10){				// パン

				}else if(i == 7){			// チャンネルボリューム
					m = j>> 2;
					//channelPartLevel[ch] = m;
					m = m >> 1;
					midi_ch[ch].partlevel = m;				

					//m = pgm_read_byte(&(divtbl[(int)m][ channelExpression[(int)ch]]));
					m = pgm_read_byte(&(divtbl[(int)m][(int) midi_ch[(int)ch].expression]));
					k = m << 2;
			
					if( play_mode == 1  ){

						change_part_level(ch,k);
						
					}else if(play_mode == 3){
						change_part_level(ch,k);
						change_part_level(ch+8,k);
					}else{
						if_s_write(0x0B,ch);
						if_s_write(0x10,k);						
						
					}

					
				}else if(i == 0){					//バンクセレクト
					//j = j & 0x01;
					//j = j ^ 0x01;
					bank = j & 0x03;
					
				}else  if(i == 11){			// エクスプレッション
		
					m = (j >> 2);
					//channelExpression[ch] = m;
					midi_ch[ch].expression = m;							
					//m = pgm_read_byte(&(divtbl[(int)m][channelPartLevel[(int)ch]]));
					m = pgm_read_byte(&(divtbl[(int)m][(int)midi_ch[(int)ch].partlevel]));
					k = m << 2;
										
					if( play_mode == 1 ){
						change_expression(ch,k);
					}else{
						if_s_write(0x0B,ch);
						if_s_write(0x10,k);						
		
					}
				}
			}
		}
		check_midimessage(); // 送信データチェック
		
		//HID_Device_USBTask(&Generic_HID_Interface);
		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}
					
/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	int i,ch;
	//MCUCR = 0x80;
	//MCUCR = 0x80;
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	
	CLKPR = 0x80;
	CLKPR = 0x00;
	

	

	USB_Init();
	
_delay_ms(50);
	usart_spi_init();
_delay_ms(501);
	reset_ymf825();
_delay_ms(100);
	startup_sound();
	
	setChannelDefault();
}
void keyon(unsigned char fnumh, unsigned char fnuml){
	if_s_write( 0x0B, 0x00 );//voice num
	if_s_write( 0x0C, 0x54 );//vovol
	if_s_write( 0x0D, fnumh );//fnum
	if_s_write( 0x0E, fnuml );//fnum
	if_s_write( 0x0F, 0x40 );//keyon = 1
	//PORTC ^= 0x80;
}

void keyoff(void){
	if_s_write( 0x0F, 0x00 );//keyon = 0
}
/** Checks for changes in the position of the board joystick, sending MIDI events to the host upon each change. */

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{

}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);
	//ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);
	USB_Device_EnableSOFEvents();

	
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
	//HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	//HID_Device_MillisecondElapsed(&Generic_HID_Interface);
}




/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
//bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         //uint8_t* const ReportID,
                                         //const uint8_t ReportType,
                                         //void* ReportData,
                                         //uint16_t* const ReportSize)
//{
	//int i;
	//uint8_t* Data        = (uint8_t*)ReportData;
	//
	//if(hid_res_mode == 6){
		//for(i = 0;i<4;i++){
//
			//eeprom_busy_wait();
			//Data[i] = eeprom_read_byte( (uint8_t*)(eeprom_no ++));
			//
		//}
	//
	//}
//
	//*ReportSize = GENERIC_REPORT_SIZE;
	//return false;
//}


/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
//void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          //const uint8_t ReportID,
                                          //const uint8_t ReportType,
                                          //const void* ReportData,
                                          //const uint16_t ReportSize)
//{
	//uint8_t* Data       = (uint8_t*)ReportData;
	//int a,ch,adr,adr_hi,adr_lo;
	//char c;
	//switch(Data[0]){
	//
		//case 0:  // Reset
			//
			//switch(Data[3]){
				//case 0:
					//reset_ymf825();
					//bank = 1;
					//mem_reset();
					//break;
				//case 1:
					//break;
				//default:
					//break;
			//}
			//break;
			//
		//case 1:		// Write Data to Register
			//
			//if(Data[1]==2){
				//if_s_write(Data[2],Data[3]);
			//}
			//break;
		//case 2:		// set pori mode midi channel 1 and 4
			//
				//if(Data[1]== 0){
					//play_mode = 0;
					//
					//setChannelDefault();
					//mem_reset();
				//
				//}else if(Data[1] ==1){
					//play_mode = 1;
					//for(a = 0;a<16;a++){
						//midi_ch[a].voice_no = a;
							//
					//}
					//setChannelDefault();
					//mem_reset();				
									//
				//}else if(Data[1] == 2){
												//
				//}else if(Data[1] == 3){
					//play_mode = 3;
					//MAX_3MUL = 8;
					//setChannelDefault();
					//mem_reset();
				//}
			//break;
			//
		//case 3: // 4or2 operation select  Data[1]:channel  Data[2]: 2 or 4
			//break;
			//
		//case 4: // algorithm select  num [0-3]4op [0-1]2op
			//ch = (int)Data[1];
			//c = Data[2];
			//break;
			//
		//case 5: // set tone data  EEPROM YMF262 ;
				//ch = (int)Data[1];
				//a = (int)Data[2];
				//c = Data[3];
				//ch = ch << 5;
				//a = a + ch;
			//
				//eeprom_busy_wait();
				//eeprom_write_byte((uint8_t *)a,c);
			//
			//break;
		//case 6:	//ready read tone data EEPROM
			//hid_res_mode = 6;
			//send_cnt = 30;
			//eeprom_no = Data[1] << 5;
			//break;
			//
		//case 7: // set tone data EEPROM YM2151
			//break;	
						//
		//case 8: // read data from EEPROM Atmega328
			//break;
			//
		//case 9: // burst write tone data
			//write_burst();
			//break;	
			//
		//case 10: //write tone array and write burst
			//tone_reg[(Data[1]*30)+Data[2]] = Data[3];
			//write_burst();
			//break;	
			//
		//case 11: //write tone array only
			//tone_reg[(Data[1]*30+Data[2])] = Data[3];
			//break;
		//
		//default:
			//break;
		//
		//}
			//
//}



void setChannelDefault(){
	char reg;
	int i,j,val;
	val = 16;
	
	for(i = 0;i<16;i++){
		//rpn_msb[i] = 0;
		//rpn_lsb[i] = 0;
		
		rpn_msb[i] = 127;
		rpn_lsb[i] = 127;
	
		//modulation_depth[i] = 20;
		///modulation_pitch[i] = 21;	//modulation の周期
		//modulation_cnt[i] = 0;    //modulation の減算カウンタ
		//modulation_tblpointer[i] = 0;
		sin_pointer[i] = 0;
		sin_pitch[i] = 3;
		sin_tbl_offs[i] = (7<< 5);
	}

	init_midich();
	channelVal = val;

}






void mem_reset(void){
	init_midich();
}


void set_timer_intrupt(void){
	OCR1A = 0x18;
	//OCR1A = 0x06;
	TCCR1A  = 0b00000000;
	TIMSK1 = (1<< OCIE1A); //compare match A interrupt
	
	TCCR1B  = (1 << WGM12);
	//TCCR1B |= (1 << CS12)|(1 << CS10);  // CTC mode top = OCR1A
	
	TCCR1B |= (1 << CS12);  // CTC mode top = OCR1A
	
	
	sei();
}

//#define USE_C_INTRUPT

#ifdef USE_C_INTRUPT
ISR(TIMER1_COMPA_vect){
	
	uint8_t i,c,d;
	signed int f;
	int adr;
	
	for(i = 0;i < channelVal;i++){
		
			c = modulation_cnt[i];	

			if(c != 0){
				c--;
				if(c == 0){
					modulation_cnt[i] = modulation_pitch[i];
					c = modulation_tblpointer[i];
					c++;
					c &= 0x01f;
					modulation_tblpointer[i] = c;
					d = modulation_depth[i];
					f = pgm_read_word(&(sin_tbl[(int)d][(int)c]));

				}else{
					modulation_cnt[i] = c;
				}
		
		}
	
	}

}
#endif


void reset_ymf825(){
	int i;
	rs_hi();
	_delay_ms(100);
	rs_lo();
	_delay_ms(10);

	rs_hi();

	wr_hi();
wr_hi();	
	_delay_ms(100);
		
	if_s_write(0x1D,0);	//5V	
	if_s_write( 0x02, 0x0e );
	 _delay_ms(1);
	if_s_write( 0x00, 0x01 );//CLKEN
	if_s_write( 0x01, 0x00 ); //AKRST
	if_s_write( 0x1A, 0xA3 );
	 _delay_ms(1);
	if_s_write( 0x1A, 0x00 );
	 _delay_ms(30);
	if_s_write( 0x02, 0x00 );
	 //add
	if_s_write( 0x19, 0xcc );//MASTER VOL
	if_s_write( 0x1B, 0x3F );//interpolation
if_s_write(0x1b,0x00);
	

	if_s_write( 0x14, 0x00 );//interpolation
	//if_s_write( 0x03, 0x03 );//Analog Gain
	if_s_write( 0x03, 0x01 );//Analog Gain 
	if_s_write( 0x08, 0xF6 );
		//if_s_write( 0x08, 0xFf );
	flush_spi_buff();
	 _delay_ms(21);
	if_s_write( 0x08, 0x00 );
	if_s_write( 0x09, 0xF8 );

	if_s_write( 0x0A, 0x00 );
	 
	if_s_write( 0x17, 0x40 );//MS_S
	if_s_write( 0x18, 0x00 );	


	if_s_write(0x20,0x0f);
	flush_spi_buff();
	_delay_us(10);
	
	if_s_write(0x21,0x0f);
	flush_spi_buff();
	_delay_us(10);

	if_s_write(0x22,0x0f);
	flush_spi_buff();	
	_delay_us(10);

		
	for(i= 0;i <16;i++){
		if_s_write(0x0b,i);			//固定値は先に書きこんでおく。
		if_s_write(0x14,0);
		tone_reg[(30*i)] = 0x01;	//BO
		tone_reg[(30*i)+1] =0x83;	//LFO,ALG
		
		tone_reg[(30*i)+2] = 0x00;  //0:sr,lfo,ksr
		tone_reg[(30*i)+3] = 0x7F;	//1:RR,DR
		tone_reg[(30*i)+4] = 0xF4;	//2:ar,sl
		tone_reg[(30*i)+5] = 0xBB;	//3:TL,KSL
		tone_reg[(30*i)+6] = 0x00;	//4:DAM,EAM,DVB,EVB
		tone_reg[(30*i)+7] = 0x10;	//5:MUL,DT
		tone_reg[(30*i)+8] = 0x40;	//6:WS,FB
		
		tone_reg[(30*i)+9] = 0x00;
		tone_reg[(30*i)+10] = 0xAF;
		tone_reg[(30*i)+11] = 0xA0;
		tone_reg[(30*i)+12] = 0x0E;
		tone_reg[(30*i)+13] = 0x03;
		tone_reg[(30*i)+14] = 0x10;
		tone_reg[(30*i)+15] = 0x40;
		
		tone_reg[(30*i)+16] = 0x00;
		tone_reg[(30*i)+17] = 0x2F;
		tone_reg[(30*i)+18] = 0xF3;
		tone_reg[(30*i)+19] = 0x9B;
		tone_reg[(30*i)+20] = 0x00;
		tone_reg[(30*i)+21] = 0x20;
		tone_reg[(30*i)+22] = 0x41;
		
		tone_reg[(30*i)+23] = 0x00;  //sr,xof,ks4r
//tone_reg[(30*i)+24] = 0xAF;
		tone_reg[(30*i)+24] = 0x7F;
		tone_reg[(30*i)+25] = 0xA0;
		tone_reg[(30*i)+26] = 0x0E;
		tone_reg[(30*i)+27] = 0x01;
		tone_reg[(30*i)+28] = 0x10;
		tone_reg[(30*i)+29] = 0x40;
		
	}
	
	write_burst();
	


	
}

void startup_sound(void){
	_delay_ms(50);
	keyon(0x1c,0x11);
	_delay_ms(30);
	keyoff();
	_delay_ms(10);
	
	keyon(0x1c,0x42);
	_delay_ms(30);
	keyoff();
	_delay_ms(10);
	
	keyon(0x1c,0x5d);
	_delay_ms(30);
	keyoff();
	_delay_ms(10);
	
	keyon(0x24,0x17);
	_delay_ms(30);
	keyoff();
	_delay_ms(10);
	
		
}

void usart_spi_init(){
	
	char c;

	//UBRR1 = 0;
	//UCSR1C = (1<<UMSEL11)|(1<<UMSEL10)|(0<< UCSZ10)|(0<<UCPOL1); //UCSZ10 = UCPHA1
	//UCSR1C = 0xc0;
	//UCSR1B = (1<<RXEN1)|(1<<TXEN1);
	//UCSR1B = (1<<RXEN1) | (1<<TXEN1) |  (1<<UDRIE1);
	//UBRR1 = UBRR_DAT;
	//UBRR1 = 40;
		SPCR = (1<<SPE)|(1<<MSTR)|(0<<SPR0)|(0<<SPR1)|(1<< SPIE);
//		SPCR = (1<<SPE)|(1<<MSTR)|(0<<SPR0)|(0<<SPR1);
		SPSR = (1<< SPI2X);
		c = SPDR;
}
void usart_spi_send(unsigned int data){
	while( !(UCSR1A & (1<< UDRE1)));
	UDR1 = data;
	//while( !(UCSR1A & (1<<RXC1)));
	data = UDR1;
}


void check_midimessage(){
	MIDI_EventPacket_t MIDIEvent;
	uint8_t MIDICommand;

	uint8_t dat1,dat2;
	if(send_cnt == 0)
		return;
	

	send_cnt = 30;	
	MIDICommand = MIDI_COMMAND_SYSEX_START_3BYTE;
	dat1 = eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));	
	MIDIEvent = (MIDI_EventPacket_t)
	{
		.Event       = MIDI_EVENT(1, MIDICommand),

		.Data1       = 0xF0,
		.Data2       = (dat1 >> 4),
		.Data3       = dat1 & 0x0f,
	};
	send_cnt--;
	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);
	
	do{

		dat1 = eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));	
		dat2= eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));
	
		MIDICommand = MIDI_COMMAND_SYSEX_3BYTE;

		MIDIEvent = (MIDI_EventPacket_t)
		{
			.Event       = MIDI_EVENT(1, MIDICommand),

			.Data1       = (dat1 >> 4),
			.Data2       = dat1 & 0x0f,
			.Data3       = (dat2 >> 4),
		};
		MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);

		dat1 = eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));	
		MIDIEvent = (MIDI_EventPacket_t)
		{
			.Event       = MIDI_EVENT(1, MIDICommand),

			.Data1       = dat2 & 0x0f,
			.Data2       = (dat1 >>4),
			.Data3       = dat1 & 0x0f,
		};
		MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);
			
		send_cnt = send_cnt -3;
	}while(send_cnt > 2);
	send_cnt = 0;

	dat1 = eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));	
	dat2= eeprom_read_byte( (uint8_t*)(eeprom_p_midi ++));
	
	MIDIEvent = (MIDI_EventPacket_t)
	{
		.Event       = MIDI_EVENT(1, MIDICommand),

		.Data1       = (dat1 >> 4),
		.Data2       = dat1 & 0x0f,
		.Data3       = (dat2 >> 4),
	};
	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);
	
	MIDICommand = MIDI_COMMAND_SYSEX_END_3BYTE;
	MIDIEvent = (MIDI_EventPacket_t)
	{
		.Event       = MIDI_EVENT(1, MIDICommand),

		.Data1       = dat2 & 0x0f,
		.Data2       = 0,
		.Data3       = 0xf7,
	};
	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);
	MIDI_Device_Flush(&Keyboard_MIDI_Interface);

	//PORTC ^= 0x80;


}


