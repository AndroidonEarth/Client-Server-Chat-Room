###
### File: makefile
### Author: Andrew Swaim
### Date: October 2019
### Description: super simple makefile for chatclient
###     deliverable for Project 1, CS372_400_F2019
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
