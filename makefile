###
### File: makefile
### Author: Andrew Swaim
### Date: October 2019
### Description: super simple makefile for chatclient
###

PROJ=chatclient
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, %.o, ${SRCS})

CC=gcc
RM=rm -f

.PHONY: default all clean

default:
	make clean ${PROJ}
all: default

${PROJ}: ${SRCS}
	${CC} -o ${PROJ} ${SRCS}

clean:
	${RM} ${OBJS} ${PROJ}
