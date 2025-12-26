#ifndef _UARTL_PRIV_H_
#define _UARTL_PRIV_H_

#include <stdint.h>

#define UARTL_ESC (uint8_t)0x8F
#define UARTL_ACK (uint8_t)0x00
#define UARTL_JOIN (uint8_t)0x01
#define UARTL_LEAVE (uint8_t)0x02
#define UARTL_DATA (uint8_t)0x03
#define UARTL_END (uint8_t)0x04

typedef enum rx_stt
{
	RXSTT_LISTEN,
	RXSTT_INIT,
	RXSTT_DATA,
	RXSTT_DATA_ESC
} rx_stt_e;

typedef struct rx_state
{
	rx_stt_e stt;
	bool nuclear;
} rx_state_t;

#endif
