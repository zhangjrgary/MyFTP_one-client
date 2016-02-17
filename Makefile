CC = gcc
LIB =

all: myftpserver myftpclient

myftpserver: myftpserver.c
	${CC} -o myftpserver myftpserver.c ${LIB}

myftpclient: myftpclient.c
	${CC} -o myftpclient myftpclient.c ${LIB}
