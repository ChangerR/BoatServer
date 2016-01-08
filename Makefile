CFLAG = -c -O0 -g
LKFLAG =

OBJS = serial-linux.o ServerConfig.o Command.o main.o

LKLIBA =

TARGET = boat

$(TARGET): $(OBJS)
	g++ -o $@ $(OBJS) $(LKFLAG) $(LKLIBA)

%.o:%.c
	gcc $(CFLAG) -o $@ $<

%.o:%.cpp
	g++ $(CFLAG) -o $@ $<

clean:
	-rm *.o
	-rm $(TARGET)
