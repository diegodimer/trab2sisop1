#include "t2fs.h"
#include "apidisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO_LEITURA -1
#define ERRO_ESCRITA -2

#define LAST_BLOCK 0

typedef struct{
    WORD  version;          /* Versão do trabalho */
    WORD  sectorSize;       /* Tamanho do setor em Bytes */
    WORD  tabelaParticoes;  /* Byte inicial da tabela de partições */
    WORD  nParticoes;       /* Quantidade de partições no disco */
    DWORD setorInicioP1;    /* endereço do bloco de início da partição 1 */
    DWORD setorFimP1;       /* endereço do ultimo bloco da partição 1 */
    BYTE  nome[25];         /* Nome da partição 1 */
} MBR;

typedef struct {
    DWORD   proxBloco;     /* Variável para o próximo bloco ocupado por essa entrada */
    DWORD   nBlocosSist;   /* Varíavel com o número de blocos no sistema */
    char    name[231];      /* Nome do arquivo cuja entrada foi lida do disco      */
    DWORD   fileSize;       /* Numero de bytes do arquivo */
} ROOTDIR;


ROOTDIR rootDirectory;
int inicializado = 0;
int debug = 1;
int setoresPorBloco;
int bloco_livre =0;

// inicializa o sistema (preenche o diretório raiz, se não tiver cria. Supoem que o disco está formatado (variavel setoresporbloco preenchida)
int init()
{
    unsigned char buffer[257];
    if(read_sector(1, buffer))
        return ERRO_LEITURA;
    ROOTDIR *dir;
    dir = (ROOTDIR *) buffer;
    if(strlen(dir->name) == 0 || strcmp(dir->name, "root")!=0)
    {
        if(debug == 1)
        {
            printf("Não tem diretório raiz\n");
            printf("Criando diretório raiz...\n");
        }
        // criação do diretório raiz
        strcpy(rootDirectory.name, "root");
        rootDirectory.fileSize   = sizeof(ROOTDIR);
        rootDirectory.proxBloco = LAST_BLOCK;                   // não tem próximo bloco = 0
        rootDirectory.nBlocosSist = setoresPorBloco;

        if(!write_sector(1, (unsigned char*)&rootDirectory))
        {
            if(debug == 1)
                printf("Diretório raiz criado com sucesso!\n");
        }
        else
        {
            printf("ERRO na leitura do diretório raiz\n");
            return ERRO_LEITURA;
        }
    }
    else
    {
        rootDirectory = *dir;
        if(debug == 1)
        {
            printf("Diretório raiz lido com sucesso!\n");
            printf("Root directory name: %s\n", rootDirectory.name);
        }
    }
    inicializado = 1;
    setoresPorBloco = rootDirectory.nBlocosSist;
    return 0;
}


// le um bloco da memória
unsigned char* readBlock(DWORD firstSector){
    if(!inicializado){
        init();
    }
    if(firstSector<1){
        return NULL; // se quer o bloco 0 ou negativo, não existe (começa em 1 o número de blocos)
    }

    unsigned char *buffer; // variável preenchida com o conteúdo do bloco
    int tamanhoRetorno = (SECTOR_SIZE*setoresPorBloco); // bloco é setorsize*nsetoresporbloco
    buffer = malloc(tamanhoRetorno);

    int index = 0;
    int i=0;
    int j;
    unsigned char* auxBuffer = malloc(SECTOR_SIZE);
    while(i!=setoresPorBloco){
        if(debug == 1){
            printf("Lendo setor %d\n", firstSector+i);
        }
        read_sector(firstSector+i, auxBuffer);
        for(j=0; j<SECTOR_SIZE;j++){
            buffer[index] = auxBuffer[j];
            index++;
        }
        i++;
    }
    return buffer;
}


/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size)
{
    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };
    if(size>= strlen("Afonto, Diego, Eduardo"))
    {
        strcpy(name, "Afonso, Diego, Eduardo");
        return 1;
    }

    return -1;
}

//aloca um bloco na memoria, retorna O SETOR INICIAL DO BLOCO 
int aloca_bloco()
{
	
	if(bloco_livre==0){
		printf("\n nao ha mais blocos disponiveis! \n");
		return -1;
	}
	unsigned char buffer[SECTOR_SIZE];
	
	read_sector(bloco_livre,buffer);
	
	int novo_bloco_livre = buffer[1]<<8 | buffer[0];
	int retorno = bloco_livre;
	bloco_livre = novo_bloco_livre;
	
	return retorno;
	
	
}
//libera um bloco ocupado, esse bloco se junta à linked list dos blocos livres
void libera_bloco(int id_bloco)
{
	unsigned char vetor0[SECTOR_SIZE]={0};
	//salva o endereço do antigo bloco livre no bloco passado como parametro
	vetor0[1] = (bloco_livre>>8);
	vetor0[0] = bloco_livre;
	if(write_sector(id_bloco,vetor0)!=0)
		printf("erro ao liberar bloco ! \n");
	else bloco_livre = id_bloco;

	
}


/*-----------------------------------------------------------------------------
Função:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block)
{
    if(sectors_per_block < 2 || sectors_per_block % 1024 != 0 || sectors_per_block >= 1024)
    {
        return -1;
    }
	//lendo o endereço da partição a ser formatada e transformando de bytes para int
	unsigned char conteudo_mbr[SECTOR_SIZE];
	if(read_sector(0,conteudo_mbr)!=0)
		printf("\n erro ao ler mbr \n");
	char conteudo_bytes[]={conteudo_mbr[8],conteudo_mbr[9],conteudo_mbr[10],conteudo_mbr[11]};
	int *end_part = (int *)conteudo_bytes;
	
	//lendo o ultimo bloco lógico (setor) da partição a ser formatada e transformando de bytes para int
	char conteudo_ultimo[] = {conteudo_mbr[12],conteudo_mbr[13],conteudo_mbr[14],conteudo_mbr[15]};
	int *end_ultimosetor = (int *)conteudo_ultimo;
	
	unsigned char vetorzeros[SECTOR_SIZE]={0};
	
	//escreve zeros em todos os setores da partição
	int i;
	for(i=*end_part;i<=*end_ultimosetor;i++){
		if(write_sector(i,vetorzeros)!=0){
			printf("\n erro na formatação dos setores \n");
			return -2;
		}
	}
	
	//escreve o endereço do próximo bloco livre nos 2 primeiros bytes de cada bloco
	
	for(i= *end_part;i<*end_ultimosetor-sectors_per_block;i=i+sectors_per_block){
		
		int prox_bloco= i + sectors_per_block;
		vetorzeros[1]=(prox_bloco>>8);
		vetorzeros[0]=prox_bloco;
		//printf(" it: %d numero sendo salvo %02x %02x \n",i,vetorzeros[0]&0xFF,vetorzeros[1]&0xFF);
		//char numint[2]={vetorzeros[0],vetorzeros[1]};
		//int gg = *(int *)numeroemint;
		//printf(" numero em int: %d \n",gg);
		if(write_sector(i,vetorzeros)!=0){
			printf("\n erro na formatação dos blocos livres\n");
			return -3;
		}
		
	}
	//define a variavel global do inicio da linked list de blocos livres como o bloco 2
	bloco_livre = *end_part + sectors_per_block;
	
	
	//!!!!!!!!!!!!!!!!!!!!!ATENÇÃO!!!!!!!!!!!!!!!!!!!!!
	//UNICO COMANDO QUE TRANSFORMA OS BYTES DO ARQUIVO EM INT CORRETAMENTE:
	// int numero_do_setor = buffer[1]<<8 | buffer[0];
	
	
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ATENÇÃO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!FOI DEFINIDO ARBITRARIAMENTE QUE UM PONTEIRO PARA BLOCO LIVRE COM VALOR 0 SIGNIFICA QUE NÃO HÁ MAIS BLOCOS LIVRES DISPONIVEIS!!!!!!!!!!
	
		
	
	
	
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um
		arquivo já existente, o mesmo terá seu conteúdo removido e
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename)
{
    // o caminho é absoluto, então filename vem desde o root até onde é, eu entro no root, vejo se
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo. Remove do arquivo
		todos os bytes a partir da posição atual do contador de posição
		(current pointer), inclusive, até o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Altera o contador de posição (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo diretório.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um diretório do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename)
{
    return -1;
}



