from serial import Serial
from threading import Thread
from typing import Optional
import queue
from queue import Queue
from enum import Enum
from time import sleep

class _State(Enum):
	DISCONN = 0
	CONNECTING = 1
	CONNECTED = 2
	LEAVING = 3

class _RXSTT(Enum):
	LISTEN = 0
	INIT = 1
	DATA = 2
	DATA_ESC = 3

class UARTL:
	ESC = 0x8F # Non ASCII
	ACK = 0
	JOIN = 1
	LEAVE = 2
	DATA = 3
	END = 4

	def __init__(self, serial: Serial) -> None:
		self._serial: Serial = serial

		self.state: _State = _State.DISCONN

		self._rx_thread: Thread = Thread(daemon=True, target=self._rcv_loop)
		self._rx_queue: Queue = Queue()
		self._rx_thread.start()

	def connect(self, timeout: float | None = None) -> bool:
		if self.state == _State.CONNECTED: return True
		
		self.state = _State.CONNECTING
		sent = self._send(UARTL.JOIN, timeout=timeout)
		return sent

	def disconnect(self) -> None:
		if self.state != _State.CONNECTED: return
		
		self.state = _State.LEAVING
		self._send(UARTL.LEAVE, timeout=0.5)
		self.state = _State.DISCONN

	def send(self, data: bytes, timeout: float | None = None) -> bool:
		if self.state != _State.CONNECTED: return False
		
		return self._send_data(data, timeout)

	def recv(self, timeout) -> Optional[bytes]:
		if self.state != _State.CONNECTED:
			return None

		try:
			return self._rx_queue.get(True, timeout)
		except queue.Empty:
			return None

	def _rcv_loop(self) -> None:
		self._rx_state: _RXSTT = _RXSTT.LISTEN
		self._rx_current = bytes()

		while True:
			if self.state == _State.CONNECTED:
				self._recv_state_machine_conn()
			elif self.state == _State.CONNECTING:
				self._recv_state_machine_wait()
			else:
				sleep(0.1) # Do nothing
			
	def _recv_state_machine_conn(self) -> None:
		buff = self._serial.read(1)
		byte = buff[0]

		if self._rx_state == _RXSTT.LISTEN:
			if byte == UARTL.ESC:   	self._rx_state = _RXSTT.INIT   	# Ignore if not start of packet
		elif self._rx_state == _RXSTT.INIT:
			if byte == UARTL.ESC:   	self._rx_state = _RXSTT.LISTEN 	# Caught mid data
			elif byte == UARTL.JOIN:                                   	# ACK joins
				self._send(UARTL.ACK)
				self._rx_state = _RXSTT.LISTEN
			elif byte == UARTL.LEAVE:									# Handle disconnects
				self.state = _State.CONNECTING
				self._rx_state = _RXSTT.LISTEN
			elif byte == UARTL.DATA: 	self._rx_state = _RXSTT.DATA   	# Start data
			elif byte == UARTL.ACK:  	self._rx_state = _RXSTT.LISTEN 	# REVIEW: Should ignore?
			elif byte == UARTL.END:  	self._rx_state = _RXSTT.LISTEN 	# REVIEW: Should ignore?
			else: self._rx_state = _RXSTT.LISTEN                       	# Caught mid data
		elif self._rx_state == _RXSTT.DATA:
			if byte == UARTL.ESC: 		self._rx_state = _RXSTT.DATA_ESC
			else: self._rx_current += buff
		elif self._rx_state == _RXSTT.DATA_ESC:
			if byte == UARTL.END: # Trasmission complete
				self._rx_queue.put(self._rx_current)
				self._rx_current = bytes()
				self._rx_state = _RXSTT.LISTEN
			elif byte == UARTL.ESC:
				self._rx_current += buff
				self._rx_state = _RXSTT.DATA
			else: # Should be impossible, reset
				self._rx_current = bytes()
				self._rx_state = _RXSTT.LISTEN

	def _recv_state_machine_wait(self) -> None:
		buff = self._serial.read(1)
		byte = buff[0]

		if self._rx_state == _RXSTT.LISTEN and byte == UARTL.ESC:
			self._rx_state = _RXSTT.INIT
		elif self._rx_state == _RXSTT.INIT:
			if byte == UARTL.JOIN:
				self._send(UARTL.ACK)			# ACK joins
				self.state = _State.CONNECTED
			self._rx_state = _RXSTT.LISTEN

	def _send_data(self, data: bytes = b"", timeout: float | None = None) -> bool:
		return self._send(UARTL.DATA, data + bytes([UARTL.ESC, UARTL.END]), timeout)
	
	def _send(self, ptype: int, data: bytes = b"", timeout: float | None = None) -> bool:
		return self._send_raw(bytes([UARTL.ESC, ptype]) + data, timeout)
	
	def _send_raw(self, data: bytes, timeout: float | None = None) -> bool:
		self._serial.write_timeout = timeout
		written = self._serial.write(data)
		if not written or written != len(data): return False

		self._serial.flush()
		return True
