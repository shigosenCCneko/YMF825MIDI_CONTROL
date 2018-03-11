/*
 * Managetone.c
 *
 * Created: 2017/10/07 23:28:28
 *  Author: Keiji
 */ 


/*

MidiCHにVoiceをリストで繋ぐ


MidiCH[0]->voice[1]->voice[5]->voice[2]->NULL
MidiCH[1]->NULL
MidiCH[2]->voice_[4]->NULL
MidiCH[3]->NULL
.
.
.

*/

#include <avr/pgmspace.h>
#include "Managetone.h"
#include <avr/io.h>
#define MAX_VOICE_NUM	16
#define FALSE 0
#define TRUE  1
#define NULL 0

struct VoiceChannel ym825_voice_ch[16];
	
struct MidiCH midi_ch[16];
extern uint8_t play_mode;
extern char tone_reg[];

extern char  modulation_depth[16];	//modulation の振れ幅　[0-63]  TLと計算テーブルを共有するため
extern char  modulation_pitch[16];	//modulation の周期
extern char  modulation_cnt[16];    //modulation の減算カウンタ
extern char  modulation_tblpointer[16]; //sin テーブルのポインタ

char  sin_pointer[16];
char  sin_pitch[16];
char  sin_tbl_offs[16];
uint8_t voice_queue[MAX_VOICE_NUM];
uint8_t voice_queue_top;
uint8_t voice_queue_tail;



int		active_voice_num;
char channel_noteno[16];

uint8_t career_no[8][4] = { {1,0,0,0},
							{0,1,0,0},
								
							{0,1,2,3},
							{3,0,0,0},
							{3,0,0,0},
							{1,3,0,0},
							{0,3,0,0},
							{0,2,3,0}	};
								
uint8_t career_val[8] = {1,2,4,2,1,2,2,3};
uint8_t rel_optval[16];

/*
char div12mod[128]  = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
	0x17,0x18,0x19,0x1a,0x1b,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,
	0x2b,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x40,0x41,0x42,
	0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x50,0x51,0x52,0x53,0x54,0x55,0x56,
	0x57,0x58,0x59,0x5a,0x5b,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,
	0x6b,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x80,0x81,0x82,
	0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x90,0x91,0x92,0x93,0x94,0x95
};

*/

/*
int p_freq[]   = {
	0x0165,	0x017A,	0x0191,	0x01A9,	0x01c2,	0x01DD,	0x01F9,0x0217,	0x0237,	0x0259,0x027D,	0x02A2,	
	
};
*/
/* 古典調律 */
/*
int p_freq[] ={
	360,380,406,427,451,481,507,541,570,601,641,676,721,
};
*/
/* 古典調律 */
/*
const char fnum_hi_tbl[128]  = {
0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x18,0x18,
0x18,0x18,0x18,0x20,0x20,0x20,0x28,0x28,0x11,0x11,0x19,0x19,0x19,0x19,0x19,0x21,
0x21,0x21,0x29,0x29,0x12,0x12,0x1a,0x1a,0x1a,0x1a,0x1a,0x22,0x22,0x22,0x2a,0x2a,
0x13,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,0x23,0x23,0x23,0x2b,0x2b,0x14,0x14,0x1c,0x1c,
0x1c,0x1c,0x1c,0x24,0x24,0x24,0x2c,0x2c,0x15,0x15,0x1d,0x1d,0x1d,0x1d,0x1d,0x25,
0x25,0x25,0x2d,0x2d,0x16,0x16,0x1e,0x1e,0x1e,0x1e,0x1e,0x26,0x26,0x26,0x2e,0x2e,
0x17,0x17,0x1f,0x1f,0x1f,0x1f,0x1f,0x27,0x27,0x27,0x2f,0x2f,0x10,0x10,0x18,0x18,
0x18,0x18,0x18,0x20,0x20,0x20,0x28,0x28,0x11,0x11,0x19,0x19,0x19,0x19,0x10,0x26,

const char fnum_hi_tbl[128]  = {
*/
/* 古典調律 */
/*
const char fnum_lo_tbl[128]  = {
0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x7c,0x16,0x2b,
0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,
0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,
0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,
0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,
0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,
0x68,0x7c,0x16,0x2b,0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,
0x43,0x61,0x7b,0x1d,0x3a,0x59,0x01,0x24,0x68,0x7c,0x16,0x2b,0x43,0x61,0x68,0x3a,

};
*/

const char fnum_hi_tbl[128]  = {
0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x18,0x18,
0x18,0x18,0x18,0x20,0x20,0x20,0x20,0x28,0x11,0x11,0x19,0x19,0x19,0x19,0x19,0x21,
0x21,0x21,0x21,0x29,0x12,0x12,0x1a,0x1a,0x1a,0x1a,0x1a,0x22,0x22,0x22,0x22,0x2a,
0x13,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,0x23,0x23,0x23,0x23,0x2b,0x14,0x14,0x1c,0x1c,
0x1c,0x1c,0x1c,0x24,0x24,0x24,0x24,0x2c,0x15,0x15,0x1d,0x1d,0x1d,0x1d,0x1d,0x25,
0x25,0x25,0x25,0x2d,0x16,0x16,0x1e,0x1e,0x1e,0x1e,0x1e,0x26,0x26,0x26,0x26,0x2e,
0x17,0x17,0x1f,0x1f,0x1f,0x1f,0x1f,0x27,0x27,0x27,0x27,0x2f,0x10,0x10,0x18,0x18,
0x18,0x18,0x18,0x20,0x20,0x20,0x20,0x28,0x11,0x11,0x19,0x19,0x19,0x19,0x10,0x1e,

};
const char fnum_lo_tbl[128]  = {
0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x65,0x7a,0x11,0x29,
0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,
0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,
0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,
0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,
0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,
0x65,0x7a,0x11,0x29,0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,
0x42,0x5d,0x79,0x17,0x37,0x59,0x7d,0x22,0x65,0x7a,0x11,0x29,0x42,0x5d,0x65,0x5d,	
};



void init_midich(void){
	int i;
	
	voice_queue_top = 0;
	voice_queue_tail = 0;
	active_voice_num = 0;
	
	
	
	for(i =0;i < MAX_VOICE_NUM; i++){
		ym825_voice_ch[i].voice_ch = i;		//voice channel no
		ym825_voice_ch[i].voice_no =i;		//voice no
		ym825_voice_ch[i].midi_ch = 127;	//midi channel no
		ym825_voice_ch[i].note_no = 255;	//midi の音番号
		ym825_voice_ch[i].velocity = 32;	
		ym825_voice_ch[i].next = NULL;
		ym825_voice_ch[i].release_cnt = 0;
		voice_queue[i] = i;				//キューにvoiceチャンネル番号

	}
	for(i = 0;i < 16;i++){
		midi_ch[i].voice_no = i;		//初期状態は MIDI_CH == voice_no
		midi_ch[i].hold	 = FALSE;
		midi_ch[i].pitchbend = 0x4000;
		midi_ch[i].modulation = 0;
		midi_ch[i].partlevel = 25;
		midi_ch[i].expression = 25;
		midi_ch[i].panport = 0;
		midi_ch[i].pitch_sens = 12;
		midi_ch[i].voice_list = NULL;	//使用しているvoice_chのリスト
		/*-------------------------------note on　時に一緒に書きこむ情報 */
		midi_ch[i].reg_16 = 0x30;	//ch_vol,expression
		midi_ch[i].reg_17 = 0x00;	//vib
		midi_ch[i].reg_18 = 0x08;	//pitchbend_hi
		midi_ch[i].reg_19 = 0x00;	//pitchbend_lo
	}
}



/*
	queueから利用可能な最も古く開放されたvoiceチャンネル番号を返す
	可能なvoiceチャンネルが無い場合はNOT_GETを返す
*/
int get_voice(uint8_t ch,uint8_t tone_no,uint8_t velo){
	int voice_ch;
	struct VoiceChannel *p;
	
	//voice_ch = NOT_GET;
	p = midi_ch[ch].voice_list;
	while(p != NULL){
		if(p->note_no == tone_no){
			//if(p->hold == TRUE){

				voice_ch =  p->voice_ch;
				mute(voice_ch);			
				//break;
				return voice_ch;
			//}
		}
		
		p = p->next;
	 }
	//if(voice_ch != NOT_GET){
		//return voice_ch;
	//}

	
	if(active_voice_num == MAX_VOICE_NUM){
		voice_ch = voice_queue_top;
		note_off(voice_ch);
		return voice_ch;		
		//return NOT_GET;
	}
	voice_ch = voice_queue[voice_queue_top++];
	if(voice_queue_top == MAX_VOICE_NUM){
		voice_queue_top = 0;
	}
		active_voice_num++;
		

	
	ym825_voice_ch[voice_ch].voice_no = midi_ch[ch].voice_no;
	ym825_voice_ch[voice_ch].note_no = tone_no;
	ym825_voice_ch[voice_ch].midi_ch = ch;
	ym825_voice_ch[voice_ch].velocity = velo;
	ym825_voice_ch[voice_ch].next = midi_ch[ch].voice_list;
	ym825_voice_ch[voice_ch].hold = FALSE;
	midi_ch[ch].voice_list = &(ym825_voice_ch[voice_ch]);

	return voice_ch;
}


/*
	queueにvoice_ch番号を戻す

*/
int  return_voice(uint8_t ch,uint8_t tone_no){
	struct VoiceChannel **p;
	int voice_ch;
	//int i;

	
	p = &(midi_ch[ch].voice_list);
	//voice_ch = NOT_GET;
	
	while( *p != NULL){
	
		if( (*p)->note_no == tone_no){
			if( midi_ch[ch].hold == TRUE){
				(*p)->hold = TRUE;		//holdならフラグを立ててキューへ戻さない
				//break;
				return NOT_GET;
			}
			voice_ch = (*p)->voice_ch;	
			*p = ((*p)->next);
			
			voice_queue[voice_queue_tail++] = voice_ch;
			if(voice_queue_tail == MAX_VOICE_NUM){
				voice_queue_tail = 0;
			}
			active_voice_num--;
			//break;
			return voice_ch;
		}
		p = &( (*p)->next);
	}
	//return voice_ch;
	return NOT_GET;	
}



void hold_on(ch){

	midi_ch[ch].hold = TRUE;
	
	
}


/*
 ホールド中のvoice_chをキューへ戻す
 
 */


void hold_off(ch){
	struct VoiceChannel **p;
	int voice_ch;
	
	midi_ch[ch].hold = FALSE;
	
	p = &(midi_ch[ch].voice_list);
	
	while( *p != NULL){
		if( (*p)->hold == TRUE){
			voice_ch = (*p)->voice_ch;	
			note_off(voice_ch);
			
			*p = ((*p)->next);
			
			voice_queue[voice_queue_tail++] = voice_ch;
			if(voice_queue_tail == MAX_VOICE_NUM){
				voice_queue_tail = 0;
			}
			active_voice_num--;		
			
			
		}else{
			p = &( (*p)->next);
		}
	}
	
}



void mute(uint8_t ch){
	if_s_write(0x0b,ch);
	//if_s_write(0x0F, ym825_voice_ch[ch].voice_no+0x60);
	if_s_write(0x0F, ym825_voice_ch[ch].voice_no + 0x20);
}




void note_on(uint8_t midich,uint8_t voicech,uint8_t i,uint8_t j,uint8_t voice_no){
	
	
	uint8_t alg,k,l,m;
	uint16_t voice_top_addr;
	channel_noteno[voicech] = i;			//monoモード消音用
	//modulation_tblpointer[(int)voicech] = 0; // or 0x1f	
	//modulation_cnt[voicech] = modulation_pitch[voicech];	 
	//sin_pointer[voicech] = 0;
	j = j & 0x7C;  //  (j >>2 ) << 2);


	//キャリアの最も小さいリリースレートを求める K
	//get algorithm
	//voice_top_addr = 30 * voice_no;
	//alg = tone_reg[voice_top_addr + 1] & 0x07;	//get algorithm no
	//l = career_val[alg];
	//
	//k = 255;
	//while(l != 0){
		//l--;
		//m = career_no[alg][l];
		//m = tone_reg[voice_top_addr + m * 7 + 3]  & 0xf0; // get release late * 16
		//if(m<k)
			//k = m;
			//
	//}
	//ym825_voice_ch[voicech].release_cnt = ((240-k)>>5);
	ym825_voice_ch[voicech].release_cnt = rel_optval[voice_no];

	
	
	
	if_s_write(0x0b,voicech);
	if_s_write(0x0C,j);  // #12
	
	
	if_s_write(0x0D,fnum_hi_tbl[i]);
	if_s_write(0x0E,fnum_lo_tbl[i]);
	
	if_s_write(0x10,midi_ch[midich].reg_16);
	if_s_write(0x11,midi_ch[midich].reg_17);
	if_s_write(0x12,midi_ch[midich].reg_18);
	if_s_write(0x13,midi_ch[midich].reg_19);


	if_s_write(0x0f,0x40|voice_no);	

	
}

void optimize_queue(){
	uint8_t min,v,i,j,k,p,pre;
	
	if(active_voice_num < MAX_VOICE_NUM - 1) {
		
		p = voice_queue_tail;
		if(p == 0){p = MAX_VOICE_NUM;}
		p--;
		min = ym825_voice_ch[voice_queue[p]].release_cnt;

		while(p != voice_queue_top){
			pre = p;
			if(p == 0){
				p = MAX_VOICE_NUM;
			}
			p--;
			
			k = ym825_voice_ch[voice_queue[p]].release_cnt;
			if(k !=0)
				k--;
			ym825_voice_ch[voice_queue[p]].release_cnt = k;
			if(k < min){
				min = k;
			}else{
				// swap
				k = voice_queue[p];
				voice_queue[p] = voice_queue[pre];
				voice_queue[pre] = k;
				/* no operation */
				//k = voice_queue[p];
				//voice_queue[p] = voice_queue[pre];
				//voice_queue[p] = k;
				
			}

		}
	}
	
}
void note_off(char voice_ch){
	//setChannel(ch);
	if_s_write(0x0b,voice_ch);
	if_s_write(0x0F, ym825_voice_ch[voice_ch].voice_no);
	
	
	
	
}



void note_off_func(int ch,char i){
		int f,adr;

		if(play_mode == 1){
			f = return_voice(ch,i);
			if( f!= NOT_GET){
				note_off(f);
			}
			
		}else if(play_mode == 3){
			if(ch <8){
				f = return_voice(ch,i);
				if(f!=NOT_GET){
					note_off(f);
				}
				f = return_voice(ch+8,i);
				if(f!=NOT_GET){
					note_off(f);
				}
			}
		}else{
			if(channel_noteno[ch] == i){
				note_off(ch);
			}
		}
	
}



void change_modulation(uint8_t ch,uint8_t mod){
	struct VoiceChannel *p;
	uint8_t voice_ch;
	
	midi_ch[ch].reg_17 = mod;
	p = midi_ch[ch].voice_list;
	while(p != NULL){
		if_s_write(0x0b,p->voice_ch);
		if_s_write(0x11,mod);
		p = p->next;
	}
}

void change_pitchbend(uint8_t ch,uint8_t i,uint8_t j){
	int f;
	int hi,lo;
	struct VoiceChannel *p;
	
	f = (i <<7) | j;
	f = calc_exp(f,midi_ch[ch].pitch_sens);
	hi = (f>>11) & 0x001f;
	lo = (f >>4) & 0x007f;
	midi_ch[ch].reg_18 = hi;
	midi_ch[ch].reg_19 = lo;
	
	p = midi_ch[ch].voice_list;	
	while(p != NULL){
		if_s_write(0x0b,p->voice_ch);
		if_s_write(0x12,hi);
		if_s_write(0x13,lo);
		p = p->next;
	}
}

void change_part_level(uint8_t ch, uint8_t val){
	struct VoiceChannel *p;
	
	midi_ch[ch].reg_16 = val;
	p = midi_ch[ch].voice_list;
	while(p != NULL){
		if_s_write(0x0b,p->voice_ch);
		if_s_write(0x10,val);
		p = p->next;
	}
}

void change_expression(uint8_t ch, uint8_t val){
	struct VoiceChannel *p;
	
	midi_ch[ch].reg_16 = val;
	p = midi_ch[ch].voice_list;
	while(p != NULL){
		if_s_write(0x0b,p->voice_ch);
		if_s_write(0x10,val);
		p = p->next;
	}
}


/* 単音用PitchBend */

	

void pitch_wheel_change(char ch,char i ,char j){
	int f,d;

	
	f = (i << 7) | j;

	f = calc_exp(f,midi_ch[ch].pitch_sens);

	//f = calc_exp(f,11);	

	d = (f >> 11) & 0x001f;



	
	if_s_write(0x0B,ch);
	if_s_write(0x12,d);
		
	d = (f >> 4) & 0x007f;
	if_s_write(0x13,d);

	
}	

	