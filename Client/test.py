#!/usr/bin/env python
import struct
from hashlib import md5

m = md5()
file = open('client.py','rb')
m.update(file.read())
file.close()
data = m.digest()
buf = struct.pack('8si16s64s','f:::BTSB',1000,data,"tset.lua")
print buf
print len(buf)
