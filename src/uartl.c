#include "uartl/uartl.h"

#include <stdint.h>
#include <string.h>

#include "uartl_priv.h"



static inline int _tx1(uartl_handle_t *handle, uint8_t type, int timeout)
{
	uint8_t esc = UARTL_ESC;
	int err = handle->tx(handle->serial, &esc, 1, timeout);
	if (err) return err;
	return handle->tx(handle->serial, &type, 1, timeout);
}

static inline int _txX(uartl_handle_t *handle, uint8_t type, uint8_t *buf, size_t len, int timeout)
{
	int err = _tx1(handle, type, timeout);
	if (err) return err;
	return handle->tx(handle->serial, buf, len, timeout);
}

static inline int _rx1(uartl_handle_t *handle, uint8_t *buf, int timeout)
{
	return handle->rx(handle->serial, buf, 1, timeout);
}

static inline void _append(uartl_handle_t *handle, rx_state_t *state, uint8_t byte)
{
	if (state->nuclear) return;
	if (handle->rx_buff_used == -handle->rx_buf_len ||	//If full
		handle->rx_buff_used > 0)						//or still populated from last message
	{
		state->nuclear = true;
		handle->rx_buff_used = 0;
		return;
	}

	handle->rx_buf[-handle->rx_buff_used] = byte;
	--handle->rx_buff_used;
}

static inline void _reset(uartl_handle_t *handle, rx_state_t *state)
{
	handle->rx_buff_used = 0;
	state->nuclear = false;
}

static inline void _finish(uartl_handle_t *handle, rx_state_t *state)
{
	//Set used to positive when done
	handle->rx_buff_used = -handle->rx_buff_used;
	state->nuclear = false;
}

static inline void state_machine_connected(uartl_handle_t *handle, rx_state_t *state)
{
	uint8_t byte;
	(void)_rx1(handle, &byte, -1); //TODO: Not ignore return
	//REVIEW: Can be made more reliable by limited timeout and rechecking state

	switch (state->stt)
	{
	case RXSTT_LISTEN:
		if (byte == UARTL_ESC) state->stt = RXSTT_INIT; //Ignore all non escape 
		break;

	case RXSTT_INIT:
		if (byte == UARTL_JOIN) (void)_tx1(handle, UARTL_ACK, -1); //TODO: Not ignore return
		else if (byte == UARTL_LEAVE)
		{
			handle->state = UARTL_CONNECTING;
			state->stt = RXSTT_LISTEN;
		}
		else if (byte == UARTL_DATA) state->stt = RXSTT_DATA;
		else state->stt = RXSTT_LISTEN; //Ignore other bytes
		break;

	case RXSTT_DATA:
		if (byte == UARTL_ESC) state->stt = RXSTT_DATA_ESC;
		else _append(handle, state, byte);
		break;

	case RXSTT_DATA_ESC:
		if (byte == UARTL_END)
		{
			_finish(handle, state);
			state->stt = RXSTT_LISTEN;
		}
		else if (byte == UARTL_ESC) _append(handle, state, byte);
		else //Broken
		{
			_reset(handle, state);
			state->stt = RXSTT_LISTEN;
		}
		break;
	}
}

static inline void state_machine_waiting(uartl_handle_t *handle, rx_state_t *state)
{
	uint8_t byte;
	(void)_rx1(handle, &byte, -1); //TODO: Not ignore return

	if (state->stt == RXSTT_LISTEN && byte == UARTL_ESC) state->stt = RXSTT_INIT;
	else if (state->stt == RXSTT_INIT)
	{
		if (byte == UARTL_JOIN)
		{
			_tx1(handle, UARTL_ACK, -1); //TODO: Not ignore return
			handle->state = UARTL_CONNECTED;
		}
		state->stt = RXSTT_LISTEN;
	}
}



int uartl_init_static(uartl_handle_t *handle, void *serial, uartl_tx_f tx, uartl_rx_f rx, void *buf, int buff_len)
{
	handle->serial = serial;
	handle->state = UARTL_DISCONN;
	handle->rx_live = false;

	handle->tx = tx;
	handle->rx = rx;
	handle->rx_buf = buf;
	handle->rx_buf_len = buff_len;

	return UARTL_SUCCESS;
}

int uartl_connect(uartl_handle_t *handle, int timeout)
{
	if (handle->state == UARTL_CONNECTED)
		return UARTL_SUCCESS;

	handle->state = UARTL_CONNECTING;
	(void)_tx1(handle, UARTL_JOIN, timeout); //TODO: Not ignore return value
	return UARTL_SUCCESS;
}

int uartl_disconnect(uartl_handle_t *handle, int timeout)
{
	if (handle->state == UARTL_CONNECTING)
	{
		handle->state = UARTL_DISCONN;
		return UARTL_SUCCESS;
	}
	if (handle->state != UARTL_DISCONN) return UARTL_SUCCESS;

	handle->state = UARTL_LEAVING;
	_tx1(handle, UARTL_LEAVE, timeout);
	handle->state = UARTL_DISCONN;
	return UARTL_SUCCESS;
}

bool uartl_is_connected(uartl_handle_t *handle) { return handle->state == UARTL_CONNECTED; }

int uartl_send(uartl_handle_t *handle, void *buf, size_t len, int timeout)
{
	if (handle->state != UARTL_CONNECTED) return UARTL_ERR_NCONN;

	return _txX(handle, UARTL_DATA, buf, len, timeout);
}

int uartl_receive(uartl_handle_t *handle, void *buf, size_t max_len, int *len)
{//Critical section go brrrrr
	if (handle->rx_buff_used <= 0) //No data or still receiving
		return UARTL_NODATA;

	if (handle->rx_buff_used > (int)max_len) //Data too big
		return UARTL_ERR_TOOBIG;

	memcpy(buf, handle->rx_buf, handle->rx_buff_used);
	if (len)
		*len = handle->rx_buff_used;

	handle->rx_buff_used = 0;
	return UARTL_SUCCESS;
}

void uartl_rx(void *ptr)
{
	uartl_handle_t *handle = ptr;
	rx_state_t state;

	handle->rx_live = true;
	state.stt = RXSTT_LISTEN;
	state.nuclear = false;

	while (1)
	{
		switch (handle->state)
		{
		case UARTL_CONNECTED:
			state_machine_connected(handle, &state);
			break;

		case UARTL_CONNECTING:
			state_machine_waiting(handle, &state);

		case UARTL_DISCONN:
		case UARTL_LEAVING:
			//Do nothing
			//REVIEW: Delay/yield here?
			break;
		}
	}

	//Catch any weird case
	handle->rx_live = false;
	while (1);
}
