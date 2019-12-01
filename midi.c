/* 
 * The software for the x0xb0x is available for use in accordance with the 
 * following open source license (MIT License). For more information about
 * OS licensing, please visit -> http://www.opensource.org/
 *
 * For more information about the x0xb0x project, please visit
 * -> http://www.ladyada.net/make/x0xb0x
 *
 *                                     *****
 * Copyright (c) 2005 Limor Fried
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *                                     *****
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "midi.h"
#include "switch.h"
#include "synth.h"
#include "main.h"
#include "led.h"
#include "dinsync.h"
#include "delay.h"


#define ACCENT_THRESH	100
#define MIDI_Q_SIZE		16

/*
*	Exported module globals 
*/
uint8_t				midi_out_addr;	// store this in EEPROM
uint8_t				midi_in_addr;	// store this in EEPROM, too!

volatile uint8_t	bar_pos; // bar position pointer, from MIDI SPP, in 1/96 steps 
volatile uint8_t	midi_run;

volatile uint8_t	midi_realtime_cmd; // distributes Midi-Start-Stop-Continue to the mode functions 
volatile int8_t		sync_startstop;

// function pointers for the current midi function 
void (*noteOnFunc) (uint8_t,uint8_t) ;
void (*noteOffFunc)(uint8_t ) ;
void (*controllerFunc)(uint8_t,uint8_t) ;
void (*progChangeFunc)(uint8_t) ;


/*
*	Module variables 
*/
static uint8_t			midi_running_status = 0;

static const uint8_t	midion_accent_velocity = 127;
static const uint8_t	midioff_velocity = 32;
static const uint8_t	midion_noaccent_velocity = 100;

static volatile  uint8_t midi_q[MIDI_Q_SIZE];		// cyclic queue for midi msgs
static volatile  uint8_t head_idx = 0;
static volatile  uint8_t tail_idx = 0;

static volatile  uint8_t midi_tx_q[MIDI_Q_SIZE];	// cyclic queue for midi msgs
static volatile  uint8_t head_tx_idx = 0;
static volatile  uint8_t tail_tx_idx = 0;

// own definitions
uint8_t slide;

uint8_t kaccent = 0;
uint8_t maccent = 0;
uint8_t mshift = 0;

extern const uint8_t loopkey_tab[20];
signed int shift = 0;

mva_data mva1;

/*
*	Forward function declarations 
*/

uint8_t	midi_getchar(void);

// interrupt on receive char
#ifdef __AVR_ATmega162__
ISR(USART0_RXC_vect)
#endif
#ifdef __AVR_ATmega2561__
ISR(USART0_RX_vect)
#endif
{
	unsigned char	c = UDR0;

	if(c>=MIDI_CLOCK)
	{
		if(sync==MIDI_SYNC)
		{
			if(c==MIDI_START ) 
			{
				bar_pos=0;
				midi_run=1; 
				sync_startstop=0;
				midi_realtime_cmd=c;
			}
			else if ( c==MIDI_CONTINUE )
			{
				midi_run=1;
				if(IS_SET1(SETTINGS1_CONTINUESTART))
				{
					sync_startstop=0;
					midi_realtime_cmd=MIDI_START;
				}
				else 
					sync_startstop=1; 
			}
			else if(c==MIDI_STOP)
			{
				midi_run=0;
				sync_startstop=0;
				midi_realtime_cmd=c;
			}
			else if(c == MIDI_CLOCK)
			{
				if(midi_run)
					bar_pos++;
				if(bar_pos>=96  )
				{
					bar_pos=0; 
					if(sync_startstop>0)
					{
						sync_startstop=0;
						midi_realtime_cmd=MIDI_START;
					}
					if(sync_startstop<0)
					{
						sync_startstop=0;
						midi_realtime_cmd=MIDI_STOP;
					}
				}
				measureTempo();
				run_tempo+=2;
			}
		}
		return; 
	}

	midi_q[tail_idx++] = c;		// place at end of q
	tail_idx %= MIDI_Q_SIZE;
}

// Midi UART TX interrupt 
ISR(USART0_UDRE_vect)
{
	uint8_t c;

	if(head_tx_idx == tail_tx_idx)
	{
		UCSR0B&=~ (1<<UDRIE0) ; // disable interrupt
	}
	else
	{
		c = midi_tx_q[head_tx_idx++];
		head_tx_idx %= MIDI_Q_SIZE;
		UDR0 = c;
	}
}

uint8_t get_midi_addr(uint8_t eeaddr)
{
	uint8_t midi_addr;

	midi_addr = internal_eeprom_read8(eeaddr);
	if(midi_addr > 15)
		midi_addr = 15;
	return midi_addr;
}

void init_midi(void)
{
	midi_in_addr = get_midi_addr(MIDIIN_ADDR_EEADDR);
	midi_out_addr = get_midi_addr(MIDIOUT_ADDR_EEADDR);
}


void midiFuncNop2(uint8_t a, uint8_t b)
{
	 // move some hot air!
}
void midiFuncNop1(uint8_t a)
{
	// move some hot air!
}


void midi_clr_input(void)
{
	cli(); 
	tail_idx = head_idx;
	sei();
	midi_running_status=0; 
	noteOnFunc=midiFuncNop2; 
	noteOffFunc=midiFuncNop1; 
	controllerFunc=midiFuncNop2; 
	progChangeFunc=midiFuncNop1;
}

void do_midi(void)
{
	uint8_t c; 
	static uint8_t complete, data1; 

	if(head_idx != tail_idx) //   Is there a char in the queue ? 
	{
		// if its a command & either for our address or 0xF,
		// set the midi_running_status
		c = midi_getchar();

		if(c &0x80)
		{		// if the top bit is high, this is a command
			if( c >= 0xF0)				// universal cmd, no addressing
			{	
				midi_running_status = c ; 
			}
			else if((c & 0xF) == midi_in_addr)	//  matches our addr
			{	
				midi_running_status = c &0xf0;
			}
			else
			{
				// not for us, continue!
				midi_running_status = MIDI_IGNORE;
				return; 
			}
			complete=0; 
		}
		else
		{
			if(midi_running_status==MIDI_CHPRESSURE || midi_running_status==MIDI_PROGCHANGE )
			{
				// single byte function
				complete=2; 
			}
			else
			{
				// dual byte functions. 
				if(complete==0)
				{
					data1=c; 
					complete=1; 
				}
				else if(complete==1) 
					complete=2; 
			}
			if(complete==2)
			{
				switch(midi_running_status)
				{
					case MIDI_NOTE_ON:
						if(c) 
						{
							noteOnFunc(data1,c);
							if (function==MIDI_CONTROL_FUNC)
								note_led_handler(data1,c);

							break; 
						}
						// no break! =>  with Velocity 0 it *IS* NOTE OFF 	
					case MIDI_NOTE_OFF:
						noteOffFunc(data1);
						if (function==MIDI_CONTROL_FUNC)
							note_led_handler(data1,0);
						
						break;
					case MIDI_CONTROLLER: 
						controllerFunc(data1,c);
						break; 
					case MIDI_PROGCHANGE:
						progChangeFunc(c); 
						break; 
					case MIDI_SPP:
						bar_pos= (data1 &0xf)*6; // data1 =1/16th notes 
						break; 
					case MIDI_PITCHBEND:
					case MIDI_POLYPRESSURE: 
					case MIDI_CHPRESSURE:
					case MIDI_IGNORE:				// somebody else's data, ignore
					default:
						break;
				}
				complete=0; 
			}
		}
	}
}


void do_midi_mode(void)
{
	// show midi addr on bank leds
	set_bank_leds(midi_in_addr);

	has_bank_knob_changed();	// ignore startup change

	slide = 0;
	mva_reset(&mva1);

	// set proper  functions for do_midi: 
	noteOnFunc=midi_note_on; 
	noteOffFunc=midi_note_off; 

	while(1)
	{
		read_switches();
		if(function != MIDI_CONTROL_FUNC)
		{
			midi_notesoff();	// clear any stuck notes
			return;
		}

		if(has_bank_knob_changed())
		{
			// bank knob was changed, change the midi address
			midi_in_addr = bank;

			// set the new midi address (burn to EEPROM)
			internal_eeprom_write8(MIDIIN_ADDR_EEADDR, midi_in_addr);

			//clear_bank_leds();
			set_bank_leds(midi_in_addr);
		}
		do_midi();
		
		runtime_key_handler();
		runtime_led_handler();
	}
}

void runtime_key_handler(void)
{
	// KEYBOARD KEYS MAPPED TO NOTE MSGS
	for (uint8_t i=0; i<13; i++) {

		// check if any notes were just pressed
		if (just_pressed(loopkey_tab[i])) {
			midi_note_on( ((C2+i) + shift*OCTAVE + 0x19), (kaccent || maccent) ? 127 : 100); 

			// turn on that LED
			set_notekey_led(i);	
		}
		
		// check if any notes were released
		if (just_released(loopkey_tab[i])) {
			midi_note_off( ((C2+i) + shift*OCTAVE + 0x19)); 

			// turn off that LED
			clear_notekey_led(i);
		}
	}

	// TRANSPOSE KEYS
	if (just_pressed(KEY_UP)) {
		if (shift < 2) {
			release_keys();
			shift++;
		}
	} else if (just_pressed(KEY_DOWN)) {
		if (shift > -1)	{
			release_keys();
			shift--;
		}
	} 

	// ACCENT KEY ACTIVE
	if (just_pressed(KEY_ACCENT)) {
		kaccent = 1;
		set_led(LED_ACCENT);
	}

	// ACCENT KEY DEACTIVE
	if (just_released(KEY_ACCENT)) {
		kaccent = 0;
		if (!maccent) clear_led(LED_ACCENT);
	}
}

void release_keys(void)
{
	// LOOP THRU KEYBOARD KEYS
	for (uint8_t i=0; i<13; i++) {

		// check if any notes were just pressed
		if (is_pressed(loopkey_tab[i])) {
			midi_note_off( ((C2+i) + shift*OCTAVE + 0x19)); 

			// turn off that LED
			clear_notekey_led(i);
		}
	}
}

void runtime_led_handler(void)
{
	// SLIDE HANDLING
	if (slide == SLIDE) {
		if (!is_led_set(LED_SLIDE)) set_led(LED_SLIDE);
	} else if (is_led_set(LED_SLIDE)) clear_led(LED_SLIDE);

	// OCTAVE HANDLING
	display_octave_shift(mshift ? mshift : shift);
}

void note_led_handler(uint8_t note, uint8_t velocity)
{
	// VALIDATE PLAYABLE RANGE
	if (!midi_note_ranged(note)) return;

	// NOTE ON
	if (velocity) {

		// turn on that LED
		set_notekey_led(note % 12);

		if (velocity > ACCENT_THRESH) {
			set_led(LED_ACCENT);
			maccent = 1;
		} else {
			if (!kaccent) clear_led(LED_ACCENT);
			maccent = 0;
		}

		mshift = 0;
		if (note <  48) mshift = -1;
		if (note >= 60) mshift = 1;
		if (note >= 72) mshift = 2;

	// NOTE OFF
	} else {

		// turn off that LED
		clear_notekey_led(note % 12);

		if (!kaccent) clear_led(LED_ACCENT);
		maccent = 0;
		mshift = 0;
	}
}


// Monophonic Voice Allocator (with Accent, suitable for the 303)
// "Newest" note-priority rule
// Modified version, allows multiple Notes with the same pitch

void mva_note_on(mva_data *p, uint8_t note, uint8_t accent)
{
	if (accent) { accent = 0x80; }
	uint8_t s = 0;
	uint8_t i = 0;

	// shift all notes back
	uint8_t m = p->n + 1;
	m = (m > MIDI_MVA_SZ ? MIDI_MVA_SZ : m);
	s = m;
	i = m;
	while (i > 0)
	{
		--s;
		p->buf[i] = p->buf[s];
		i = s;
	}
	// put the new note first
	p->buf[0] = note | accent;
	// update the voice counter
	p->n = m;
}

void mva_note_off(mva_data *p, uint8_t note)
{
	uint8_t s = 0;

	// find if the note is actually in the buffer
	uint8_t m = p->n;
	uint8_t i = m;
	while (i) // count backwards (oldest notes first)
	{
		--i;
		if (note == (p->buf[i] & 0x7F))
		{
			// found it!
			if (i < (p->n - 1)) // don't shift if this was the last note..
			{
				// remove it now.. just shift everything after it
				s = i;
				while (i < m)
				{
					++s;
					p->buf[i] = p->buf[s];
					i = s;
				}
			}
			// update the voice counter
			if (m > 0) { p->n = m - 1; }
			break;
		}
	}
}

void mva_reset(mva_data *p) 
{
	p->n = 0;
}

void midi_note_off(uint8_t note) {
	if (mva1.n == 0) return;

	// VALIDATE PLAYABLE RANGE
	note = midi_note_ranged(note);
	if (!note) return;

	mva_note_off(&mva1, note);

	if (mva1.n > 0) {
		uint8_t accent	= mva1.buf[0] & 0x80 ? ACCENT : 0;
		note			= mva1.buf[0] & 0x7F;
		slide			= SLIDE;

		note -= 0x19;
		note |= slide;
		note |= accent;
		note_on(note);

	} else {
		slide = 0;
		note_off(0);
	}
}

void midi_note_on(uint8_t note, uint8_t velocity) {

	if (velocity == 0 && note != 0) {
		// strange midi thing: velocity 0 -> note off!
		midi_note_off(note);
	} else {

		// VALIDATE PLAYABLE RANGE
		note = midi_note_ranged(note);
		if (!note) return;

	    mva_note_on(&mva1, note, velocity > ACCENT_THRESH);

	    uint8_t accent	= mva1.buf[0] & 0x80 ? ACCENT : 0;
	    note			= mva1.buf[0] & 0x7F;
	    slide			= mva1.n > 1 ? SLIDE : 0;

		note -= 0x19;
		note |= slide;
		note |= accent;
		note_on(note);
	}
}

uint8_t midi_note_ranged(uint8_t note) {

	// TT-303 midi play mode (1OCT-)
	if (IS_SET2( SETTINGS2_OCT_DOWN )) note-=OCTAVE;

	// move incomming notes to playable range. 
	// Note 0x19 and 0x20 have the same pitch as 21, 
	// as the buffer OP-Amp can't go that close to ground rail... 

	//	while (note<0x20)		note+=OCTAVE;
	//	while (note>=0x3F+0x19) note-=OCTAVE;

	// removed octave loop,
	// midiplay has a bigger
	// range then pattern_play

	if (note < LOWESTNOTE_MIDIPLAY)		return 0;
	if (note > HIGHESTNOTE_MIDIPLAY)	return 0;

	return note;
}

void midi_send_note_on(uint8_t note)
{

	if((note & 0x3F) == 0)
	{
	/*	Rest 
		Midi does not know about rests
		So nothing to do.  
	*/
	}
	else
	{
		midi_putchar((MIDI_NOTE_ON) | midi_out_addr);
		midi_putchar((note & 0x3F) + 0x19); // note
		if(note & ACCENT )	// if theres an accent, give high velocity
			midi_putchar(midion_accent_velocity);
		else
			midi_putchar(midion_noaccent_velocity);
	}
}

void midi_send_note_off(uint8_t note)
{
	if((note & 0x3F) == 0)
	{
		// Note was a Rest - nothing to do; 
	}
	else
	{
		midi_putchar((MIDI_NOTE_OFF ) | midi_out_addr); // command
		midi_putchar((note & 0x3F) + 0x19);		// note
		midi_putchar(midioff_velocity);			// velocity
	}
}

void midi_putchar(uint8_t c)
{
	cli();
	midi_tx_q[tail_tx_idx++] = c;	// place at end of q
	tail_tx_idx %= MIDI_Q_SIZE;
	UCSR0B |= (1<<UDRIE0) ;			// enable tx interrupt
	sei();
}




uint8_t midi_getchar(void)
{
	char	c;

//	while(head_idx == tail_idx);  <--- that should never loop as *all* calls to this should only be made when there is a char waiting 

	cli();
	c = midi_q[head_idx++];
	head_idx %= MIDI_Q_SIZE;
	sei();

	return c;
}

// sends a midi stop and 'all notes off' message
void midi_stop(void)
{
	midi_putchar(MIDI_STOP);
	midi_notesoff();
}

void midi_notesoff(void)
{
	midi_putchar((MIDI_CONTROLLER) | midi_out_addr);
	midi_putchar(MIDI_ALL_NOTES_OFF);
	midi_putchar(0);
	midi_putchar(MIDI_ALL_SOUND_OFF);
	midi_putchar(0);
}
