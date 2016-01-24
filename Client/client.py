#! /usr/bin/env python
#coding=utf-8
import wx
import socket
import Queue
import time
import select
import os
import struct
from hashlib import md5

class FileUploadTask:
	def __init__(self,filename):
		m = md5()
		self.file = open(filename,"rb")
		m.update(self.file.read())
		self.md5 = m.digest()
		self.file.seek(0,2)
		self.length = self.file.tell()
		self.file.seek(0,0)
		self.count = (self.length + 2047) / 2048
		self.id = 0
		self.filename = filename
		self.pos = 0
		
	def processTask(self,client):
		if self.id == 0:
			buf = struct.pack('4si16B64s','BTSB',self.count,self.md5,self.filename)
			client.send(buf)
		elif self.id <= count:
			writelen = 2048
			if writelen > self.length:
				writelen = self.length
			self.file.seek(self.pos,0)
			data = self.file.read(writelen)
			format = '4s3i64s' + str(writelen) + 'B';
			buf = struct.pack(format,'LTSB',self.id,self.pos,writelen,self.filename,data)
			self.pos = self.pos + writelen
			self.length = self.length - writelen
			client.send(buf)
		self.id = self.id + 1
		
	def endTask(self,filename):
		if filename == self.filename:
			self.file.close()
			return True
		else 
			return False
			
class BoatClient:
    def __init__(self,addr,port):
        self.addr = (addr , port)
        self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.queue = Queue.Queue(maxsize=0xffff)
        #self.sock.bind(self.addr)
        self.last = time.time()*1000.0
        self.power = 20

    def handleMessage(self):
        if self.queue.empty() == False :
            msg = self.queue.get(False)
            self.sock.sendto(msg,self.addr)

        rlist,wlist,xlist = select.select([self.sock],[],[],0)
        for s in rlist:
            data,addr = self.sock.recvfrom(4096)
            print "received message:",data

        if time.time()*1000.0 - self.last > 45000.0:
            self.sock.sendto("1:::",self.addr)
            self.last = time.time()*1000.0

    def thro(self,direction):
        msg = "2:::{\"name\":\"thro\",\"args\":[" + str(direction * self.power) + "]}"
        self.queue.put(msg)

    def yaw(self,direction):
        msg = "2:::{\"name\":\"yaw\",\"args\":[" + str(direction * self.power) + "]}"
        self.queue.put(msg)

    def state(self,s):
        msg = "2:::{\"name\":\"ControlState\",\"args\":[" + str(s) + "]}"
        self.queue.put(msg)

	def send(self,msg):
		self.queue.put(msg)

class PaintWindow(wx.Window):
		def __init__(self, parent, id):
			wx.Window.__init__(self, parent, id)
			self.Bind(wx.EVT_KEY_DOWN,self.onKeyDown)
			self.Bind(wx.EVT_KEY_UP,self.onKeyUp)
			self.Bind(wx.EVT_IDLE,self.idle)
			self.times = 0		
			
			self.client = BoatClient("127.0.0.1",5000)		
			
		def onKeyDown(self,event):
			if self.client != None:
				if event.GetKeyCode() == 87:
					self.client.thro(1)
				elif event.GetKeyCode() == 65:
					self.client.yaw(-1)
				elif event.GetKeyCode() == 83:
					self.client.thro(-1)
				elif event.GetKeyCode() == 68:
					self.client.yaw(1)
				elif event.GetKeyCode() == 49:
					self.client.state(0)
				elif event.GetKeyCode() == 50:
					self.client.state(1)
				elif event.GetKeyCode() == 51:
					self.client.state(2)

		def onKeyUp(self,event):
			if self.client != None:
				if event.GetKeyCode() == 87:
					self.client.thro(0)
				elif event.GetKeyCode() == 65:
					self.client.yaw(0)
				elif event.GetKeyCode() == 83:
					self.client.thro(0)
				elif event.GetKeyCode() == 68:
					self.client.yaw(0)

		def idle(self,event):
			if self.client != None:
				self.client.handleMessage()
			event.RequestMore(True)
			
		def sendFile(self,filename):
			print filename

class PaintFrame(wx.Frame):
	def __init__(self, parent):
		wx.Frame.__init__(self, parent, -1, "Boat Client", size = (480, 600))
		self.paint = PaintWindow(self, -1)
		menubar = wx.MenuBar()
		#File menu
		fileMenu = wx.Menu()
		fileMenu.Append(100,"&Upload","")
		menubar.Append(fileMenu,"&File")

		helpMenu = wx.Menu()
		helpMenu.Append(200,"About","")
		menubar.Append(helpMenu,"&Help")

		self.SetMenuBar(menubar)
		self.Bind(wx.EVT_MENU,self.onFileOpen,id=100)
		self.Bind(wx.EVT_MENU,self.onAbout,id=200)

	def onFileOpen(self,event):
		dialog = wx.FileDialog(self,"Open file...",os.getcwd(),style=wx.OPEN,wildcard="*")
		if dialog.ShowModal() == wx.ID_OK:
			self.paint.sendFile(dialog.GetPath())
		dialog.Destroy()
		
	def onAbout(self,event):
		msg = wx.MessageDialog (self,"copyright @ ABR Innovation","about",wx.OK)
		msg.ShowModal()
		msg.Destroy()
		

if __name__ == '__main__':
    app = wx.App()
    frame = PaintFrame(None)
    frame.Show(True)
    app.MainLoop()
