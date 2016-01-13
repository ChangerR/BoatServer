#! /usr/bin/env python
#coding=utf-8
import wx

class PaintWindow(wx.Window):
        def __init__(self, parent, id):
            wx.Window.__init__(self, parent, id)
            self.Bind(wx.EVT_KEY_DOWN,self.onKeyDown)
            self.Bind(wx.EVT_KEY_UP,self.onKeyUp)

        def onKeyUp(self,event):
            print("KeyUp=" + str(event.GetKeyCode()))

        def onKeyDown(self,event):
            print("KeyDown=" + str(event.GetKeyCode()))

class PaintFrame(wx.Frame):
    def __init__(self, parent):
        wx.Frame.__init__(self, parent, -1, "Boat Client", size = (480, 600))
        self.paint = PaintWindow(self, -1)

if __name__ == '__main__':
    app = wx.App()
    frame = PaintFrame(None)
    frame.Show(True)
    app.MainLoop()
