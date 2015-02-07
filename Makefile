LDFLAGS=-lconfig -L/usr/local/natinst/ni4882/lib -lgpibapi -lreadline

all:	pp2pp-slow-server lv-pp2pp-slow.so pp2pp-cmd

pp2pp-slow-server:	pp2pp-slow-server.c pp2pp-slow.h

lv-pp2pp-slow.so:	lv-pp2pp-slow.c pp2pp-slow.h
	gcc -fpic -shared lv-pp2pp-slow.c -o lv-pp2pp-slow.so

pp2pp-cmd: pp2pp-cmd.cpp pp2pp-slow.h

