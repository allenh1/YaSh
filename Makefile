#
# H sh
#
# Use GNU compiler
cc = gcc -g
CC = g++ -fPIC --std=c++14 -O2

LEX=lex
YACC=yacc

all: hsh

lex.yy.o: shell.l 
	$(LEX) shell.l
	$(CC) -c lex.yy.c

y.tab.o: shell.y
	$(YACC) -d shell.y
	$(CC) -c y.tab.c

command.o: command.cc shell-readline.h
	$(CC) -c command.cc

hsh: y.tab.o lex.yy.o command.o shell-readline.h
	$(CC) -g -lpthread -o hsh lex.yy.o y.tab.o command.o -lfl

install:
	make clean
	make release
	sudo mv hsh /usr/bin/hsh
	make all

release: y.tab.o lex.yy.o command.o shell-readline.h
	$(CC) -o hsh lex.yy.o y.tab.o command.o -lfl

clean:
	rm -f lex.yy.c y.tab.c  y.tab.h shell *.o *~
