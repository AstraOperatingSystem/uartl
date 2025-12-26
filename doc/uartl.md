# UARTL

UARTL implements a simple protocol for full duplex packet exchange with connections. There is no error checking, no delivery guarantees but it likely serves as a sufficient base to implement more reliable delivery over.

## Terminology

    The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL
    NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and
    "OPTIONAL" in this document are to be interpreted as described in
    RFC 2119.  

tx - Transmittion, transmit or transmitted  
rx - Receival, receive or received

## Protocol

### Special values

The core of UARTL communication revolves around the ESCape byte `0x8F`. It indicates a special meaning byte comes after. The other special byte values are:

- `0x00` - ACK
- `0x01` - JOIN
- `0x02` - LEAVE
- `0x03` - DATA
- `0x04` - END

For the rest of this document, references ESC and any of the other special bytes should be considered as the mentioned binary values. Furthermore, any pair \<A>+\<B> represents the value A followed by B, in contiguous memory or transmitted sequentially.

### Opening connections

To open a connection, the initiator transmits a ESC+JOIN pair. The connection is considered established only when an ESC+ACK pair is received.
Any other bytes received between ESC+JOIN tx and ESC+ACK rx are to be ignored.

After failing to establish a connection, a system may remain listening on the channel for others to initiate connections. If this is the case, the system MUST listen for ESC+JOIN pairs, and if one is found it MUST be immediately replied to with ESC+ACK.

### Data transmission

Once a connection is established, it can be used to transmit arbitrary binary data. Transmissions MUST be initiated with an ESC+DATA pair, after which binary data can be transferred, with one exception: any ESC bytes MUST be escaped, by duplicating the ESC value. Once the entire packet is sent, an ESC+END pair MUST be sent to signal packet end.

### Closing connections

To close a connection, a system SHOULD transmit an ESC+LEAVE pair, to indicate to any listener that it will be disconnecting.
