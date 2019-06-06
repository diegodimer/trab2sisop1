#
# Makefile de EXEMPLO
#
# OBRIGATÓRIO ter uma regra "all" para geração da biblioteca e de uma
# regra "clean" para remover todos os objetos gerados.
#
# É NECESSARIO ADAPTAR ESSE ARQUIVO de makefile para suas necessidades.
#  1. Cuidado com a regra "clean" para não apagar o "support.o"
#
# OBSERVAR que as variáveis de ambiente consideram que o Makefile está no diretótio "cthread"
# 

CC=gcc
LIB_DIR=../lib
INC_DIR=../include


all: gera_todos

MYDIR = .

#loop por todos os arquivos .c do diretório compilando eles 
gera_todos:
	@for f in $(shell ls ${MYDIR}/*.c); do echo "Gerando arquivo " $${f}; gcc -static $${f} -L$(LIB_DIR) -I$(INC_DIR) -lt2fs -o $${f%.c}.run ; done
	
	
roda_todos:
	@for f in $(shell ls ${MYDIR}/*.run); do echo "Rodando arquivo teste " $${f}; ./$${f} ; echo " "; done
	

clean:
	@echo "Arquivos de teste sendo apagados"
	rm -rf  *.run
	@echo " "