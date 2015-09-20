CC = g++
CFLAGS = -g -m64 -O
EXEC = Daemon 
OBJS = $(patsubst ./%.cpp, ./%.o, $(wildcard ./*.cpp))
all:	$(EXEC)
#	cp $(EXEC) ../bin
$(EXEC) : $(OBJS)	
	$(CC) -o $(EXEC) $(OBJS)
%.o : %.cpp	
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm $(EXEC) *.o
