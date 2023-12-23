CC = g++

GLLIBS = -lglut -lGLEW -lGL

all: dcel.cpp 
	$(CC) dcel.cpp -g -o dcel $(GLLIBS)

clean:
	rm -f dcel
