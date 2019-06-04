// ARQUIVO USADO PARA TESTAR O CODIGO !!!!!!!!!!!!!
#include <stdio.h>
#include <stdlib.h>
#include "t2fs.h"

int main( int argc, char *argv[])
{	
	int tam_setor = 256; //tamanho do setor em bytes
	
	char *conteudo;
	conteudo = malloc(tam_setor);
	
	//lendo setor 1 do disco e gravando em conteudo
	read_sector (1, conteudo);
	printf("\n bytes do setor 1: %p \n",(void *)conteudo);
	
	return 0;
}