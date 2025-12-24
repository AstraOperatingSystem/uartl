from serial import Serial
from threading import Thread
from typing import Optional
import queue
from queue import Queue
from enum import Enum

class _State(Enum):
		DISCONN = 0
		CONN = 1
		LEAVING = 2
		EXIT = 99

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
		self.remote_connected: bool = False

		self.rx_thread: Thread = Thread(daemon=True, target=self._rcv_loop)
		self.rx_queue: Queue = Queue()

	def connect(self, timeout: float | None = None) -> bool:
		if self.state == _State.CONN: return True
		
		sent = self._send(UARTL.JOIN)
		if not sent: return False

		self._serial.timeout = timeout
		read = self._serial.read(1)

		if len(read) != 1 or read[0] != UARTL.ACK: return False

		self.state = _State.CONN
		self.remote_connected = True
		self.rx_thread.start()
		return True

	def disconnect(self) -> None:
		if self.state != _State.CONN: return
		
		self.state = _State.LEAVING
		self._send(UARTL.LEAVE, timeout=0.5)
		self.state = _State.DISCONN
		self.remote_connected = False

	def send(self, data: bytes, timeout: float | None = None) -> bool:
		if self.state != _State.CONN: return False
		
		return self._send_data(data, timeout)

	def recv(self, timeout) -> Optional[bytes]:
		try:
			return self.rx_queue.get(True, timeout)
		except queue.Empty:
			return None

	def _rcv_loop(self) -> None:
		class STT(Enum):
			LISTEN = 0
			INIT = 1
			DATA = 2
			DATA_ESC = 3

		state: STT = STT.LISTEN
		current = bytes()

		while self.state == _State.CONN:
			buff = self._serial.read(1)
			byte = buff[0]

			if state == STT.LISTEN:
				if byte == UARTL.ESC: state = STT.INIT       # Ignore if not start of packet
			elif state == STT.INIT:
				if byte == UARTL.ESC:   state = STT.LISTEN   # Caught mid data
				elif byte == UARTL.JOIN:                     # ACK joins
					self._send_raw(bytes(UARTL.ACK))
					self.remote_connected = True
					state = STT.LISTEN
				elif byte == UARTL.LEAVE:
					state = STT.LISTEN
					self.remote_connected = False
				elif byte == UARTL.DATA:  state = STT.DATA   # Start data
				elif byte == UARTL.ACK:   state = STT.LISTEN # REVIEW: Should ignore?
				elif byte == UARTL.END:   state = STT.LISTEN # REVIEW: Should ignore?
				else: state = STT.LISTEN                     # Caught mid data
			elif state == STT.DATA:
				if byte == UARTL.ESC: state = STT.DATA_ESC
				else: current += buff
			elif state == STT.DATA_ESC:
				if byte == UARTL.END: # Trasmission complete
					self.rx_queue.put(current)
					current = bytes()
					state = STT.LISTEN
				elif byte == UARTL.ESC:
					current += buff
					state = STT.DATA
				else: # Should be impossible, reset
					current = bytes()
					state = STT.LISTEN

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
