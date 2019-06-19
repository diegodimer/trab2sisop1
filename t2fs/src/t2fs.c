#include "t2fs.h"
#include "apidisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO_LEITURA -1
#define ERRO_ESCRITA -2

#define INVALID_PATH -3

#define LAST_BLOCK 0

#define ARQ_REGULAR 0x01
#define ARQ_DIRETORIO 0x02
#define ARQ_LINK 0x03
#define MAX_DIR_OPEN 3

typedef struct
{
    WORD  version;          /* Versão do trabalho */
    WORD  sectorSize;       /* Tamanho do setor em Bytes */
    WORD  tabelaParticoes;  /* Byte inicial da tabela de partições */
    WORD  nParticoes;       /* Quantidade de partições no disco */
    DWORD setorInicioP1;    /* endereço do bloco de início da partição 1 */
    DWORD setorFimP1;       /* endereço do ultimo bloco da partição 1 */
    BYTE  nome[25];         /* Nome da partição 1 */
} MBR;

typedef struct
{
    DWORD   nBlocosSist;   /* Varíavel com o número de blocos no sistema */
    char    name[256];     /* PATH ABSOLUTO DO DIRETÓRIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    int		bloco_livre;
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRETÓRIOS FILHOS */
} ROOTDIR;

typedef struct
{
    char    name[256];     /* PATH ABSOLUTO DO DIRETÓRIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    DWORD   setorDados;    /* PONTEIRO PARA O PRIMEIRO SETOR COM OS DADOS (ARQUIVO REGULAR) */
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRETÓRIOS FILHOS */
} DIRENT3;

FILE2 handle_global=0;
//array com ponteiros para arquivos abertos atualmente no sistema
DIRENT3 arquivos_abertos[10];


ROOTDIR *rootDirectory;
int inicializado = 0;
int debug = 0;
DWORD setoresPorBloco=2;
int bloco_livre =0;



// Funções auxiliares
int writeBlock(unsigned char *, DWORD); // função que escreve em um bloco
unsigned char* readBlock(DWORD); // função que lê um bloco e retorna um ponteiro para esse bloco
int aloca_bloco(); // função que aloca um bloco
void libera_bloco(int ); // função que desaloca um bloco (preenche com 0s)
int init(); // função que inicia o sistema (preenche variaveis necessárias para execução
DIRENT3 *lookForDir(char*, DWORD*); // função que procura um diretório (não deve ser chamada por usuários)
void listBlocks(DIRENT3*, DWORD **, int *); // caminha recursivamente no grafo listando o bloco dos filhos de um diretorio
// variáveis relativas ao current path
char *currentPath=NULL;
DIRENT3 *currentDir;
int raiz; // se está no diretório raiz


DIRENT3 *openDirectories[MAX_DIR_OPEN];
int dirIndex;


// inicializa o sistema (preenche o diretório raiz, se não tiver cria. Supoem que o disco está formatado (variavel setoresporbloco preenchida)
int init()
{
    unsigned char *buffer;
    inicializado = 1;
    buffer = readBlock(1);

    if(setoresPorBloco == 2)
    {
        char conteudo_ultimo[] = {buffer[0],buffer[1],buffer[2],buffer[3]};
        int *stblk = (int *)conteudo_ultimo;
        setoresPorBloco = *stblk;
        buffer = readBlock(1);
    }
    if(buffer == NULL)
    {
        return ERRO_LEITURA;
    }
    int quantos_filhos =(256 * setoresPorBloco) - 265 - 3;

    rootDirectory = malloc(sizeof(*rootDirectory)+quantos_filhos);
    rootDirectory = (ROOTDIR *) buffer;

    if(strcmp(rootDirectory->name, "root")!=0)
    {
        if(debug == 1)
        {
            printf("Nao tem diretorio raiz\n");
            printf("Criando diretório raiz...\n");
        }

        free(rootDirectory);
        // criação do diretório raiz
        rootDirectory = malloc(sizeof(*rootDirectory)+quantos_filhos);
        strcpy(rootDirectory->name, "root");
        rootDirectory->fileSize    = sizeof(*rootDirectory)+quantos_filhos;
        rootDirectory->fileType    = ARQ_DIRETORIO;
        rootDirectory->nBlocosSist = setoresPorBloco;
        rootDirectory->numFilhos   = 0;
        rootDirectory->bloco_livre = bloco_livre;

        if(!writeBlock((unsigned char*)rootDirectory, 1))
        {
            if(debug == 1)
                printf("Diretorio raiz criado com sucesso!\n");
        }
        else
        {
            printf("ERRO na escrita do diretorio raiz\n");
            return ERRO_LEITURA;
        }
    }
    else
    {

        if(debug == 1)
        {
            printf("Diretorio raiz lido com sucesso!\n");
            printf("Root directory name: %s\n", rootDirectory->name);
        }

        setoresPorBloco = rootDirectory->nBlocosSist;
        bloco_livre = rootDirectory->bloco_livre;

    }

    dirIndex=0;
    int i=0;
    for(i=0; i<MAX_DIR_OPEN; i++)
        openDirectories[i]=NULL;

    return 0;
}

/** ESSA FUNÇÃO NÃO CUIDA SE PODE OU NÃO ESCREVER NO BLOCO, NEM A CORRETUDE (FIRSTSECTOR SENDO MULTIPLO DE 2)!
 ELA APENAS ESCREVE **/
// escreve data no bloco que começa no setor firstSector
int writeBlock(unsigned char *data, DWORD firstSector)
{
    if (firstSector == -1)
        return -1;
    if(!inicializado)
    {
        init();
    }
    int index = 0;
    int i,j;
    unsigned char aux[SECTOR_SIZE] = {0};
    for(i=0; i<setoresPorBloco; i++) // pra cada setor
    {
        for(j=0; j<SECTOR_SIZE; j++)  // monta o aux com o tamanho do setor
        {
            aux[j] = data[index+j];
        }
        if(debug == 1)
            printf("escrevendo no setor %d\n", firstSector+i);
        index += SECTOR_SIZE; // pra começar de data um setor a frente sempre
        write_sector( firstSector+i, aux );

    }
    return 0;
}

// le um bloco da memória
unsigned char* readBlock(DWORD firstSector)
{

    if(firstSector<1)
    {
        return NULL; // se quer o bloco 0 ou negativo, não existe (começa em 1 o número de blocos)
    }

    unsigned char *buffer; // variável preenchida com o conteúdo do bloco

    int tamanhoRetorno = (SECTOR_SIZE*setoresPorBloco); // bloco é setorsize*nsetoresporbloco
    buffer = calloc(tamanhoRetorno, sizeof(unsigned char));
    int index = 0;
    int i=0;
    int j;
    unsigned char* auxBuffer = malloc(SECTOR_SIZE);
    while(i!=setoresPorBloco)
    {
        if(debug == 1)
        {
            printf("Lendo setor %d\n", firstSector+i);
        }
        read_sector(firstSector+i, auxBuffer);
        for(j=0; j<SECTOR_SIZE; j++)
        {
            buffer[index] = auxBuffer[j];
            index++;
        }
        i++;
    }
    free(auxBuffer);
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
    if(size>= strlen("Afonso, Diego, Eduardo"))
    {
        strcpy(name, "Afonso, Diego, Eduardo");
        return 1;
    }

    return -1;
}

//aloca um bloco na memoria, retorna O SETOR INICIAL DO BLOCO
int aloca_bloco()
{

    if(rootDirectory->bloco_livre==0)
    {
        printf("\n nao ha mais blocos disponiveis! \n");
        return 0;
    }
    unsigned char buffer[SECTOR_SIZE];

    read_sector(rootDirectory->bloco_livre,buffer);

    int novo_bloco_livre = buffer[1]<<8 | buffer[0];
    int retorno = rootDirectory->bloco_livre;
    rootDirectory->bloco_livre = novo_bloco_livre;

    writeBlock((unsigned char*)rootDirectory, 1);
    return retorno;


}
//libera um bloco ocupado, esse bloco se junta à linked list dos blocos livres
void libera_bloco(int id_bloco)
{
    unsigned char vetor0[SECTOR_SIZE]= {0};
    //salva o endereço do antigo bloco livre no bloco passado como parametro
    vetor0[1] = (rootDirectory->bloco_livre>>8);
    vetor0[0] = rootDirectory->bloco_livre;
    if(write_sector(id_bloco,vetor0)!=0)
        printf("erro ao liberar bloco ! \n");
    else
        rootDirectory->bloco_livre = id_bloco;

    writeBlock((unsigned char*)rootDirectory, 1);
}


/*-----------------------------------------------------------------------------
Função:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block)
{
    if(sectors_per_block < 2 || sectors_per_block % 2 != 0 || sectors_per_block >= 1024)
    {
        return -1;
    }
    //lendo o endereço da partição a ser formatada e transformando de bytes para int
    unsigned char conteudo_mbr[SECTOR_SIZE];
    if(read_sector(0,conteudo_mbr)!=0)
        printf("\n erro ao ler mbr \n");
    char conteudo_bytes[]= {conteudo_mbr[8],conteudo_mbr[9],conteudo_mbr[10],conteudo_mbr[11]};
    int *end_part = (int *)conteudo_bytes;

    //lendo o ultimo bloco lógico (setor) da partição a ser formatada e transformando de bytes para int
    char conteudo_ultimo[] = {conteudo_mbr[12],conteudo_mbr[13],conteudo_mbr[14],conteudo_mbr[15]};
    int *end_ultimosetor = (int *)conteudo_ultimo;

    unsigned char vetorzeros[SECTOR_SIZE]= {0};

    //escreve zeros em todos os setores da partição
    int i;
    for(i=*end_part; i<=*end_ultimosetor; i++)
    {
        if(write_sector(i,vetorzeros)!=0)
        {
            printf("\n erro na formatação dos setores \n");
            return -2;
        }
    }

    //escreve o endereço do próximo bloco livre nos 2 primeiros bytes de cada bloco

    for(i= *end_part; i<*end_ultimosetor-sectors_per_block; i=i+sectors_per_block)
    {

        int prox_bloco= i + sectors_per_block;
        vetorzeros[1]=(prox_bloco>>8);
        vetorzeros[0]=prox_bloco;
        //printf(" it: %d numero sendo salvo %02x %02x \n",i,vetorzeros[0]&0xFF,vetorzeros[1]&0xFF);
        //char numint[2]={vetorzeros[0],vetorzeros[1]};
        //int gg = *(int *)numeroemint;
        //printf(" numero em int: %d \n",gg);
        if(write_sector(i,vetorzeros)!=0)
        {
            printf("\n erro na formatação dos blocos livres\n");
            return -3;
        }

    }
    //define a variavel global do inicio da linked list de blocos livres como o bloco 2
    bloco_livre = *end_part + sectors_per_block;
    setoresPorBloco = sectors_per_block;
    inicializado=0;
    init();


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
	int size_filename=0;
	while(filename[size_filename]!='\0'){
		size_filename++;
	}

	if(size_filename>31){
		printf("\n nome deve ter entre 0 e 31 caracteres! \n");
		return -1;
	}
	if(handle_global>=10){
		printf(" Ja existem 10 arquivos abertos! \n");
		return -2;
	}
	

	char caminho_completo[256];
	getcwd2(caminho_completo,256);	
	
	DIRENT3* diretorio_atual=NULL;
	DIRENT3* filho=NULL;
	DWORD* bloco_dir;
	int i;
	int blocofilho;
	unsigned char* buffer_dir;

	int teste_raiz=0;
	bloco_dir=malloc(sizeof(DWORD));
	if(strcmp(caminho_completo,"/")!=0){
		
		diretorio_atual = lookForDir(caminho_completo,bloco_dir);
	}
	else{
		printf("é raiz \n");
		teste_raiz =1;
		
	}

	
	strcat(caminho_completo,filename);


	printf(" caminho completo: %s ",caminho_completo);
	
	int bloconovo = aloca_bloco();
	
	DIRENT3 *novoarquivo;
	novoarquivo = malloc(sizeof(*novoarquivo));
	strcpy(novoarquivo->name,caminho_completo);
	novoarquivo->fileType = 0x01;
	novoarquivo->fileSize=0;
	//PARA ARQUIVOS REGULARES, NUMFILHOS INDICA O CURRENT POINTER DO ARQUIVO
	novoarquivo->numFilhos = 0;
	novoarquivo->setorDados = bloconovo +1;
	
	write_sector(bloconovo,(unsigned char*)novoarquivo);
	
	
	//coloca um ponteiro para a dirent3 desse arquivo no array dos arquivos abertos
	arquivos_abertos[handle_global]=novoarquivo;
	//atualiza o valor do handle global
	handle_global +=1;	
	
	if(teste_raiz==0){
		printf("numero de filhos: %d ",diretorio_atual->numFilhos);
		int excluiu=0;
		for(i=0;i<diretorio_atual->numFilhos;i++){
			blocofilho = diretorio_atual->dirFilhos[i];
			buffer_dir = readBlock(blocofilho);
			filho = (DIRENT3*)buffer_dir;
			if(strcmp(filho->name,caminho_completo)==0){
				printf("\n ja existe arquivo com esse nome, excluindo... \n");
				delete2(caminho_completo);
				diretorio_atual->dirFilhos[i]=bloconovo;
				excluiu=1;
			}
		}
		if(excluiu==0){
			diretorio_atual->dirFilhos[diretorio_atual->numFilhos]=bloconovo;
			diretorio_atual->numFilhos = diretorio_atual->numFilhos+1;
		}

		printf("escrevendo diretorio atual no bloco %d \n",*bloco_dir);
		writeBlock((unsigned char*)diretorio_atual,*bloco_dir);
	}
	else{
		int root_excluiu=0;
		for(i=0;i< rootDirectory->numFilhos;i++){
			blocofilho = rootDirectory->dirFilhos[i];
			buffer_dir = readBlock(blocofilho);
			filho = (DIRENT3*)buffer_dir;
			if(strcmp(filho->name,caminho_completo)==0){
				printf("\n ja existe arquivo com esse nome, excluindo... \n");
				delete2(caminho_completo);
				rootDirectory->dirFilhos[i]=bloconovo;
				root_excluiu=1;
			}
		}
		if(root_excluiu==0){
			rootDirectory->dirFilhos[rootDirectory->numFilhos]=bloconovo;
			rootDirectory->numFilhos = rootDirectory->numFilhos+1;
		}
	
			
	}

    return (handle_global-1);
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
	//ESSA FUNÇÃO SUPÕE QUE ARQUIVOS TERMINEM COM '\0'

	DIRENT3 *arquivoatual = arquivos_abertos[handle];
	
	//-1 porque o primeiro setor esta sendo ocupado pelo header do arquivo
	DWORD primeirobloco = arquivoatual->setorDados -1;
	
	//numfilhos armazena o contador atual do programa
	int contador = arquivoatual->numFilhos;
	
	int setores_por_bloco = rootDirectory->nBlocosSist;
	
	int tamanho = arquivoatual->fileSize;
	
	//tirar todo o arquivo da memoria
	unsigned char buffer_reader[SECTOR_SIZE]={0};
	
	int tamanho_arredondado=0;
	while(tamanho>0){
		tamanho = tamanho -254;
		tamanho_arredondado += 254;
	}
	
	unsigned char *arquivo_lido;
	arquivo_lido = malloc(tamanho_arredondado);
	
	int i;
	int j;
	int k=0;
	//le os 254 bytes de cada setor do primeiro bloco
	for(i=1;i<setores_por_bloco;i++){
		
		if(read_sector(primeirobloco+i,buffer_reader)!=0){
			printf("\n erro ao ler setor %d \n",primeirobloco+i);
			return -1;
		}
		printf(" em read2: leu setor %d \n",primeirobloco+i);		

		//poe o conteudo lido em arquivo_lido
		for(j=0;j<254;j++){
			arquivo_lido[k] = buffer_reader[j];
			k++;
		}
		
		tamanho_arredondado = tamanho_arredondado - 254;
		if(tamanho_arredondado==0)
			break;
	}
	
	read_sector(primeirobloco+setores_por_bloco-1,buffer_reader);
	
	int numero_do_setor = buffer_reader[256]<<8 | buffer_reader[255];
	
	//le o resto do arquivo
	while(tamanho_arredondado!=0){
		
		//vai lendo blocos
		for(i=0;i<setores_por_bloco;i++){
			
			if(read_sector(numero_do_setor+i,buffer_reader)!=0){
				printf("\n erro ao ler setor %d \n",numero_do_setor+i);
				return -1;
			}
			for(j=0;j<254;j++){
				arquivo_lido[k] = buffer_reader[j];
				k++;
			}
			tamanho_arredondado = tamanho_arredondado - 254;
			if(tamanho_arredondado==0)
				break;
			
		}
		read_sector(numero_do_setor+i,buffer_reader);
		numero_do_setor = buffer_reader[256]<<8 | buffer_reader[255];
		
	}
	printf(" FUNC READ2: conteudo de arquivo_lido: \n");
	printf(" valor de contador: %d valor de size: %d \n",contador,size);
	for(i=0;i<=contador+size;i++){
		printf("%c",arquivo_lido[i]);
	}
	printf(" \n");
	
	//agr vem o pulo do gato
	//só truncar a string de acordo com os bytes indicados no contador
	arquivo_lido = arquivo_lido + contador;
	strncpy(buffer,(char *)arquivo_lido,size);
	if(arquivoatual->fileSize-contador<size){
		//contador chegou ao final do arquivo
		arquivoatual->numFilhos = arquivoatual->fileSize;
		return (arquivoatual->fileSize-contador);		
	}
	else {
		//atualiza o contador
		arquivoatual->numFilhos = contador+size;
		return size;
	}
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size)
{	
	//ESSA FUNÇÃO SUPÕE QUE O "SIZE" NÃO INCLUA O \0 DO FINAL

	DIRENT3 *arquivoatual = arquivos_abertos[handle];
	
	//-1 porque o primeiro setor esta sendo ocupado pelo header do arquivo
	DWORD primeirobloco = arquivoatual->setorDados -1;
	
	//numfilhos armazena o contador atual do programa
	int contador = arquivoatual->numFilhos;
	
	int setores_por_bloco = rootDirectory->nBlocosSist;
	
	int tamanho = arquivoatual->fileSize;
	
	//tirar todo o arquivo da memoria
	unsigned char buffer_reader[SECTOR_SIZE]={0};
	
	int tamanho_arredondado=0;
	while(tamanho>0){
		tamanho = tamanho -254;
		tamanho_arredondado += 254;
	}
	
	unsigned char *arquivo_lido;
	
	
	int tamanho2 = arquivoatual->fileSize + size;
	int tam_malloc=0;
	while(tamanho2>0){
		tamanho2 = tamanho2 -254;
		tam_malloc += 254;
	}
	arquivo_lido = malloc(tam_malloc);
	
	int i;
	int j;
	int k=0;
	int numero_do_setor;
	//le os 254 bytes de cada setor do primeiro bloco
	if(tamanho_arredondado>0){
		for(i=1;i<setores_por_bloco;i++){
		
			if(read_sector(primeirobloco+i,buffer_reader)!=0){
				printf("\n erro ao ler setor %d \n",primeirobloco+i);
				return -1;
			}
		
			//poe o conteudo lido em arquivo_lido
			for(j=0;j<254;j++){
				arquivo_lido[k] = buffer_reader[j];
				k++;
			}
		
			tamanho_arredondado = tamanho_arredondado - 254;
			if(tamanho_arredondado<=0)
				break;
		}
	
		read_sector(primeirobloco+setores_por_bloco-1,buffer_reader);
	
		numero_do_setor = buffer_reader[256]<<8 | buffer_reader[255];
	}
	//le o resto do arquivo
	while(tamanho_arredondado>0){
		
		//vai lendo blocos
		for(i=0;i<setores_por_bloco;i++){
			
			if(read_sector(numero_do_setor+i,buffer_reader)!=0){
				printf("\n erro ao ler setor %d \n",numero_do_setor+i);
				return -1;
			}
			for(j=0;j<254;j++){
				arquivo_lido[k] = buffer_reader[j];
				k++;
			}
			tamanho_arredondado = tamanho_arredondado - 254;
			if(tamanho_arredondado<=0)
				break;
			
		}
		read_sector(numero_do_setor+i,buffer_reader);
		numero_do_setor = buffer_reader[256]<<8 | buffer_reader[255];
		
	}
	
	//escrever o conteudo novo
	for(i=0;i<=size;i++){
		arquivo_lido[contador+i] = buffer[i];
		
	}
	//se adicionou conteúdo no fim do arquivo poe \0 no final
	if(contador+size>arquivoatual->fileSize)
	{	
		arquivoatual->fileSize = contador+size;
		arquivo_lido[contador+size]='\0';
	}
	printf(" conteudo de arquivo_lido: \n");
	for(i=0;i<=contador+size;i++){
		printf("%c",arquivo_lido[i]);
	}
	printf(" \n");
	
	//grava de novo o arquivo
	int tamanho_novo = arquivoatual->fileSize;
	unsigned char buffer_writer[SECTOR_SIZE];
	k=0;
	for(i=1;i<setores_por_bloco;i++){
		
		//testa se esta no ultimo setor do primeiro bloco
		if(i==(setores_por_bloco-1)){
			if(read_sector(primeirobloco+setores_por_bloco-1,buffer_reader)!=0){
				printf("\n erro ao pegar endereço do proximo bloco no setor %d \n",numero_do_setor+i);
				return -1;
			}
			numero_do_setor = buffer_reader[256]<<8 | buffer_reader[255];

			if(numero_do_setor==0){
				numero_do_setor = aloca_bloco();
			}
			buffer_writer[256]=(numero_do_setor>>8);
			buffer_writer[255]=numero_do_setor;
			
		}
		else{
			buffer_writer[256]=0;
			buffer_writer[255]=0;
		}
		
		//poe o conteudo lido em arquivo_lido
		for(j=0;j<254;j++){
			buffer_writer[j]=arquivo_lido[k];
			k++;
		}
		
		
		if(write_sector(primeirobloco+i,buffer_writer)!=0){
			printf("\n erro ao escrever setor %d \n",primeirobloco+i);
			return -1;
		}
		printf("conteudo escrito no setor %d \n",primeirobloco+i);

		tamanho_novo = tamanho_novo - 254;
		if(tamanho_novo<=0)
			break;
		
		
	}
	int numero_auxiliar;
	while(tamanho_novo>0){
		
		//vai escrevendo blocos
		for(i=0;i<setores_por_bloco;i++){
			
			//testa se esta no ultimo setor do bloco
			if(i==(setores_por_bloco-1)){
				if(read_sector(numero_do_setor+setores_por_bloco-1,buffer_reader)!=0){
					printf("\n erro ao pegar endereço do proximo bloco no setor %d \n",numero_do_setor+i);
					return -1;
				}
				numero_auxiliar = buffer_reader[256]<<8 | buffer_reader[255];
				if(numero_auxiliar==0){
					numero_auxiliar = aloca_bloco();
				}
				buffer_writer[256]=(numero_auxiliar>>8);
				buffer_writer[255]=numero_auxiliar;
			
			}
			else{
				buffer_writer[256]=0;
				buffer_writer[255]=0;
			}
			
			
			for(j=0;j<254;j++){
				buffer_writer[j]=arquivo_lido[k];
				k++;
			}
			if(write_sector(numero_do_setor+i,buffer_writer)!=0){
			printf("\n erro ao escrever setor %d \n",primeirobloco+i);
			return -1;
			}
			
			
			if(i==(setores_por_bloco-1))
				numero_do_setor = numero_auxiliar;


			tamanho_novo = tamanho_novo - 254;
			if(tamanho_novo<=0)
				break;
			
		}
		
	}	
	

	arquivoatual->numFilhos = contador+size;
	return size;
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
    	DIRENT3 *arquivoatual = arquivos_abertos[handle];
	
	if((int)offset<(int)-1)
		return -1;

	if(offset == -1){
		arquivoatual->numFilhos = arquivoatual->fileSize;
	}
	else {
		arquivoatual->numFilhos = offset;
	}
	//printf("novo valor do contador: %d",arquivoatual->numFilhos);
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo diretório.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{

    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };


    if(pathname[0] != '/' || strlen(pathname) < 2) // o caminho é sempre absoluto, então se não entrou / no início não tá tentando acessar diretório raiz
    {
        printf("pathname incorreto, uso: /path_to_dir\n");
        return INVALID_PATH;
    };

    char *pathname2 = strdup(pathname); // não da pra pegar direto de pathname
    char *dir = strtok(pathname2, "/"); // o que procurar no diretório raiz

    // bloco onde salvar o novo diretório
    DWORD novoSubDiretorioBloco;
    DWORD novoDiretorioBloco;
    // novo diretório
    DIRENT3 *novoDiretorio = NULL;
    DIRENT3 *novoSubDiretorio = NULL;
    DIRENT3 *dirAuxiliar = NULL;

    int quantos_filhos =(256 * setoresPorBloco) - 265 - 3; // quantos filhos cada diretório pode ter
    unsigned char *buffer = NULL;

    int m=0;
    int n=0;
    int encontrado = 0;

    while(rootDirectory->numFilhos > n && encontrado == 0)  // procuro nos filhos do diretório raiz o filho dir
    {
        novoDiretorioBloco = rootDirectory->dirFilhos[n]; // o bloco onde o novoDiretorio está é no diretório raiz
        buffer = readBlock(novoDiretorioBloco);
        novoDiretorio = (DIRENT3 *)buffer;


        // comparar o diretório que estou (novodiretorio) com o que quero criar
        if(!strcmp(novoDiretorio->name, dir) && novoDiretorio->fileType == ARQ_DIRETORIO)
        {
            // se eles forem iguais encontrei
            encontrado = 1;
            if(debug == 1)
            {
                printf("Encontrei %s no diretorio raiz!\n", dir);
            }
        }
        else
        {
            n++;

        }

    }

    // se o diretório raiz tem 0 filhos ou eu não encontrei, preciso criar dentro do diretório raiz o dir
    if(encontrado == 0)
    {
        novoDiretorioBloco = aloca_bloco();
        // se não tem filhos no raiz, criar um
        if(novoDiretorioBloco == 0)
        {
            return -1;

        }

        if(debug == 1)
        {
            printf("Nao encontrei %s no diretorio raiz\nCriando ele como %d filho do raiz\n", dir, rootDirectory->numFilhos);
            printf("Criando filho do diretorio raiz no bloco %d\n", novoDiretorioBloco);
        }
        // se o diretório raiz ja tem o tamanho de um bloco em bytes não pode alocar mais um diretório

        // preencho os dados do novo diretório
        novoDiretorio = malloc(sizeof(*novoDiretorio)+quantos_filhos);
        novoDiretorio->dirFilhos[0] = -1;
        novoDiretorio->fileSize     = sizeof(*novoDiretorio)+quantos_filhos;
        novoDiretorio->fileType     = ARQ_DIRETORIO;
        novoDiretorio->numFilhos    = 0;
        novoDiretorio->setorDados   = LAST_BLOCK;
        strcpy(novoDiretorio->name, dir);

        // atualizo o diretório raiz
        if(rootDirectory->numFilhos >= (quantos_filhos/2))  // se não tem filhos ja tá alocado a memória pra um filho
        {
            printf("max de filhos atingido!\n");
            return -1;
        }
        else
        {
            rootDirectory->dirFilhos[rootDirectory->numFilhos] = novoDiretorioBloco;
        }
        rootDirectory->numFilhos = rootDirectory->numFilhos + 1; // atualizo o número de filhos


        if(!writeBlock((unsigned char *)rootDirectory, 1)) // salvo o diretório raiz na memória
        {
            if(!writeBlock((unsigned char*)novoDiretorio, novoDiretorioBloco)) // salvo o novo diretorio na memória
            {
                if(debug)
                    printf("Diretorio %s criado com sucesso no bloco %d!\n", dir, novoDiretorioBloco);
                else
                    printf("Diretorio %s criado com sucesso.\n", dir);
            }
        }
    }

    dir = strtok(NULL, "/");
    encontrado = 0;
    m=0;
    while(dir != NULL) // enquanto ainda tiver subdiretórios
    {
        encontrado = 0;
        m=0;
        // procurar em novoDiretorio a entrada com nome dir
        // se encontrar -> ela é novoDiretorio
        // se não encontrar -> criar ela e ela é novoDiretorio
        while(m < novoDiretorio->numFilhos && encontrado == 0)  // itera por todos os filhos do novo dirretório ou até encontrar
        {
            if(debug)
                printf("iterando pelos filhos de %s\n", novoDiretorio->name);
            //procurar em novoDiretorio os filhos até achar algum com nome de dir
            // le o bloco
            buffer = readBlock(novoDiretorio->dirFilhos[m]);
            dirAuxiliar = (DIRENT3 *) buffer;
            // compara o dirAuxiliar com o que estou procurando
            if(!strcmp(dirAuxiliar->name, dir) && dirAuxiliar->fileType == ARQ_DIRETORIO)
            {
                if(debug == 1)
                {
                    printf("Encontrado %s como subdiretorio de %s\n", dir, novoDiretorio->name);
                }
                encontrado = 1;
                // se for igual encontrei
                novoDiretorioBloco = novoDiretorio->dirFilhos[m];
                novoDiretorio = dirAuxiliar;
                // ele é o novoDiretorio, para procurar o proxímo token nele
            }
            else
            {
                m++;
            }
        }
        m=0; // para próxima iteração

        // se acabou o loop e não encontrou
        if (encontrado == 0)
        {
            novoSubDiretorioBloco = (DWORD)aloca_bloco(); // aloca bloco pra guardar o novo diretório

            if(novoSubDiretorioBloco == 0)  // se não consegue alocar bloco não pode criar
            {
                return -1;
            }
            if( novoDiretorio->numFilhos  >= (quantos_filhos/2 ))
            {
                printf("max de filhos atingido!\n");
                return -2; // não tem mais espaço para diretórios filhos retorna erro
            }


            novoSubDiretorio = malloc( sizeof(*novoSubDiretorio) + quantos_filhos);
            novoSubDiretorio->fileSize     = sizeof(*novoSubDiretorio) + quantos_filhos;
            novoSubDiretorio->fileType     = ARQ_DIRETORIO;
            novoSubDiretorio->numFilhos    = 0;
            novoSubDiretorio->setorDados   = LAST_BLOCK;
            novoSubDiretorio->dirFilhos[0] = -1;
            strcpy(novoSubDiretorio->name,dir);

            // criar em novoDiretorio a entrada dir
            // e faz de dir o novoDiretorio

            novoDiretorio->dirFilhos[novoDiretorio->numFilhos] = novoSubDiretorioBloco;
            novoDiretorio->numFilhos = novoDiretorio->numFilhos + 1;

            if(!writeBlock((unsigned char *)novoDiretorio, novoDiretorioBloco)) // salvo o diretório raiz na memória
            {
                if(!writeBlock((unsigned char*)novoSubDiretorio, novoSubDiretorioBloco)) // salvo o novo diretorio na memória
                {
                    if (debug)
                        printf("Diretorio %s criado com sucesso!\n%s eh subdiretorio de %s e esta no bloco %d\n", novoSubDiretorio->name, novoSubDiretorio->name, novoDiretorio->name, novoSubDiretorioBloco);
                    else
                        printf("Diretorio %s criado com sucesso.", novoSubDiretorio->name);
                }
            }
            *novoDiretorio = *novoSubDiretorio;
            novoDiretorioBloco = novoSubDiretorioBloco;
        }

        dir = strtok(NULL, "/");
        encontrado = 0;
    }

    free(buffer);
    return 0;

}


/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um diretório do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{
    if(!inicializado)
    {
        init();
    }
    char *string = strdup(pathname); // pra caso dê problema com a string antes

    DWORD block;
    DIRENT3 *toBeRemoved = lookForDir(pathname, &block); // procura diretório a ser removido
    if(toBeRemoved == NULL)
    {
        printf("Diretorio nao encontrado!\n");
        return -1;
    }
    if(toBeRemoved->fileType == ARQ_REGULAR)
    {
        printf("Nao eh arquivo de diretorio!\n");
        return -2;
    }

    DWORD *blocksToErase = NULL; // lista de blocos que precisam ser desalocados
    int count = 1;
    if(debug)
        printf("about to remove dir %s on block %d", toBeRemoved->name, block);
    int i;

    listBlocks(toBeRemoved, &blocksToErase, &count);

    if(count == 1)
    {
        blocksToErase = malloc(sizeof(DWORD)); // se nao tem filhos poem só ele na lista
    }

    blocksToErase[count-1] = block;

    for(i=0; i<count; i++)
    {
        if(debug)
            printf("%d\n", blocksToErase[i]);
        libera_bloco(blocksToErase[i]); // libera o bloco

    }

//     decrementa 1 do numero de filhos do diretorio pai
    // pega o penúltimo diretório na hierarquia, ele é o pai do diretorio que estou removendo (no sentido se entrou /a/b, removeu /b o pai é a
    char *str = strtok(string, "/");
    char *straux = str;
    char *newPath = malloc(strlen(string));
    newPath[0] = '/';
    newPath[1] = '\0';
    int encontrado=0;
    while(!encontrado)
    {
        straux = strtok(NULL,"/");

        if(straux== NULL)
        {
            if(debug)
                printf("\n ->>> %s\n", newPath);
            encontrado=1;
        }
        else
        {

            strcat(newPath, str);
            strcat(newPath, "/");
            str = straux;

        }
    }
///
    int indexToRemove;
    if(strcmp(newPath, "/")==0)
    {
        if(debug)
            printf("Filho do dir. raiz!\n");

        // encontra nos filhos diretório raiz o index do bloco que vai ser tirado e desloca o array todo uma posição atrás dele
        for(i=0; i!= -1 ; i++)
        {
            if(rootDirectory->dirFilhos[i] == block)
            {
                indexToRemove = i;
                i=-2;
            }
        }
        for(i=indexToRemove; i<rootDirectory->numFilhos; i++)
        {
            rootDirectory->dirFilhos[i] = rootDirectory->dirFilhos[i+1];
        }
        rootDirectory->numFilhos = rootDirectory->numFilhos-1;
        // atualiza o diretorio raiz no disco
        writeBlock((unsigned char*)rootDirectory, 1);
    }
    else
    {
        DWORD blocoPai;
        DIRENT3 *pai = lookForDir(newPath, &blocoPai);
        if(debug)
            printf("pai: %s\n", pai->name);

        // encontra nos filhos diretório pai o index do bloco que vai ser tirado e desloca o array todo uma posição atrás dele
        for(i=0; i!= -1 ; i++)
        {
            if(pai->dirFilhos[i] == block)
            {
                indexToRemove = i;
                i=-2;
            }
        }
        for(i=indexToRemove; i<pai->numFilhos; i++)
        {
            pai->dirFilhos[i] = pai->dirFilhos[i+1];
        }
        pai->numFilhos = pai->numFilhos-1;
        //atualiza o pai no disco
        writeBlock((unsigned char*)pai, blocoPai);
    }
    return 0;
}

void listBlocks(DIRENT3* alvo, DWORD **list, int *count)
{
    // pra cada diretório que entra nessa função eu listo os filhos dele e chamo recursivamente essa função para listar os filhos dele
    // fazendo percurso no grafo
    int i=0;
    if(debug)
        printf("LISTANDO FILHOS DE %s\n", alvo->name);

    for(i=0; i<alvo->numFilhos; i++) // pra cada diretório filho desse diretório
    {
        DWORD* biggerList = NULL;
        biggerList = (DWORD*) realloc (*list, *count * sizeof(DWORD)); // realoco a lista para caber mais o elemento que preciso

        if (biggerList!=NULL) // se consegui realocar
        {
            *list=biggerList; // atualizo a lista
            *(*list+*count-1)=alvo->dirFilhos[i]; // adiciono o bloco do diretorio filho na lista
            unsigned char *buffer = readBlock(alvo->dirFilhos[i]);
            DIRENT3 *filho = (DIRENT3 *)buffer;

            *count = *count + 1;
            listBlocks(filho, list, count); // leio o diretório filho e chamo a função para ele

        }
        else
        {
            free (*list);
            puts ("Error (re)allocating memory\n");
            exit (1);
        }
    }

}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname)
{

    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };
    if(!pathname || strcmp(pathname, "/") == 0) // só cd no linux leva pro diretório raiz
    {
        if(debug==1)
        {
            printf("Ir para o dir. raiz\n");
        }
        free(currentPath);
        currentPath = malloc(sizeof(char)*2);
        strcpy(currentPath, "/");
        raiz = 1;
        return 0;
    }

    int sucesso = 0;
    DIRENT3 *pathDir = lookForDir(pathname, NULL);
    if(debug)
        printf("CURRENT PATH NO DIRETORIO %s\n", pathDir->name);

    if(pathDir==NULL)
    {
        sucesso = 0;
    }
    else
    {
        sucesso = 1;
    }
    if(sucesso) // se eu achei o diretório pathname é sucesso, newpath é pathname
    {
        char *newPath = calloc(strlen(pathname), sizeof(char));
        if(newPath==NULL)
            return -1;
        strcpy(newPath, pathname);
        free(currentPath);
        currentPath = newPath;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size)
{
    if(!inicializado)
    {
        init();
    }
    if(currentPath == NULL)
    {
        currentPath = malloc(sizeof(char)*2);
        strcpy(currentPath, "/");
    }
    if(strlen(currentPath)>size)
        return -1;
    else
        strcpy(pathname, currentPath);
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{
    int handle;

    if(!inicializado)
    {
        init();
    }
    if(dirIndex >= MAX_DIR_OPEN)
    {
        printf("Nao pode abrir mais diretorios. Feche algum aberto!\n");
        return -1;
    }
    DIRENT3 *dirToOpen = lookForDir(pathname, NULL);

    if(dirToOpen == NULL)
    {
        printf("Diretorio nao encontrado.\n");
        return -1;
    }
    else
    {
        if(dirToOpen->fileType == ARQ_LINK)  // se é um link vai atrás do diretório real **ESPERAMOS QUE O USUARIO NAO APAGUE ELE E DEIXE O LINK**
        {
            while(dirToOpen->fileType == ARQ_LINK){
                unsigned char* buffer = NULL;
                buffer=readBlock(dirToOpen->setorDados);
                dirToOpen = (DIRENT3 *)buffer;
            }
        }
        if(dirToOpen->fileType == ARQ_DIRETORIO)
        {
            int i;
            for(i=0; i<MAX_DIR_OPEN; i++)
            {
                if(openDirectories[i] == NULL)
                {
                    handle = i;
                    i=MAX_DIR_OPEN;
                }
            }
            openDirectories[handle] = dirToOpen;
            if(debug)
            {
                printf("Abriu diretorio %s\n", openDirectories[handle]->name);
            }
            dirIndex++;
        }
        else
        {
            printf("Arquivo nao eh do tipo diretorio!\n");
            return -2;
        }

        return handle;
    }

}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    if(!inicializado)
    {
        init();
    }

    if(openDirectories[handle] != NULL)
    {
        dentry->fileSize = sizeof(DIRENT2) + openDirectories[handle]->numFilhos;
        dentry->fileType = openDirectories[handle]->fileType;
        strcpy(dentry->name, openDirectories[handle]->name);
        return 0;
    }
    else
    {
        return -1;
    }
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle)
{
    if(!inicializado)
    {
        init();
    }
    if(openDirectories[handle] != NULL)
    {
        printf("Closed file %s\n", openDirectories[handle]->name);
        free(openDirectories[handle]);
        openDirectories[handle] = NULL;
        dirIndex--;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (HARDLINK) para um arquivo
diretório ou regular. O bloco alvo do link está no campo bloco de dados, quando
o arquivo alvo é apagado, o link continua existindo até ser apagado também. Se
o link é apagado, nada acontece com o arquivo.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename)
{
    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };

    if(linkname[0] != '/' || strlen(linkname) < 2) // o caminho é sempre absoluto, então se não entrou / no início não tá tentando acessar diretório raiz
    {
        printf("pathname incorreto, uso: /path_to_dir\n");
        return INVALID_PATH;
    };

    DWORD bloco;
    DIRENT3 *dir = lookForDir(filename, &bloco); // procura o alvo do link
    if(dir == NULL)
    {
        printf("Erro: %s nao foi encontrado!\n", filename);
        return -1;
    }



    // cria o arquivo de link
    // pega o penúltimo diretório na hierarquia, ele é o pai do diretorio que estou removendo (no sentido se entrou /a/b, removeu /b o pai é a
    char *string = strdup(linkname); // cria uma cópia do linkname
    char *str = strtok(string, "/");
    char *straux = str;
    char *newPath = malloc(strlen(linkname));
    char *linkname2 = malloc(strlen(linkname));
    newPath[0] = '/';
    newPath[1] = '\0';
    int encontrado=0;
    while(!encontrado)
    {
        strcpy(linkname2, straux); // guarda o nome do arquivo de link
        straux = strtok(NULL,"/");

        if(straux== NULL)
        {
            if(debug)
                printf("\n ->>> %s\n", newPath);
            encontrado=1;
        }
        else
        {

            strcat(newPath, str);
            strcat(newPath, "/");
            str = straux;

        }
    }
    if(debug)
        printf("procurar %s e criar %s\n", newPath, linkname2);


    DIRENT3* link = malloc(sizeof(*link) + sizeof(DWORD));
    link->dirFilhos[0] = 0;
    link->fileType = ARQ_LINK;
    strcpy(link->name, linkname2);
    link->setorDados = bloco;
    link->fileSize = sizeof(DIRENT3);
    link->numFilhos = 0;
    DWORD blocoLink = (DWORD)aloca_bloco();
    if(blocoLink==0)
    {
        printf("Nao foi possivel alocar bloco para o link\n");
        return -4;
    }




    if(strcmp(newPath,"/")==0)
    {
        // link esta no diretorio raiz

        rootDirectory->dirFilhos[rootDirectory->numFilhos] = blocoLink;
        rootDirectory->numFilhos+=1;
        if(!writeBlock((unsigned char*)link, blocoLink)) // salvo o novo diretorio na memória
        {
            if(!writeBlock((unsigned char*)rootDirectory, 1)) // salvo o novo diretorio na memória
            {
                if (debug)
                    printf("Link %s criado com sucesso no bloco %d!\n", link->name, blocoLink);
                else
                    printf("Link %s criado com sucesso.", link->name);
                free(link);
            }
        }
        return 0;
    }


    DWORD blocoPai;
    DIRENT3 *link_pai = lookForDir(newPath, &blocoPai); // informações do pai do link
    if(link_pai == NULL || link_pai->fileType != ARQ_DIRETORIO)
    {
        printf("Nao eh possivel criar link, certifique-se de que o path eh de /dir/.../dir/link\n");
        return -3;
    }
    else
    {

        link_pai->dirFilhos[link_pai->numFilhos] = blocoLink;
        link_pai->numFilhos+=1;
        if(!writeBlock((unsigned char*)link, blocoLink)) // salvo o novo diretorio na memória
        {
            if(!writeBlock((unsigned char*)link_pai, blocoPai)) // salvo o novo diretorio na memória
            {
                if (debug)
                    printf("Link %s criado com sucesso no bloco %d!\n", link->name, blocoLink);
                else
                    printf("Link %s criado com sucesso.", link->name);
                free(link);
            }
        }


    }

    free(linkname2);
    free(newPath);
    free(dir);
    free(link_pai);
    return 0;

}

DIRENT3 *lookForDir(char* path, DWORD* bloco)
{
    if(path[0] != '/') // o caminho é sempre absoluto, então se não entrou / no início não tá tentando acessar diretório raiz
    {
        printf("pathname incorreto, uso: /path_to_dir\n");
        return NULL;
    };

    char *pathname2 = strdup(path); // não da pra pegar direto de pathname
    char *dir = strtok(pathname2, "/"); // o que procurar no diretório raiz


    DWORD novoDiretorioBloco; // bloco onde está o novo diretório lido
    DIRENT3 *novoDiretorio = NULL; // novo diretório lido
    DIRENT3 *dirAuxiliar = NULL; // diretório auxiliar na procura de subdiretorios
    unsigned char *buffer = NULL; // buffer pra leitura de blocos

    // contadores e flag
    int m=0;
    int n=0;
    int encontrado = 0;

    while(rootDirectory->numFilhos > n && encontrado == 0)  // procuro nos filhos do diretório raiz o filho dir
    {
        novoDiretorioBloco = *(rootDirectory->dirFilhos+n); // o bloco onde o novoDiretorio está é no diretório raiz
        buffer = readBlock(novoDiretorioBloco);
        novoDiretorio = (DIRENT3 *)buffer;

        // comparar o diretório que estou (novodiretorio) com o que quero criar
        if(!strcmp(novoDiretorio->name, dir))
        {
            // se eles forem iguais encontrei
            encontrado = 1;
            if(debug == 1)
            {
                printf("Encontrei %s no diretorio raiz!\n", novoDiretorio->name);
            }
        }
        else
        {
            n++;

        }

    }

    // se não encontrei retorna null e desaloca memória do buffer
    if(encontrado == 0)
    {
        if(debug == 1)
        {
            printf("Diretório %s nao encontrado no diretorio raiz!\n", dir);
        }
        free(buffer);
        return NULL;
    }
    else // se encontrei procura o próximo subdiretório
    {
        dir = strtok(NULL, "/");
        encontrado = 0;
        m=0;
        while(dir != NULL) // enquanto ainda tiver subdiretórios
        {
            // procurar em novoDiretorio a entrada com nome dir
            // se encontrar -> ela é novoDiretorio
            while(m < novoDiretorio->numFilhos && encontrado == 0)  // itera por todos os filhos do novo dirretório ou até encontrar
            {
                //procurar em novoDiretorio os filhos até achar algum com nome de dir
                // le o bloco
                buffer = readBlock(novoDiretorio->dirFilhos[m]);
                dirAuxiliar = (DIRENT3 *) buffer;
                // compara o dirAuxiliar com o que estou procurando
                if(!strcmp(dirAuxiliar->name, dir))
                {
                    if(debug == 1)
                    {
                        printf("Encontrado %s como subdiretorio de %s no bloco %d\n", dir, novoDiretorio->name, novoDiretorio->dirFilhos[m]);
                    }

                    novoDiretorioBloco = novoDiretorio->dirFilhos[m];
                    encontrado = 1;
                    // se for igual encontrei
                    novoDiretorio = dirAuxiliar;
                    // ele é o novoDiretorio, para procurar o proxímo token nele
                }
                else
                {
                    m++;
                }
            }
            m=0; // para próxima iteração

            // se acabou o loop e não encontrou, desaloca buffer e retorna null
            if (encontrado == 0)
            {
                if(debug==1)
                {
                    printf("Nao encontrado subdiretorio %s\n", dir);
                }
                free(buffer);
                return NULL;
            }

            dir = strtok(NULL, "/");
            encontrado = 0;
        }
        if(bloco!=NULL)
            *bloco = novoDiretorioBloco;
        return novoDiretorio;
    }
}


