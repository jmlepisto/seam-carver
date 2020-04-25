CC = g++
CFLAGS = -g -Wall
SRCS = src/*.cpp
PROG = Carver

OPENCV = \
-I /usr/local/include/opencv4 \
-L /usr/lib \
-lopencv_core \
-lopencv_imgproc \

LIBS = $(OPENCV)

$(PROG):$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)
