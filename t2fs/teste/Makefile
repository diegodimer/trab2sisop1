#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
# 

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src


geratudo: regra1 regra2 regra3

regra1: #cria .o da biblioteca
	$(CC) -c t2fs.c -o t2fs.o -Wall
	
regra2: #cria .a da biblioteca
	ar rcs libt2fs.a t2fs.o apidisk.o

regra3: #cria o executavel e linka a biblioteca
	$(CC) -o executavel main.c -L../teste -lt2fs -Wall
	


limpa:
	rm -rf t2fs.o libt2fs.a executavel
