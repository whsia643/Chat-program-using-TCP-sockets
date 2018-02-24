all: chat

chat:	chat.c
	gcc chat.c -o chat

clean:
	rm -rf *o chat

