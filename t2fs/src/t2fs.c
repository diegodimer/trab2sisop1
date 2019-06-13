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

ROOTDIR *rootDirectory;
int inicializado = 0;
int debug = 1;
DWORD setoresPorBloco=2;
int bloco_livre =0;



// Funções auxiliares
int writeBlock(unsigned char *, DWORD); // função que escreve em um bloco
unsigned char* readBlock(DWORD); // função que lê um bloco e retorna um ponteiro para esse bloco
int aloca_bloco(); // função que aloca um bloco
void libera_bloco(int ); // função que desaloca um bloco (preenche com 0s)
int init(); // função que inicia o sistema (preenche variaveis necessárias para execução
DIRENT3 *lookForDir(char*); // função que procura um diretório (não deve ser chamada por usuários)

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
            rootDirectory->dirFilhos[0] = novoDiretorioBloco;
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
                printf("Diretorio %s criado com sucesso!\n", dir);
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
                    printf("Diretorio %s criado com sucesso!\n%s eh subdiretorio de %s\n", novoSubDiretorio->name, novoSubDiretorio->name, novoDiretorio->name);
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
    return -1;
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
    DIRENT3 *pathDir = lookForDir(pathname);
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
    if(!inicializado)
    {
        init();
    }
    if(dirIndex >= MAX_DIR_OPEN)
    {
        printf("muitos diretorios abertos\n");
        return -1;
    }
    DIRENT3 *dirToOpen = lookForDir(pathname);

    if(dirToOpen == NULL)
    {
        return -1;
    }
    else
    {
        if(dirToOpen->fileType == ARQ_DIRETORIO)
        {
            openDirectories[dirIndex] = dirToOpen;
            printf("Opened file %s\n", openDirectories[dirIndex]->name);
            dirIndex++;
        }

        return 0;
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
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename)
{
    return -1;
}

DIRENT3 *lookForDir(char* path)
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
                        printf("Encontrado %s como subdiretorio de %s\n", dir, novoDiretorio->name);
                    }
                    encontrado = 1;
                    // se for igual encontrei
                    novoDiretorio = dirAuxiliar;
                    novoDiretorioBloco = *(novoDiretorio->dirFilhos+m);
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

        return novoDiretorio;
    }
}


