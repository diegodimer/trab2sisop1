#include <stdio.h>
#include <stdlib.h>
#include "t2fs.h"

int main(int argc, char *argv[])
{
		int tam_setor = 256
	
		if(format(4)!=0)
			printf("\n erro na formatacao \n");
		char navy_seal[400] = "What the fuck did you just fucking say about me, you little bitch? I'll have you know I graduated top of my class in the Navy Seals, and I've been involved in numerous secret raids on Al-Quaeda, and I have over 300 confirmed kills. I am trained in gorilla warfare and I'm the top sniper in the entire US armed forces.";
		
		char nome[31]="navysealpasta";
		
		int header_meme = create2(nome);
		
		if(header_meme<0)
			printf("\n erro ao criar arquivo");
		
		if(write2(header_meme,navy_seal,318)<0)
			printf("\n erro ao escrever navyseal \n");
		
		char retorno[382];
		
		if(read2(header_meme,retorno,382)<0)
			printf("\n erro ao ler navyseal \n");
		
	return 0;
	
}