# Programarea Calculatoarelor, seria CC
# Tema2 - 2048

build: 2048

2048: 2048.c
	gcc -Wall 2048.c -o 2048 -lcurses

.PHONY:

pack:
	zip -FSr 312CC_Banila_Rares.zip README Makefile 2048.c

clean:
	rm -f 2048 data

run:
	./2048
