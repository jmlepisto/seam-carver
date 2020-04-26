CC = g++
CFLAGS = -g -Wall
SRCS = src/*.cpp
PROG = carver

OPENCV = `pkg-config opencv4 --cflags --libs`
LIBS = $(OPENCV)

all:$(PROG)

$(PROG):$(SRCS)
	$(CC) $(CFLAGS) -fopenmp -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
