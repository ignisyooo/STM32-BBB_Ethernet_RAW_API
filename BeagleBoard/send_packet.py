#!/usr/bin/env python
# -*- coding: utf-8 -*-

from socket import *
import struct

# Note: w zależności od użytego sprzętu pamietaj o modyfikacji nazwy interfejsu
def sendeth(ethernet_packet, payload, interface = "eth0"):
  """Wyślij pakiet przez interfejs."""
  s = socket(AF_PACKET, SOCK_RAW)
  s.bind((interface, 0))
  return s.send(ethernet_packet + payload)

def pack(byte_sequence):
  """Przekształć listę bytów na string."""
  return b"".join(map(chr, byte_sequence))
      
if __name__ == "__main__":
    # Note: ten przykład zawiera pakiety hardcoded, 
    # to znaczy będzie działał tylko w systemi dla niego przeznaczonym.

    # HEADER
    dst_addr = [0x00, 0x80, 0xe2, 0x00, 0x00, 0x00]
    src_addr = [0xd0, 0x03, 0xeb, 0x27, 0xb8, 0xee] 
    protocol = [0x99, 0x99]
    header = dst_addr + src_addr + protocol
    # HEADER

    payload =   [0x08, 0x00, 0x2b, 0x45, 0x11, 0x22, 0x00, 0x02, 0xa9, 0xf4,
                0x53, 0x00, 0x00, 0x00, 0x00, 0xf5, 0x7b, 0x01, 0x00, 0x00,
                0x08, 0x00, 0x2b, 0x45, 0x11, 0x22, 0x00, 0x02, 0xa9, 0xf4,
                0x53, 0x00, 0x00, 0x00, 0x00, 0xf5, 0x7b, 0x01, 0x00, 0x00,
                0x08, 0x00, 0x2b, 0x45, 0x11, 0x22, 0x00, 0x02, 0xa9, 0xf4,
                0x53, 0x00, 0x00, 0x00, 0x00, 0xf5, 0x7b, 0x01, 0x00, 0x00,
                0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17]
    
    # payload = [0x01] * 64
    msg_len = len(dst_addr) + len(src_addr) + len(protocol) + len(payload)
    pay_len = len(payload)

    pay_len_bytes = [0x00, 0x00]
    pay_len_bytes[0] = (pay_len >> 8) & 0xff
    pay_len_bytes[1] = (pay_len) & 0xff

    # print(payload)
    # print(pay_len)
    # print(pay_len_bytes)

    payload = pay_len_bytes + payload

    # Konsturukcja i wysłanie pakietu
    r = sendeth(pack(header), pack(payload))
    print("Wysłano ramke Ethernet o długości %d z payload'em o długości %d bajtów." % (r, pay_len))
