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

all: regra1 regra2


regra1: #cria o .o da biblioteca
	@echo "Criando arquivo t2fs.o"
	$(CC) -I$(INC_DIR) -c $(SRC_DIR)/t2fs.c -o $(BIN_DIR)/t2fs.o -Wall
	@echo " "
	
regra2: #cria .a da biblioteca
	ar rcs $(LIB_DIR)/libt2fs.a $(BIN_DIR)/t2fs.o $(LIB_DIR)/*.o
	

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~


