CFLAG = -c -O0 -g  -I json
LKFLAG =
SRCDIR = $(shell pwd)

OBJS = serial-linux.o ServerConfig.o Command.o Hardware.o Timer.o NetServer.o Pilot.o UDPFileTransfer.o \
		AutoController.o MD5.o logger.o

LKLIBA = -L .

TARGET = boat

LUAJITHEADER = lauxlib.h lua.h lua.hpp luaconf.h luajit.h lualib.h
LUAJITLIB = libluajit.a
	
$(TARGET): $(LUAJITHEADER) $(OBJS) main.o $(LUAJITLIB)
	g++ -o $@ $(OBJS) main.o $(LKFLAG) $(LKLIBA) -lluajit -ldl -lpthread

$(LUAJITLIB):
	make -C luajit
	cp luajit/src/libluajit*.a $@

$(LUAJITHEADER):
	cd luajit/src && cp $(LUAJITHEADER) ../..

AutoController.o:$(LUAJITHEADER) AutoController.cpp
	g++ $(CFLAG) -o $@ AutoController.cpp

AutoControllerMock.o:$(LUAJITLIB) AutoController-mock.cpp
	g++ $(CFLAG) -o $@ AutoController-mock.cpp

script_test.o:$(LUAJITHEADER)

logger.o: logger.cpp 
	g++ $(CFLAG) -o $@ logger.cpp
	
TestSerial:TestSerial.o $(OBJS)
	g++ -o $@ $(OBJS) TestSerial.o $(LKFLAG) $(LKLIBA)

TestNetServer:TestNetServer.o $(OBJS) $(LUAJITLIB)
	g++ -o $@ $(OBJS) TestNetServer.o $(LKFLAG) $(LKLIBA) -lluajit -ldl

TestHardware:TestHardware.o $(OBJS)
	g++ -o $@ $(OBJS) TestHardware.o $(LKFLAG) $(LKLIBA)

TestMD5:TestMd5.o MD5.o
	g++ -o $@ TestMd5.o MD5.o $(LKFLAG) $(LKLIBA)

script_test:script_test.o logger.o AutoControllerMock.o
	g++ -o $@ script_test.o logger.o AutoControllerMock.o $(LKFLAG) $(LKLIBA) -lluajit -ldl

install:boat boat.config
	install -d /usr/local/boat
	install -m 755 boat /usr/local/boat
	install -m 666 boat.config /usr/local/boat
	install -d /usr/local/boat/workdir
	install -m 666 workdir/*.lua /usr/local/boat/workdir
	install -m 777 boat.service /usr/local/boat
	ln -s /usr/local/boat/boat.service /etc/init.d/boat
	update-rc.d boat defaults 90

uninstall:
	update-rc.d -f boat remove
	-rm /etc/init.d/boat
	-rm -r /usr/local/boat

%.o:%.c
	gcc $(CFLAG) -o $@ $<

%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	-rm *.o
	-rm $(TARGET) $(LUAJITLIB) $(LUAJITHEADER) script_test
	cd luajit&&make clean
