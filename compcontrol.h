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


// void do_computer_control(void);
uint8_t calc_CRC8(uint8_t *buff, uint16_t size);
uint8_t input_uint8(void);

void	send_msg(uint8_t *buff, uint16_t len);
void	send_status(uint8_t stat);
void	send_tempo(uint8_t tempo);
void	send_seq_params(void);
void	do_uart_cmd(void);

#define INVALID_MSG			0xff
#define DONE_MSG			0xfe
#define OVERFLOW_MSG		0xfd

#define PING_MSG			0x01
#define PING_MSG_LEN		0
#define STATUS_MSG			0x80
#define STATUS_MSG_LEN		1
#define PATT_MSG			0x19
#define PATT_MSG_LEN		PATT_SIZE

#define WR_PATT_MSG			0x10
#define WR_PATT_MSG_LEN		PATT_SIZE + 2
#define RD_PATT_MSG			0x11
#define RD_PATT_MSG_LEN		2
#define LOAD_PATT_MSG		0x12
#define GET_PATT_MSG		0x13
#define PLAY_PATT_MSG		0x14
#define STOP_PATT_MSG		0x15

#define WR_TRACK_MSG		0x20
#define RD_TRACK_MSG		0x21
#define LOAD_TRACK_MSG		0x22
#define GET_TRACK_MSG		0x23
#define TRACK_MSG			0x29

#define START_SEQ_MSG		0x30
#define STOP_SEQ_MSG		0x31
#define GET_SEQ_MSG			0x32
#define SET_SYNC_MSG		0x33

#define GET_TEMPO_MSG		0x40
#define SET_TEMPO_MSG		0x41
#define TEMPO_MSG			0x42
#define TEMPO_MSG_LEN		2

#define GET_SEQ_PARAMS_MSG	0x43
#define SET_SEQ_PARAMS_MSG	0x44
#define SEQ_PARAMS_MSG		0x45
#define MSG_FW_VER			0x81

#define SEQ_PARAMS_MSG_LEN	5


//
// added c0nb0x debugging stuff:
//

#define MSG_ISSUPP			0x82
#define MSG_DIAG			0x83
#define MSG_TEXTOUT			0x84
#define MSG_GET_MEM			0x85
#define MSG_SET_MEM			0x86
#define MSG_PATBUF			0x87
#define MSG_GET_INFO		0x88

#define XMSG_TEXTOUT

enum TEXTOUT_TYPE
{
    TXTO_STR        = 0, // null-terminated string
    TXTO_UI8        = 1, // uint8_t
    TXTO_SI8        = 2, // int8_t
    TXTO_UI16       = 3, // uint16_t
    TXTO_SI16       = 4, // int16_t
    TXTO_UI32       = 5, // uint32_t
    TXTO_SI32       = 6, // int32_t
    TXTO_BIN        = 7, // <binary data>
};

#ifdef XMSG_TEXTOUT
void textout(char* s);
void textout_bin(uint8_t* data, uint8_t len);
void textout_ui8(uint8_t x);
void textout_ui16(uint16_t x);
void textout_ui32(uint32_t x);
void textout_si8(int8_t x);
void textout_si16(int16_t x);
void textout_si32(int32_t x);
#else
    #define textout(x)
    #define textout_bin(x,y)
    #define textout_ui8(x)
    #define textout_ui16(x)
    #define textout_ui32(x)
    #define textout_si8(x)
    #define textout_si16(x)
    #define textout_si32(x)
#endif//XMSG_TEXTOUT