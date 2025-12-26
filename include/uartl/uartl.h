#ifndef _UARTL_H_
#define _UARTL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define UARTL_ERR_TOOBIG -3
#define UARTL_ERR_NCONN -2
#define UARTL_ERR_UNSCPEC -1
#define UARTL_SUCCESS 0
#define UARTL_NODATA 1

//TODO: Document interface
//Return err
typedef int (*uartl_tx_f)(void *serial, void *buf, size_t len, int timeout);
typedef int (*uartl_rx_f)(void *serial, void *buf, size_t len, int timeout);

typedef enum uartl_state {
	UARTL_DISCONN,
	UARTL_CONNECTING,
	UARTL_CONNECTED,
	UARTL_LEAVING,
} uartl_state_e;

typedef struct uartl_handle
{
	/// @brief UART handle.
	void *serial;
	/// @brief Connection state.
	uartl_state_e state;
	/// @brief True if rx thread is alive.
	bool rx_live;

	/// @brief UART transmit function.
	uartl_tx_f tx;
	/// @brief UART receive function.
	uartl_rx_f rx;

	/// @brief Receive buffer.
	uint8_t *rx_buf;
	/// @brief Receive buffer length.
	int rx_buf_len;
	/// @brief Space in receive buffer used. Negative if being populated.
	volatile int rx_buff_used;
} uartl_handle_t;


/// @brief Initializes a uartl handle.
/// @param handle Pointer to the handle store location.
/// @param serial Pointer to a serial handle. Will be passed to tx.
// REVIEW: tx should be blocking?
/// @param tx UART transmit function. Should be blocking.
/// @param rx UART receive function. Should be blocking.
/// @param buff Buffer for temporary storage. Should be big enough to hold the largest message allowed.
/// @param buff_len The length of the buffer.
/// @return int Zero on success, error code otherwise.
/// @remark Call should be closely followed by a thread/task start with uartl_rx and handle as the argument.
int uartl_init_static(uartl_handle_t *handle, void *serial, uartl_tx_f tx, uartl_rx_f rx, void *buf, int buff_len);

/// @brief Connects to uartl or begins listen mode.
/// @param handle UARTL handle.
/// @param timeout Timeout for the rx/tx operations.
//TODO: Document return values
/// @return 
int uartl_connect(uartl_handle_t *handle, int timeout);

/// @brief Disconnects from uartl.
/// @param handle UARTL handle.
/// @param timeout Timeout for the rx/tx operations.
//TODO: Document return values
/// @return 
int uartl_disconnect(uartl_handle_t *handle, int timeout);

/// @brief Checks whether a given UARTL instance is connected.
/// @param handle The handle to check.
/// @return True if connected, false otherwise.
bool uartl_is_connected(uartl_handle_t *handle);

/// @brief Sends a byte array over UARTL.
/// @param handle UARTL handle.
/// @param buf Data to send.
/// @param len Length of data to send.
/// @param timeout Timeout for rx/tx operations.
//TODO: Document return values
/// @return 
int uartl_send(uartl_handle_t *handle, void *buf, size_t len, int timeout);

/// @brief Fetches a packet from the receive buffer.
/// @param handle UARTL handle.
/// @param buf Buffer to write to.
/// @param max_len Size of the buffer.
/// @param len Pointer to store actual bytes read.
//TODO: Document return values
/// @return 
int uartl_receive(uartl_handle_t *handle, void *buf, size_t max_len, int *len);

/// @brief Receiver task/thread for uartl.
/// @param handle UARTL handle returned by uartl_init functions.
void uartl_rx(void *handle);

#endif
