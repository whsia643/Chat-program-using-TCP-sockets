all: a3chat

a3chat:	a3chat.c
	gcc a3chat.c -o chat

clean:
	rm -rf *o chat
tar:
	tar cfv submit.tar makefile a3chat.c projectreport3.pdf
