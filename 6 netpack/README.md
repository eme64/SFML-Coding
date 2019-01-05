### Idea:
networking library:

L0: connection: client-id, flow control?
Socket: rcv/send. queuing, send notification (trigger next send?)

L1: lossy vs. reliable (no ordering, but seq number still)
SocketProtocol:
  lossy: seq num, but can duplicate or get lost
  reliable: seq num, eventual, out of order
    rcv: window of rcv messages. send ack: first missing (+max 10 oldest gaps) -> implicit acks!
    send: keep window of messages not acked yet.
      resend some after time out
L2: reliable package: arbitrary size, given to handler when complete
L3: Data Request, broadcast
L4: User level login.


Packet header:
dimensions: reliable? seq-num, handler-nr
