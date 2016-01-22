CFLAG = -c -O0 -g  -I json
LKFLAG =

OBJS = serial-linux.o ServerConfig.o Command.o Hardware.o Timer.o NetServer.o Pilot.o

LKLIBA =

TARGET = boat

LUAJITHEADER = lauxlib.h lua.h lua.hpp luaconf.h luajit.h lualib.h
LUAJITLIB = libluajit.a

$(LUAJITLIB):
	make -C luajit
	cp luajit/src/libluajit*.a $@

$(LUAJITHEADER):
	cd luajit/src && cp $(LUAJITHEADER) ../..

AutoController.o:$(LUAJITHEADER) AutoController.cpp
	g++ $(CFLAG) -o $@ AutoController.cpp


$(TARGET): $(OBJS) main.o
	g++ -o $@ $(OBJS) main.o $(LKFLAG) $(LKLIBA)

TestSerial:TestSerial.o $(OBJS)
	g++ -o $@ $(OBJS) TestSerial.o $(LKFLAG) $(LKLIBA)

TestNetServer:TestNetServer.o $(OBJS)
	g++ -o $@ $(OBJS) TestNetServer.o $(LKFLAG) $(LKLIBA)

TestHardware:TestHardware.o $(OBJS)
	g++ -o $@ $(OBJS) TestHardware.o $(LKFLAG) $(LKLIBA)

TestMD5:TestMd5.o MD5.o
	g++ -o $@ TestMd5.o MD5.o $(LKFLAG) $(LKLIBA)

%.o:%.c
	gcc $(CFLAG) -o $@ $<

%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	-rm *.o
	-rm $(TARGET) $(LUAJITLIB) $(LUAJITHEADER)
	cd luajit&&make clean
