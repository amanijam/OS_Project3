mysh: shell.c interpreter.c shellmemory.c scheduler.c
	gcc -D FRAMESIZE=$(framesize) -D VARMEMSIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c scheduler.c -ggdb
	gcc -o mysh shell.o interpreter.o shellmemory.o scheduler.o

clean: 
	rm mysh; rm *.o
