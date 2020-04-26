CC = g++
CFLAGS = -g -Wall
SRCS = src/*.cpp
PROG = Carver.run

OPENCV = `pkg-config opencv4 --cflags --libs`
LIBS = $(OPENCV)

all:$(PROG)

$(PROG):$(SRCS)
	$(CC) $(CFLAGS) -fopenmp -g -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
