CFLAG = -c -O0 -g  -I json
LKFLAG =

OBJS = serial-linux.o ServerConfig.o Command.o Hardware.o Timer.o NetServer.o Pilot.o

LKLIBA =

TARGET = boat

$(TARGET): $(OBJS) main.o
	g++ -o $@ $(OBJS) main.o $(LKFLAG) $(LKLIBA)

TestSerial:TestSerial.o $(OBJS)
	g++ -o $@ $(OBJS) TestSerial.o $(LKFLAG) $(LKLIBA)

TestNetServer:TestNetServer.o $(OBJS)
	g++ -o $@ $(OBJS) TestNetServer.o $(LKFLAG) $(LKLIBA)

TestHardware:TestHardware.o $(OBJS)
	g++ -o $@ $(OBJS) TestHardware.o $(LKFLAG) $(LKLIBA)

%.o:%.c
	gcc $(CFLAG) -o $@ $<

%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	-rm *.o
	-rm $(TARGET)
