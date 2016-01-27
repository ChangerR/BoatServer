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
import json

class FileUploadTask:
	def __init__(self,filename):
		self.m = md5()
		self.file = open(filename,"rb")
		self.m.update(self.file.read())
		self.file.seek(0,2)
		self.length = self.file.tell()
		self.file.seek(0,0)
		self.count = (self.length + 2047) / 2048
		self.id = 0
		self.filename = str(filename.split("/")[-1])
		self.pos = 0
		print "file task init end"

	def processTask(self,client):
		if self.id == 0:
			data = struct.unpack('16B',self.m.digest())
			buf = struct.pack('8si16B64s','f:::BTSB',self.count,
								data[0],data[1],data[2],data[3],
								data[4],data[5],data[6],data[7],
								data[8],data[9],data[10],data[11],
								data[12],data[13],data[14],data[15],str(self.filename.split("/")[-1]))
			client.send(buf)
		elif self.id <= self.count:
			writelen = 2048
			if writelen > self.length:
				writelen = self.length
			self.file.seek(self.pos,0)
			data = self.file.read(writelen)
			buf = struct.pack('8s3i64s','f:::LTSB',self.id,self.pos,writelen,self.filename)
			self.pos = self.pos + writelen
			self.length = self.length - writelen
			buf = buf + data
			print len(buf)
			client.send(buf)
		self.id = self.id + 1

	def endTask(self,filename):
		if filename == self.filename:
			self.file.close()
			return True
		else :
			return False

class BoatClient:
	def __init__(self,addr,port,father):
		self.addr = (addr , port)
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
		self.queue = Queue.Queue(maxsize=0xffff)
		#self.sock.bind(self.addr)
		self.last = time.time()*1000.0
		self.power = 20
		self.tasks = []
		self.father = father;

	def handleMessage(self):
		if self.queue.empty() == False :
		    msg = self.queue.get(False)
		    self.sock.sendto(msg,self.addr)

		rlist,wlist,xlist = select.select([self.sock],[],[],0)
		for s in rlist:
			data,addr = self.sock.recvfrom(4096)
			#print "received message:",data
			rdata = json.loads(data)
			if rdata['name'] == 'status':
				status = rdata['args'][0]
				#print status
			elif rdata['name'] == 'filetrans':
				filetrans = rdata['args'][0]
				print rdata
				if filetrans['type'] == 0:
					for task in self.tasks:
						if task.endTask(filetrans['filename']) == True:
							self.tasks.remove(task)
							break
			elif rdata['name'] == 'controlfiles':
				self.father.setFileList(rdata['args'])

		if time.time()*1000.0 - self.last > 45000.0:
			self.sock.sendto("1:::",self.addr)
			self.last = time.time()*1000.0

		for task in self.tasks:
			task.processTask(self)

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

	def sendFile(self,filename):
		task = FileUploadTask(filename)
		self.tasks.append(task)

	def requsetList(self):
		msg = "3:::listcontrolfiles"
		self.queue.put(msg)

class PaintWindow(wx.Window):
		def __init__(self, parent, id):
			wx.Window.__init__(self, parent, id)
			self.Bind(wx.EVT_KEY_DOWN,self.onKeyDown)
			self.Bind(wx.EVT_KEY_UP,self.onKeyUp)
			self.Bind(wx.EVT_IDLE,self.idle)
			self.times = 0
			self.requestBtn = wx.Button(self,label=u"更新列表",pos=(280,20),size=(90,30))
			self.listc = wx.Choice(self,-1,(10,20),size=(260,30),choices=[])
			self.setLuaBtn = wx.Button(self,label=u"设置脚本",pos=(380,20),size=(90,30))
			self.listState = wx.Choice(self,-1,(10,70),size=(200,30),choices=[u'手动控制',u'半自动控制',u'自动控制'])
			wx.StaticText(self,-1,u"当前控制状态",(240,75),style=wx.ALIGN_CENTER)
			self.state = wx.TextCtrl(self,-1,u'手动控制',pos=(340,70),size=(130,30),style=wx.TE_READONLY)
			self.state.SetBackgroundColour("lightgrey")
			self.panel = wx.Panel(self,-1,pos=(10,130),size=(460,300))
			self.panel.SetBackgroundColour("darkgrey")
			self.logstatus = wx.TextCtrl(self,-1,'',pos=(10,460),size=(460,130),style=wx.TE_READONLY|wx.TE_MULTILINE)
			self.logstatus.SetBackgroundColour("lightgrey")
			self.lc = wx.ListCtrl(self.panel, -1,pos=(10,10),size=(200,280),style=wx.LC_REPORT)
			self.lc.InsertColumn(0, u'状态')
			self.lc.InsertColumn(1, u'数值')
			self.lc.SetColumnWidth(0,100)
			self.lc.SetColumnWidth(0,100)
			self.lc.SetItemCount(8)
			self.lc.Append((u'roll',0))
			self.lc.Append((u'pitch',0))
			self.lc.Append((u'yaw',0))
			self.lc.Append((u"经度",0))
			self.lc.Append((u"经度",0))
			self.lc.Append((u"高度",0))
			self.lc.Append((u"速度",0))
			self.lc.Append((u"时间",0))
			self.Bind(wx.EVT_BUTTON,self.onRequestList,self.requestBtn)
			self.Bind(wx.EVT_CHOICE,self.onStateChoice,self.listState)
			self.client = BoatClient("127.0.0.1",6666,self)
			self.control_list = []

		def onStateChoice(self,event):
			self.client.state(self.listState.GetCurrentSelection())

		def onRequestList(self,event):
			self.client.requsetList();

		def setFileList(self,lists):
			self.control_list = lists
			self.listc.SetItems(lists)
			self.listc.Select(0)

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
			self.client.sendFile(filename)

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
