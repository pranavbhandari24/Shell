CXX=gcc

all: msh

msh: msh.c
	${CXX} -o msh msh.c

clean:
	rm msh