/*
 * Managetone.h
 *
 * Created: 2017/10/08 1:05:18
 *  Author: Keiji
 */ 

#include<avr/io.h>

#ifndef MANAGETONE_H_
#define MANAGETONE_H_

#define NOT_GET 255
#define HOLD	128

void init_midich(void);
int get_voice(uint8_t,uint8_t,uint8_t);
int return_voice(uint8_t,uint8_t);


struct VoiceChannel{
	char voice_ch;
	char voice_no;
	char note_no;
	char midi_ch;
	char velocity;
	char hold;
	struct VoiceChannel *next;

};







struct MidiCH{
	char voice_no;
	char hold;
	uint16_t pitchbend;
	char modulation;
	char partlevel;
	char expression;
	char panport;
	char pitch_sens;
	char reg_12;	//0x0c
	char reg_16;	//0x10
	char reg_17;	//0x11
	char reg_18;	//0x12
	char reg_19;	//0x13
	
	struct VoiceChannel *voice_list;
	
};

extern struct MidiCH midi_ch[];
extern struct VoiceChannel  ym825_voice_ch[];

#endif /* MANAGETONE_H_ */