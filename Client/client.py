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
import platform

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
		if platform.system() == 'Windows':
			self.filename = str(filename.split("\\")[-1])
		else :
			self.filename = str(filename.split("/")[-1])
		self.pos = 0

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
			#print len(buf)
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
		self.tasks = []
		self.father = father
                self._thro = 0
                self._yaw = 0
                self._led = [0,0]

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
				self.father.onStatus(status)
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
			elif rdata['name'] == 'log':
				#print rdata
				self.father.appendLog(rdata['args'][0])

		if time.time()*1000.0 - self.last > 45000.0:
			self.sock.sendto("1:::",self.addr)
			self.last = time.time()*1000.0

		for task in self.tasks:
			task.processTask(self)

	def thro(self,direction):
                if self._thro != direction:
		    msg = "2:::{\"name\":\"thro\",\"args\":[" + str(direction) + "]}"
		    self.queue.put(msg)
                    self._thro = direction

	def yaw(self,direction):
                if self._yaw != direction:
		    msg = "2:::{\"name\":\"yaw\",\"args\":[" + str(direction) + "]}"
		    self.queue.put(msg)
                    self._yaw = direction

	def state(self,s):
		msg = "2:::{\"name\":\"ControlState\",\"args\":[" + str(s) + "]}"
		self.queue.put(msg)

	def led(self,id,power):
                if self._led[id] != power: 
		    msg = "2:::{\"name\":\"led\",\"args\":[" + str(id) + "," +  str(power) +"]}"
		    self.queue.put(msg)
                    self._led[id] = power;

	def send(self,msg):
		self.queue.put(msg)

	def sendFile(self,filename):
		task = FileUploadTask(filename)
		self.tasks.append(task)

	def requsetList(self):
		msg = "3:::listcontrolfiles"
		self.queue.put(msg)

	def setPower(self,power):
		self.power = power

	def setControlLua(self,file):
		msg = "4:::" + file
		self.queue.put(msg)

class PaintWindow(wx.Window):
		def __init__(self, parent, id):
			wx.Window.__init__(self, parent, id)
			self.Bind(wx.EVT_IDLE,self.idle)
			self.times = 0
			self.SetBackgroundColour("white")
			self.requestBtn = wx.Button(self,label=u"更新列表",pos=(280,20),size=(90,30))
			self.listc = wx.Choice(self,-1,(10,20),size=(260,30),choices=[])
			self.setLuaBtn = wx.Button(self,label=u"设置脚本",pos=(380,20),size=(90,30))
			self.listState = wx.Choice(self,-1,(10,70),size=(200,30),choices=[u'手动控制',u'半自动控制',u'自动控制'])
			self.listState.Select(0)
			wx.StaticText(self,-1,u"功率",(235,75),style=wx.ALIGN_CENTER)
			self.powerSlide = wx.Slider(self,100,20,1,pos=(270,70),size=(200,-1),style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS)
			wx.StaticText(self,-1,u"灯光1",(10,130),style=wx.ALIGN_CENTER)
			self.led1Slide = wx.Slider(self,10010,0,0,pos=(50,115),size=(170,-1),style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS)
			wx.StaticText(self,-1,u"灯光2",(235,130),style=wx.ALIGN_CENTER)
			self.les2Slide = wx.Slider(self,10011,0,0,pos=(280,115),size=(190,-1),style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS)
			self.Bind(wx.EVT_SLIDER,self.onLED1Slide,self.led1Slide)
			self.Bind(wx.EVT_SLIDER,self.onLED2Slide,self.les2Slide)
			#self.state = wx.TextCtrl(self,-1,u'手动控制',pos=(340,70),size=(130,30),style=wx.TE_READONLY)
			#self.state.SetBackgroundColour("lightgrey")
			self.panel = wx.Panel(self,-1,pos=(10,170),size=(460,300))
			self.panel.SetBackgroundColour("grey")
			self.logstatus = wx.TextCtrl(self,-1,'',pos=(10,480),size=(460,150),style=wx.TE_READONLY|wx.TE_MULTILINE)
			self.logstatus.SetBackgroundColour("lightgrey")
			self.lc = wx.ListCtrl(self.panel, -1,pos=(10,10),size=(200,280),style=wx.LC_REPORT)
			self.lc.InsertColumn(0, u'状态')
			self.lc.InsertColumn(1, u'数值')
			self.lc.SetColumnWidth(0,100)
			self.lc.SetColumnWidth(1,100)
			#self.lc.SetItemCount(8)
			self.lc.Append((u'roll',0))
			self.lc.Append((u'pitch',0))
			self.lc.Append((u'yaw',0))
			self.lc.Append((u"经度",0))
			self.lc.Append((u"纬度",0))
			self.lc.Append((u"速度",0))
			self.lc.Append((u"时间",0))
			self.lc.Append((u"更新时间",0))
			self.lc.Append((u"运行模式",u"手动控制"))
			self.lc.Append((u"灯光模组1",0))
			self.lc.Append((u"灯光模组2",0))
			self.lc.Append((u"Thrust1",0))
			self.lc.Append((u"Thrust2",0))
			self.lc.Append((u"Motor1",0))
			self.lc.Append((u"Motor2",0))
			self.lc.Append((u"Motor3",0))
			self.lc.Append((u"Motor4",0))
			self.laserp = wx.Panel(self.panel,-1,pos=(220,10),size=(230,280))
			self.laserp.SetBackgroundColour("white")
			self.laser5 = wx.TextCtrl(self.laserp,-1,'0',pos=(90,10),size=(50,30),style=wx.TE_READONLY|wx.TE_CENTRE)
			self.laser3 = wx.TextCtrl(self.laserp,-1,'0',pos=(10,80),size=(50,30),style=wx.TE_READONLY|wx.TE_CENTRE)
			self.laser4 = wx.TextCtrl(self.laserp,-1,'0',pos=(170,80),size=(50,30),style=wx.TE_READONLY|wx.TE_CENTRE)
			self.laser1 = wx.TextCtrl(self.laserp,-1,'0',pos=(10,190),size=(50,30),style=wx.TE_READONLY|wx.TE_CENTRE)
			self.laser2 = wx.TextCtrl(self.laserp,-1,'0',pos=(170,190),size=(50,30),style=wx.TE_READONLY|wx.TE_CENTRE)
			image = wx.Image("boat.bmp",wx.BITMAP_TYPE_BMP)
			wx.StaticBitmap(self.laserp,bitmap=image.ConvertToBitmap(),pos=(65,55))
			self.Bind(wx.EVT_BUTTON,self.onRequestList,self.requestBtn)
			self.Bind(wx.EVT_BUTTON,self.onSetControlLua,self.setLuaBtn)
			self.Bind(wx.EVT_CHOICE,self.onStateChoice,self.listState)
			self.Bind(wx.EVT_KEY_DOWN,self.onKeyDown,self)
			self.Bind(wx.EVT_KEY_UP,self.onKeyUp,self)
			self.client = BoatClient("192.168.1.108",6666,self)
			self.control_list = []

		def onSetControlLua(self,event):
			self.client.setControlLua(self.listc.GetItems()[self.listc.GetCurrentSelection()])

		def onLED1Slide(self,event):
			self.client.led(1,event.GetInt())

		def onLED2Slide(self,event):
			self.client.led(2,event.GetInt())

		def onStatus(self,status):
			self.lc.SetStringItem(0,1,str(status[u"roll"]))
			self.lc.SetStringItem(1,1,str(status[u"pitch"]))
			self.lc.SetStringItem(2,1,str(status[u"yaw"]))
			self.lc.SetStringItem(3,1,str(status[u"longitude"]))
			self.lc.SetStringItem(4,1,str(status[u"latitude"]))
			self.lc.SetStringItem(5,1,str(status[u"speed"]))
			self.lc.SetStringItem(6,1,str(status[u"time"]))
			self.lc.SetStringItem(7,1,str(status[u"updateTime"]))
			self.lc.SetStringItem(8,1,self.listState.GetItems()[status[u"controlState"]])
			self.lc.SetStringItem(9,1,str(status[u"led1"]))
			self.lc.SetStringItem(10,1,str(status[u"led2"]))
			self.lc.SetStringItem(11,1,str(status[u"thrust1"]))
			self.lc.SetStringItem(12,1,str(status[u"thrust2"]))
			self.lc.SetStringItem(13,1,str(status[u"motor1"]))
			self.lc.SetStringItem(14,1,str(status[u"motor2"]))
			self.lc.SetStringItem(15,1,str(status[u"motor3"]))
			self.lc.SetStringItem(16,1,str(status[u"motor4"]))
			self.laser1.SetValue(str(status[u'laser1']))
			self.laser2.SetValue(str(status[u'laser2']))
			self.laser3.SetValue(str(status[u'laser3']))
			self.laser4.SetValue(str(status[u'laser4']))
			self.laser5.SetValue(str(status[u'laser5']))

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
					self.client.thro(self.powerSlide.GetValue())
				elif event.GetKeyCode() == 65:
					self.client.yaw(-self.powerSlide.GetValue())
				elif event.GetKeyCode() == 83:
					self.client.thro(-self.powerSlide.GetValue())
				elif event.GetKeyCode() == 68:
					self.client.yaw(self.powerSlide.GetValue())
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
			time.sleep(0.001)

		def sendFile(self,filename):
			self.client.sendFile(filename)

		def appendLog(self,logs):
			self.logstatus.AppendText(logs + '\r\n')

class PaintFrame(wx.Frame):
	def __init__(self, parent):
		wndsize = (480,640)
		if platform.system() == 'Windows':
			wndsize = (496,695)
		wx.Frame.__init__(self, parent, -1, u"无人船控制系统", size = wndsize)
		self.paint = PaintWindow(self, -1)
		menubar = wx.MenuBar()
		#File menu
		fileMenu = wx.Menu()
		fileMenu.Append(100,u"&上传","")
		menubar.Append(fileMenu,u"&文件")

		helpMenu = wx.Menu()
		helpMenu.Append(200,u"&关于","")
		menubar.Append(helpMenu,u"&帮助")

		self.SetMenuBar(menubar)
		self.Bind(wx.EVT_MENU,self.onFileOpen,id=100)
		self.Bind(wx.EVT_MENU,self.onAbout,id=200)

	def onFileOpen(self,event):
		dialog = wx.FileDialog(self,u"上传控制脚本",os.getcwd(),style=wx.OPEN,wildcard="*.lua")
		if dialog.ShowModal() == wx.ID_OK:
			self.paint.sendFile(dialog.GetPath())
		dialog.Destroy()

	def onAbout(self,event):
		msg = wx.MessageDialog (self,u"copyright © 杭州爱易特智能技术有限公司 2015-2016",u"关于此系统",wx.OK)
		msg.ShowModal()
		msg.Destroy()


if __name__ == '__main__':
	app = wx.App()
	frame = PaintFrame(None)
	frame.Show(True)
	app.MainLoop()
