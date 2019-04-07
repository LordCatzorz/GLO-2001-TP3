#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

#define MAX_BLOCK_INODE ((BASE_BLOCK_INODE + (N_INODE_ON_DISK/NUM_INODE_PER_BLOCK)) -1)

// Quelques fonctions qui pourraient vous être utiles
int NumberofDirEntry(int Size) {
	return Size/sizeof(DirEntry);
}

int min(int a, int b) {
	return a<b ? a : b;
}

int max(int a, int b) {
	return a>b ? a : b;
}

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname(), que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. 
   3e6a98197487be5b26d0e4ec2051411f
*/
int GetDirFromPath(const char *pPath, char *pDir) {
	strcpy(pDir,pPath);
	int len = strlen(pDir); // length, EXCLUDING null
	int index;
	
	// On va a reculons, de la fin au debut
	while (pDir[len]!='/') {
		len--;
		if (len <0) {
			// Il n'y avait pas de slash dans le pathname
			return 0;
		}
	}
	if (len==0) { 
		// Le fichier se trouve dans le root!
		pDir[0] = '/';
		pDir[1] = 0;
	}
	else {
		// On remplace le slash par une fin de chaine de caractere
		pDir[len] = '\0';
	}
	return 1;
}

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */   
int GetFilenameFromPath(const char *pPath, char *pFilename) {
	// Pour extraire le nom de fichier d'un path complet
	char *pStrippedFilename = strrchr(pPath,'/');
	if (pStrippedFilename!=NULL) {
		++pStrippedFilename; // On avance pour passer le slash
		if ((*pStrippedFilename) != '\0') {
			// On copie le nom de fichier trouve
			strcpy(pFilename, pStrippedFilename);
			return 1;
		}
	}
	return 0;
}

/* Un exemple d'utilisation des deux fonctions ci-dessus :
int bd_create(const char *pFilename) {
	char StringDir[256];
	char StringFilename[256];
	if (GetDirFromPath(pFilename, StringDir)==0) return 0;
	GetFilenameFromPath(pFilename, StringFilename);
	                  ...
*/


/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode) {
	printf("\t\t========= inode %d ===========\n",iNode.iNodeStat.st_ino);
	printf("\t\t  blocks:%d\n",iNode.iNodeStat.st_blocks);
	printf("\t\t  size:%d\n",iNode.iNodeStat.st_size);
	printf("\t\t  mode:0x%x\n",iNode.iNodeStat.st_mode);
	int index = 0;
	for (index =0; index < N_BLOCK_PER_INODE; index++) {
		printf("\t\t      Block[%d]=%d\n",index,iNode.Block[index]);
	}
}

/* Cette fonction prendre en paramètre un numéro de block
   Elle retourne:
     -1 Si problème de lecture du bloc.
     -2 Si le numéro de bloc est hors des valeurs possibles.
      0 Si ok et retourne un tableau de dirEntry dans le paramètre outDirTable alloué à l'adresse du pointé par le pointeur. (Need free)*/
int getDirBlockFromBlockNumber(const UINT16 blockNumber, DirEntry** outDirTable)
{
	if ((blockNumber > MAX_BLOCK_INODE) && (blockNumber <= N_BLOCK_ON_DISK))
	{
		char dataRawBlock[BLOCK_SIZE];
		if (ReadBlock(blockNumber, dataRawBlock) > 0) {
			*outDirTable = (DirEntry*)malloc(BLOCK_SIZE);
			memcpy(*outDirTable, dataRawBlock, BLOCK_SIZE);
			return 0;
		} else {
			return -1; //Bad read
		}
	} else {
		return -2; // Out of bound blockNumber.
	}
}

int writeDirBlockToBlockNumber(const UINT16 blockNumber, DirEntry* dirTable)
{
	if ((blockNumber > MAX_BLOCK_INODE) && (blockNumber <= N_BLOCK_ON_DISK))
	{
		if (WriteBlock(blockNumber, (char*)(dirTable)) > 0)
		{
			return 0;
		} else {
			return -1; //Bad read
		}
	} else {
		return -2; // Out of bound blockNumber.
	}
}

/* Cette contion prendre en paramètre un numéro de block
   Elle retourne:
       -1 Si problème de lecture du block.
       -2 Si le blockNumber est hors de l'interle
        0 Si ok et retourne un tableau d'InodeEntry alloué à l'adresse du pointé par le pointeur. (Need free)*/
int getINodeBlockFromBlockNumber(const UINT16 blockNumber, iNodeEntry** outINodeTable)
{
	if ((blockNumber >= BASE_BLOCK_INODE) && (blockNumber <= MAX_BLOCK_INODE))
	{
		char iNodeRawBlock[BLOCK_SIZE];
		if (ReadBlock(blockNumber, iNodeRawBlock) > 0)
		{
			*outINodeTable = (iNodeEntry*)malloc(BLOCK_SIZE);
			memcpy(*outINodeTable, iNodeRawBlock, BLOCK_SIZE);
			return 0;
		} else {
			return -1; //Bad read
		}
	} else {
		return -2; // Out of bound blockNumber.
	}
}

int getINodeBlockNumberFromINodeNumber(const UINT16 inodeNumber) {
	return  BASE_BLOCK_INODE + (inodeNumber / NUM_INODE_PER_BLOCK);
}

int writeINodeEntry(const iNodeEntry* iNodeEntryToSave)
{
	int blockNumber = getINodeBlockNumberFromINodeNumber(iNodeEntryToSave->iNodeStat.st_ino);
	if ((blockNumber >= BASE_BLOCK_INODE) && (blockNumber <= MAX_BLOCK_INODE))
	{
		iNodeEntry* iNodeTable;
		if (getINodeBlockFromBlockNumber(blockNumber, &iNodeTable) < 0)
		{
			return -1; //Bad Read
		}

		memcpy(&iNodeTable[iNodeEntryToSave->iNodeStat.st_ino % NUM_INODE_PER_BLOCK], iNodeEntryToSave, sizeof(iNodeEntry));


		if (WriteBlock(blockNumber, (char*)iNodeTable) > 0) {
			free(iNodeTable);
			return 0;
		} else {
			free(iNodeTable);
			return -1; //Bad read
		}
	} else {
		return -2; // Out of bound blockNumber.
	}
	return -1;
}

/* Cette fonction prendre en paramètre un numéro d'i-node.
   Elle retourne:
      -2 Si un problème de lecture sur le disque.
      -1 Si ce numéro n'est pas valide.
      0  Si ce numéro n'est pas assignée
      1  Si ce numéro est assignée */
int getINodeNumberIsAssigned(const UINT16 iNodeNumber)
{
	if (iNodeNumber <= 0 || iNodeNumber > N_INODE_ON_DISK) {
		return -1; // Invalid INodeNumber
	}
	char bitmapFreeINode[BLOCK_SIZE];
	if (ReadBlock(FREE_INODE_BITMAP, bitmapFreeINode) > 0)
	{
		return bitmapFreeINode[iNodeNumber] == 0 ? 1 : 0; 
	} else
	{
		return -2; // Bad read.
	}
}

/* Cette fonction prend en paramètre un iNodeNumber.
   Elle retourne
       -1 Si ce numéro de i-node n'est pas valide.
       -2 Si ce numéro de i-node n'est pas assignée.
       -3 Si un problème de lecture est survenu.
        0 Si le iNode est valide et outiNodeEntry contient ce i-node */
int getINodeEntryFromINodeNumber(const UINT16 iNodeNumber, iNodeEntry* outiNodeEntry)
{
	switch (getINodeNumberIsAssigned(iNodeNumber))
	{
		case -1:
			return -1; // Invalid iNode
		case 0:
			return -2; // Non Assigne
		default:
			break;//Continue
	}
	
	char iNodeBlockNumber = getINodeBlockNumberFromINodeNumber(iNodeNumber);
	char iNodeInBlockOffset = iNodeNumber% NUM_INODE_PER_BLOCK;

	iNodeEntry* iNodeTable;
	switch (getINodeBlockFromBlockNumber(iNodeBlockNumber, &iNodeTable))
	{
		case -2:
			return -1; // Invalid iNode
		case -1:
			return -3; // Bad read
		default:
			break;//Continue
	}
	memcpy(outiNodeEntry,&(iNodeTable[iNodeInBlockOffset]), sizeof(iNodeEntry));
	free(iNodeTable);
	return 0;
}


/* Cette fonction prend en paramètre un iNode d'un répertoire et le nom d'un fichier.
    Elle retourne 
       -2 si problème de lecture du bloc.
       -1 si le fichier n'Existe pas
        0 Si le fichier est trouvé et outINodeNumber contient l'inode de ce fichier.*/
int getFileInodeNumberFromDirectoryINode(const iNodeEntry* parentInode, const char* fileString, UINT16* outINodeNumber)
{
	DirEntry* dirEntryTable;
	switch (getDirBlockFromBlockNumber(parentInode->Block[0], &dirEntryTable))
	{
		case -1:
		case -2:
			return -2; // Invalid read
		default:
			break;//Continue
	}

	int numberOfChildren = NumberofDirEntry(parentInode->iNodeStat.st_size);

	for (size_t i = 0; i < numberOfChildren; i++)
	{
		if (strcmp(dirEntryTable[i].Filename, fileString) ==0) {
			*outINodeNumber = dirEntryTable[i].iNode;
			free(dirEntryTable);
			return 0;
		}
	}
	free(dirEntryTable);
	return -1;
}

/* Cette fonction prend en paramètre un chemin de dossier/fichier.
   Elle retourne:
       0 Si le iNode a bien été trouvé, et affecte ce numéro d'inode dans le paramètre iNodeNumber
      -1 Si le chemin est invalide.
*/
int getINodeNumberOfPath(const char *pPath, UINT16* iNodeNumber){
	if ((strlen(pPath)>0) && (strcmp(pPath, "/")==0)){
		*iNodeNumber=ROOT_INODE; //Is Root
		return 0;
	} 
	char fileString[FILENAME_SIZE];
	switch (GetFilenameFromPath(pPath, fileString))
	{
		case 0:
			return -1; //Invalid path
		default:
			break;//Continue
	}
	char dirString[strlen(pPath) - strlen(fileString)];

	switch (GetDirFromPath(pPath, dirString))
	{
		case 0:
			return -1; //Invalid path
		default:
			break;//Continue
	}
	//Permet de sauter le premier caractère.
	//Obtenir l'iNode du parent
	switch(getINodeNumberOfPath(dirString, iNodeNumber))
	{
		case -1:
			return -1; //Invalid path
		case -2:
			return -2; // Invalid read;
		default:
			break;//Continue
	}
	iNodeEntry parentInode;
	switch (getINodeEntryFromINodeNumber(*iNodeNumber, &parentInode))
	{
		case -1:
		case -2:
		case -3: 
			return -2; // Invalid read
		default:
			break;//Continue
	}

	switch(getFileInodeNumberFromDirectoryINode(&parentInode, fileString, iNodeNumber))
	{
		case 0:
			return 0;
		case -1:
			return -1; // File Not found
		case -2:
			return -2; // Invalid read

	}
}

/* Cette fonction prendre un numéro d'inode en paramètre et va indiquer qu'il est réservé.
   Cette fonction retourne 
    -1 s'il y a un problème
     1 sinon. */
int TakeINode(UINT16 InodeNumber)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	if (ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap) < 1) {
		return -1;
	}
	InodeFreeBitmap[InodeNumber]=0; //This inode is now taken
	if (WriteBlock(FREE_INODE_BITMAP, InodeFreeBitmap) < 1) {
		return -1;
	};
	printf("Saisie i-node %d \n", InodeNumber);
	return 1;
}

/* Cette fonction prendre un numéro d'inode en paramètre et va indiquer qu'il est libre.
   Cette fonction retourne 
    -1 s'il y a un problème
     1 sinon. */
int FreeINode(UINT16 InodeNumber)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
	if (ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap) < 1) {
		return -1;
	}
	InodeFreeBitmap[InodeNumber]=1; //This inode is now free
	if (WriteBlock(FREE_INODE_BITMAP, InodeFreeBitmap) < 1) {
		return -1;
	};
	printf("Relache i-node %d \n", InodeNumber);
	return 1;
}

/* Cette fonction prendre un numéro de bloc en paramètre et va indiquer qu'il est réservé.
   Cette fonction retourne 
    -1 s'il y a un problème
     1 sinon. */
int TakeBloc(UINT16 BlockNum)
{
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
	if (ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) < 1) {
		return -1;
	}
	BlockFreeBitmap[BlockNum]=0; //Bloc is now taken
	if (WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) < 1) {
		return -1;
	}
	printf("Saisie bloc %d \n",BlockNum);
	return 1;
}

/* Cette fonction prendre un numéro de bloc en paramètre et va indiquer qu'il est libre.
   Cette fonction retourne 
    -1 s'il y a un problème
     1 sinon. */
int FreeBloc(UINT16 BlockNum)
{
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
	if (ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) < 1) {
		return -1;
	}
	BlockFreeBitmap[BlockNum] = 1; //Bloc is now free
	if (WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) < 1) {
		return -1;
	}
	printf("Relache bloc %d \n", BlockNum);
	return 1;
}

/* Cette fonction prendre un entré d'inode et libérer tous les blocs mémoires associées.
   Cette fonction retourne 
    -1 s'il y a un problème
     1 sinon. */
int FreeFile(iNodeEntry* fileInode)
{
	for(UINT16 iBloc = 0; iBloc < fileInode->iNodeStat.st_blocks; iBloc++)
	{
		FreeBloc(fileInode->Block[iBloc]);
	}
	FreeINode(fileInode->iNodeStat.st_ino);
}


/*private var*/
UINT16 _reserveNewINodeNumber_nextCheckPosition = 2;

/* Cette fonction réserve un retourne un numéro d'iNode libre.
	Retourne -1 si aucun inode libre. */
UINT16 ReserveNewINodeNumber(){
	char iNodeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP, iNodeBitmap);
	for(UINT16 iNodeOffset = 0; iNodeOffset < N_INODE_ON_DISK; iNodeOffset++) {
		UINT16 checkPosition = _reserveNewINodeNumber_nextCheckPosition + iNodeOffset;
		if (checkPosition == N_INODE_ON_DISK) {
			// Reinitialise position if going over the limit. (Round Robin)
			checkPosition = 0;
		} 

		if (iNodeBitmap[checkPosition] == 1) {
			// Reserve this block inode.
			if (TakeINode(checkPosition) != 1)
			{
				return -2; //Problem setting the value.
			}
			// Memorize where to check next time;
			_reserveNewINodeNumber_nextCheckPosition = checkPosition + 1;
			return checkPosition;
		}
	}
	return -1;
}

/*private var*/
UINT16 _reserveNewINodeBlock_nextCheckPosition = 2;

/* Cette fonction réserve un retourne un numéro de bloc libre.
   Retourne 
     -1 si aucun bloc libre.
     -1 si probleme de reservation */
UINT16 ReserveNewBlockNumber(){
	char blockBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, blockBitmap);
	for(UINT16 blockOffset = 0; blockOffset < N_BLOCK_ON_DISK; blockOffset++) {
		UINT16 checkPosition = _reserveNewINodeBlock_nextCheckPosition + blockOffset;
		if (checkPosition == N_BLOCK_ON_DISK) {
			// Reinitialise position if going over the limit. (Round Robin)
			checkPosition = 0;
		} 

		if (blockBitmap[checkPosition] == 1) {
			// Reserve this block number.
			if (TakeBloc(checkPosition) != 1)
			{
				return -2; //Problem setting the value.
			}
			// Memorize where to check next time;
			_reserveNewINodeBlock_nextCheckPosition = checkPosition + 1;
			return checkPosition;
		}
	}
	return -1;
}


/* cette fnction prend un iNode de répertoire, un iNode de fichier et un nom de fichier.
	Elle va modifier la mémoire associé au iNode de répertoire pour ajouter ce nouveau fichier.
	Elle va aussi modifier les pointeurs d'inode en paramètre, mais ne va pas les enregistrer.
	Elle retourne
		-1 si problème de lecture ou d'écriture.
		0  sinon.
*/
int AddFileInDir(iNodeEntry* dirInode, iNodeEntry* fileInode, const char* endFileName) {
	DirEntry* dirEntryTable;
	if (getDirBlockFromBlockNumber(dirInode->Block[0], &dirEntryTable) != 0)
	{
		return -1; // Problem reading block.
	}
	
	UINT16 position = NumberofDirEntry(dirInode->iNodeStat.st_size);
	dirEntryTable[position].iNode = fileInode->iNodeStat.st_ino;
	strcpy(dirEntryTable[position].Filename, endFileName);

	dirInode->iNodeStat.st_size += sizeof(DirEntry);
	fileInode->iNodeStat.st_nlink ++;

	// Écrire les données dans le block du répertoire.
	if (writeDirBlockToBlockNumber(dirInode->Block[0], dirEntryTable) != 0)
	{
		free(dirEntryTable);
		return -1; // Problem writing block.
	};

	free(dirEntryTable);
	return 0;
}

/* cette fnction prend un iNode de répertoire, un iNode de fichier et un nom de fichier.
	Elle va modifier la mémoire associé au iNode de répertoire pour retirer ce fichier.
	Elle va aussi modifier les pointeurs d'inode en paramètre, mais ne va pas les enregistrer.
	Elle retourne
		-2 si fichier pas trouvé.
		-1 si problème de lecture ou d'écriture.
		0  sinon.
*/
int RemoveFileFromDir(iNodeEntry* dirInode, iNodeEntry* fileInode, const char* fileName)
{
	DirEntry* dirEntryTable;
	if (getDirBlockFromBlockNumber(dirInode->Block[0], &dirEntryTable) != 0)
	{
		return -1; // Problem reading block.
	}
	UINT16 nbDir = NumberofDirEntry(dirInode->iNodeStat.st_size);

	int trouve = 0;
	for(UINT16 iDir = 0; iDir < NumberofDirEntry(dirInode->iNodeStat.st_size); iDir++)
	{
		if (trouve == 1)
		{// Si on a trouvé le nom du fichier, descendre ma position d'un bloc.
			dirEntryTable[iDir - 1] = dirEntryTable[iDir];
		} else if (strcmp(dirEntryTable[iDir].Filename, fileName) == 0)
		{
			trouve = 1;
		}
	} 
	// Écrire les données dans le block du répertoire.
	if (writeDirBlockToBlockNumber(dirInode->Block[0], dirEntryTable) != 0)
	{
		free(dirEntryTable);
		return -1; // Problem writing block.
	};

	if (trouve == 0)
	{
		return -2; // File not found in dir
	}

	dirInode->iNodeStat.st_size -= sizeof(DirEntry);
	fileInode->iNodeStat.st_nlink--;
}

/*
	Cette fonction prend un chemin en parametre, et retourne
	ce chemin divisé en "cheminParent" et "fichier".
	Retourne
		-1 si le chemin n''est pas valide
		1  sinon
*/
int splitPath(const char* pPath, char* pParentPath, char* pFile) {
	
	switch(GetFilenameFromPath(pPath, pFile))
	{
		case 0: 
			return -1;
		case 1:
			break; //continue
	}
	switch(GetDirFromPath(pPath, pParentPath))
	{
		case 0: 
			return -1;
		case 1:
			break; //continue
	}

	return 1;
}

/* Cette fonction prendre un iNodeEntry en paramètre.
	Elle retourne
		0 si c'est un fichier.
	 	-1 sinon.*/
int InodeEntryIsFile(iNodeEntry* node)
{
	if (node->iNodeStat.st_mode & G_IFREG)
	{
		return 0;
	} 
	else 
	{
		return -1;
	}
}

/* Cette fonction prendre un iNodeEntry en paramètre.
	Elle retourne
		0 si c'est un répertoire.
	 	-1 sinon.*/
int InodeEntryIsDirectory(iNodeEntry* node)
{
	if (node->iNodeStat.st_mode & G_IFDIR)
	{
		return 0;
	} 
	else 
	{
		return -1;
	}
}

/* Cette fonction prend un chemin.
	Elle retourne
		-1 Si le chemin jusqu'au "parent" est invalide
		0  Si le chemin jusqu'au "parent" est valide, mais pas la dernière section.
			parentINodeEntry contient alors l'inodeEntry de ce parent et pEndFileName la string du nom terminal
		1  Si le chemin en entier est valide.
			parentINodeEntry contient alors l'inodeEntry du parent
			et endFileINodeEntry celui de la terminaison et pEndFileName la string du nom terminal
*/
int splitPathToInodeEntry(const char* pPath, iNodeEntry* parentINodeEntry, iNodeEntry* endFileINodeEntry, char* pEndFileName)
{
		// Check if path valid
	char pathTo[strlen(pPath)];

	switch (splitPath(pPath, pathTo, pEndFileName))
	{
		case 0:
			break;
		case -1:
			return -1;
	}
	
	// Check if folder exists.
	UINT16 parentINodeNumber;
	switch (getINodeNumberOfPath(pathTo, &parentINodeNumber))
	{
		case -1 :
			return -1; //Dest folder doesnt exist.
		case 0:
			break; //continue
	}

	// Fetch parent inodeEntry
	if (getINodeEntryFromINodeNumber(parentINodeNumber, parentINodeEntry) != 0)
	{
		return -1;
	}

	// Check if file exists.
	UINT16 endFileInodeNumber;
	if(getFileInodeNumberFromDirectoryINode(parentINodeEntry, pEndFileName, &endFileInodeNumber) != 0)
	{
		return 0; //File do not exists
	}

	if (getINodeEntryFromINodeNumber(endFileInodeNumber, endFileINodeEntry) != 0)
	{
		return 0;
	}
	return 1;
}

/* ----------------------------------------------------------------------------------------
	C'est votre partie, bon succès!
	3e6a98197487be5b26d0e4ec2051411f
   ---------------------------------------------------------------------------------------- */

int bd_countusedblocks(void) {
	UINT16 usedblock = 0;
	char bitmapFreeBlock[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, bitmapFreeBlock);
	for(UINT16 i = 0; i < N_BLOCK_ON_DISK; i++)
	{
		usedblock += bitmapFreeBlock[i] == 0 ? 1 : 0;
	}
	return usedblock;
}

int bd_stat(const char *pFilename, gstat *pStat) {
	UINT16 iNodeNumber = -1;
	switch (getINodeNumberOfPath(pFilename, &iNodeNumber))
	{
		case -1:
			return -1; // Invalid path
		default:
			break; // Continue 
	}
	iNodeEntry fileiNodeEntry;
	switch(getINodeEntryFromINodeNumber(iNodeNumber, &fileiNodeEntry))
	{
		case -1:
		case -2:
		case -3:
			return -2; // Internal error
		default:
			break;// Continue
	}
	*pStat = fileiNodeEntry.iNodeStat;
	return 0;
}

int bd_create(const char *pFilename) {
	iNodeEntry parentDirectoryINodeEntry;
	iNodeEntry fileInodeEntry; // Should not be initialised in splitPath.
	char endFilename[FILENAME_SIZE];
	switch (splitPathToInodeEntry(pFilename, &parentDirectoryINodeEntry, &fileInodeEntry, endFilename))
	{
		case -1:
			return -1; //Invalid path
		case 0:
			break; //Continue
		case 1:
			return -2; //File already exists
	}

	UINT16 fileInodeNumber;
	// Get new i-Node number
	fileInodeNumber = ReserveNewINodeNumber();
	if (fileInodeNumber == -1) {
		return -1; // Can't add no more files.
	}

	// Get it's i-Node entry
	if (getINodeEntryFromINodeNumber(fileInodeNumber, &fileInodeEntry) != 0) {
		return -1; // Error getting the iNodeEntry
	}

	// Initialiser un fichier complètement vide.
	fileInodeEntry.iNodeStat.st_ino = fileInodeNumber;
	fileInodeEntry.iNodeStat.st_nlink = 0;
	fileInodeEntry.iNodeStat.st_size = 0;
	fileInodeEntry.iNodeStat.st_blocks = 0;
	fileInodeEntry.iNodeStat.st_mode = 0;
	fileInodeEntry.iNodeStat.st_mode |= G_IFREG;
	fileInodeEntry.iNodeStat.st_mode |= G_IRWXU;
	fileInodeEntry.iNodeStat.st_mode |= G_IRWXG;

	if (AddFileInDir(&parentDirectoryINodeEntry, &fileInodeEntry, endFilename) != 0)
	{
		return -1; // Error setting the value.
	}

	writeINodeEntry(&parentDirectoryINodeEntry);
	writeINodeEntry(&fileInodeEntry);
	
	return 0;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
	iNodeEntry parentDirectoryINodeEntry; //Unused
	iNodeEntry fileInodeEntry;
	char endFilename[FILENAME_SIZE]; //Unused
	switch (splitPathToInodeEntry(pFilename, &parentDirectoryINodeEntry, &fileInodeEntry, endFilename))
	{
		case -1:
		case 0:
			printf("Le fichier %s est inexistant!\n", pFilename);
			return -1;
		case 1:
			break; //continue
	}

	if(InodeEntryIsDirectory(&fileInodeEntry) == 0)
	{
		printf("Le fichier %s est un répertoire!\n", pFilename);
		return -2; //Path is directory
	}

	UINT16 nbOctetLu = 0;
	const UINT16 nombreMaxBloc = fileInodeEntry.iNodeStat.st_blocks;
	UINT16 blocEnCours = offset / BLOCK_SIZE;
	
	UINT16 nbOctetRestantAvantFin = min(fileInodeEntry.iNodeStat.st_size - offset, numbytes); // Ne pas lire plus que la taille du fichier
	UINT16 nbOctetToRead = 0;

	while(nbOctetRestantAvantFin > 0 && blocEnCours < nombreMaxBloc)
	{
		UINT16 innerBlockOffset =  blocEnCours == 0? offset % BLOCK_SIZE : 0 ;
		char readblock[BLOCK_SIZE];
		ReadBlock(fileInodeEntry.Block[blocEnCours], readblock);

		nbOctetToRead = min(nbOctetRestantAvantFin, BLOCK_SIZE - innerBlockOffset);
		memcpy(&(buffer[nbOctetLu]), &readblock[innerBlockOffset], nbOctetToRead);

		nbOctetRestantAvantFin -= nbOctetToRead;
		nbOctetLu += nbOctetToRead;
		blocEnCours++;
	}

	return nbOctetLu;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
	iNodeEntry parentDirectoryINodeEntry; //Unused
	iNodeEntry fileInodeEntry;
	char endFilename[FILENAME_SIZE]; //Unused
	switch (splitPathToInodeEntry(pFilename, &parentDirectoryINodeEntry, &fileInodeEntry, endFilename))
	{
		case -1:
		case 0:
			printf("Le fichier %s est inexistant!\n", pFilename);
			return -1;
		case 1:
			break; //continue
	}

	if(InodeEntryIsDirectory(&fileInodeEntry) == 0)
	{
		printf("Le fichier %s est un répertoire!\n", pFilename);
		return -2; //Path is directory
	}

	if (offset > fileInodeEntry.iNodeStat.st_size)
	{
		printf("L'offset demande est trop grand!\n");
		return -3; // Offset plus grand que la taille du fichier.
	}

	if(offset > (N_BLOCK_PER_INODE * BLOCK_SIZE))
	{
		return -4; // Offset plus grand que la taille maximal d'un fichier.
	}

	if((offset + numbytes) > (N_BLOCK_PER_INODE * BLOCK_SIZE))
	{
		printf("Le fichier %s deviendra trop gros!\n", pFilename); // Taille final du fichier dépasse la limite du système. Continuer tout de même.
	}

	UINT16 blocEnCours = offset / BLOCK_SIZE;

	UINT16 nbOctetRestantAvantFin = min((N_BLOCK_PER_INODE * BLOCK_SIZE)- offset, numbytes); // Ne pas lire plus que la taille du fichier
	UINT16 nbOctetToRead = 0;
	UINT16 nbOctetWritten = 0;

	while(nbOctetRestantAvantFin > 0)
	{
		UINT16 innerBlockOffset =  nbOctetWritten == 0? offset % BLOCK_SIZE : 0 ;
		nbOctetToRead = min(nbOctetRestantAvantFin, BLOCK_SIZE - innerBlockOffset);
		char readblock[BLOCK_SIZE];
		if (blocEnCours >= fileInodeEntry.iNodeStat.st_blocks) // Si on veut écrire sur un block qui n'existe pas.
		{
			fileInodeEntry.iNodeStat.st_blocks++;
			fileInodeEntry.Block[blocEnCours] = ReserveNewBlockNumber();
		} 
		else // Sinon, si on va chercher le texte de l'ancien bloc.
		{
			ReadBlock(fileInodeEntry.Block[blocEnCours], readblock);
		}
		//Copier du buffer, puis enregistrer.
		memcpy(&readblock[innerBlockOffset], &(buffer[nbOctetWritten]), nbOctetToRead);
		WriteBlock(fileInodeEntry.Block[blocEnCours], readblock);
		
		nbOctetRestantAvantFin -= nbOctetToRead;
		nbOctetWritten += nbOctetToRead;
		blocEnCours++;
	}
	fileInodeEntry.iNodeStat.st_size = max(offset + nbOctetWritten, fileInodeEntry.iNodeStat.st_size); // Agrandir la stat de taille, si texte ajouté.

	//Sauvegarder les nouvelles données du fichier.
	writeINodeEntry(&fileInodeEntry);
	return nbOctetWritten;
}

int bd_mkdir(const char *pDirName) {
	iNodeEntry parentDirectoryINodeEntry;
	iNodeEntry endFolderInodeEntry; // Should not be initialised in splitPath.
	char endFolderName[FILENAME_SIZE];
	switch (splitPathToInodeEntry(pDirName, &parentDirectoryINodeEntry, &endFolderInodeEntry, endFolderName))
	{
		case -1:
			return -1; //Invalid path
		case 0:
			break; //Continue
		case 1:
			return -2; //File already exists
	}
	UINT16 endFolderInodeNumber;
	// Get new i-Node number
	endFolderInodeNumber = ReserveNewINodeNumber();
	if (endFolderInodeNumber == -1) {
		return -1; // Can't add no more files.
	}

	// Get it's i-Node entry
	if (getINodeEntryFromINodeNumber(endFolderInodeNumber, &endFolderInodeEntry) != 0) {
		return -1; // Error getting the iNodeEntry
	}

	// Initialiser un fichier complètement vide.
	endFolderInodeEntry.iNodeStat.st_ino = endFolderInodeNumber;
	endFolderInodeEntry.iNodeStat.st_nlink = 0;
	endFolderInodeEntry.iNodeStat.st_size = 0;
	endFolderInodeEntry.iNodeStat.st_mode = 0;
	endFolderInodeEntry.iNodeStat.st_mode |= G_IFDIR;
	endFolderInodeEntry.iNodeStat.st_mode |= G_IRWXU;
	endFolderInodeEntry.iNodeStat.st_mode |= G_IRWXG;
	endFolderInodeEntry.iNodeStat.st_blocks = 1;
	endFolderInodeEntry.Block[0] = ReserveNewBlockNumber();


	//Ajouter dans parent vers enfant.
	if (AddFileInDir(&parentDirectoryINodeEntry, &endFolderInodeEntry, endFolderName) != 0)
	{
		return -1; // Error setting the value.
	}
	//Ajouter lien sur soit même enfant.
	if (AddFileInDir(&endFolderInodeEntry, &endFolderInodeEntry, ".") != 0)
	{
		return -1; // Error setting the value.
	}
	//Ajouter lien dans enfant vers parent.
	if (AddFileInDir(&endFolderInodeEntry, &parentDirectoryINodeEntry, "..") != 0)
	{
		return -1; // Error setting the value.
	}

	writeINodeEntry(&parentDirectoryINodeEntry);
	writeINodeEntry(&endFolderInodeEntry);
	
	return 0;
}
	
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {	
	// Validation origine
	// Check if file exists.
	UINT16 srcFileINodeNumber;
	switch (getINodeNumberOfPath(pPathExistant, &srcFileINodeNumber))
	{
		case -1 :
			return -1; //File not exists
		case 0:
			break; //continue
	}

	iNodeEntry srcFileINodeEntry;
	if (getINodeEntryFromINodeNumber(srcFileINodeNumber, &srcFileINodeEntry) != 0)
	{
		return -1; //File not exists
	}

	if ((InodeEntryIsDirectory(&srcFileINodeEntry) == 0))
	{
		return -3; // is folder, not file
	}

	iNodeEntry destParentDirectoryINodeEntry;
	iNodeEntry destFileInodeEntry; // Should not be initialised in splitPath.
	char destFilename[FILENAME_SIZE];
	switch (splitPathToInodeEntry(pPathNouveauLien, &destParentDirectoryINodeEntry, &destFileInodeEntry, destFilename))
	{
		case -1:
			return -1; //Invalid path
		case 0:
			break; //Continue
		case 1:
			return -2; //File already exists
	}

	// Add link to original inode in new folder with a new name.
	AddFileInDir(&destParentDirectoryINodeEntry, &srcFileINodeEntry, destFilename);

	writeINodeEntry(&destParentDirectoryINodeEntry);
	writeINodeEntry(&srcFileINodeEntry);

	return 0;
}

int bd_unlink(const char *pFilename) {
	iNodeEntry parentDirectoryINodeEntry;
	iNodeEntry fileInodeEntry;
	char filename[FILENAME_SIZE];
	switch (splitPathToInodeEntry(pFilename, &parentDirectoryINodeEntry, &fileInodeEntry, filename))
	{
		case -1:
		case 0:
			return -1; //Invalid path
		case 1:
			break; //Continue
	}

	if (InodeEntryIsFile(&fileInodeEntry) != 0)
	{
		return -2; // Not a file.
	}

	switch (RemoveFileFromDir(&parentDirectoryINodeEntry, &fileInodeEntry, filename))
	{
		case 0:
			break; //continue
		case -1:
		case -2:
			return -1; //File not found in dir/Internal Error
	}

	if (fileInodeEntry.iNodeStat.st_nlink == 0)
	{
		FreeFile(&fileInodeEntry);
	}

	writeINodeEntry(&parentDirectoryINodeEntry);
	writeINodeEntry(&fileInodeEntry);

	return 0;
}

int bd_rmdir(const char *pFilename) {
	iNodeEntry parentDirectoryINodeEntry;
	iNodeEntry endFolderINodeEntry;
	char endFolderName[FILENAME_SIZE];
	switch (splitPathToInodeEntry(pFilename, &parentDirectoryINodeEntry, &endFolderINodeEntry, endFolderName))
	{
		case -1:
		case 0:
			return -1; //Invalid path
		case 1:
			break; //Continue
	}

	if (InodeEntryIsDirectory(&endFolderINodeEntry) != 0)
	{
		return -2; // Not a file.
	}

	if (NumberofDirEntry(endFolderINodeEntry.iNodeStat.st_size) != 2)
	{
		return -3; // N'a pas que deux fichiers.
	}

	RemoveFileFromDir(&parentDirectoryINodeEntry, &endFolderINodeEntry, endFolderName);
	FreeFile(&endFolderINodeEntry); //Libérer Inodes et blocs

	writeINodeEntry(&parentDirectoryINodeEntry);
	//Pas besoin d'enregistrer l'enfant, puisque tout a été libéré.
	return 0;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
	return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	// Check if folder exists.
	UINT16 directoryINodeNumber;
	switch (getINodeNumberOfPath(pDirLocation, &directoryINodeNumber))
	{
		case -1 :
			return -1; //Dest folder doesnt exist.
		case 0:
			break; //continue
	}

	iNodeEntry directoryINodeEntry;
	if (getINodeEntryFromINodeNumber(directoryINodeNumber, &directoryINodeEntry) != 0)
	{
		return -1;
	}

	if(InodeEntryIsDirectory(&directoryINodeEntry) == -1)
	{
		return -1; // Not a directory
	}

	if (getDirBlockFromBlockNumber(directoryINodeEntry.Block[0], ppListeFichiers) != 0)
	{
		return -1; // Problem reading block.
	}
	return NumberofDirEntry(directoryINodeEntry.iNodeStat.st_size);
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_formatdisk() {
	return -1;
}

int bd_chmod(const char *pFilename, UINT16 st_mode) {
	UINT16 inodeNumber;
	switch (getINodeNumberOfPath(pFilename, &inodeNumber))
	{
		case -1 :
			return -1; //Invalid name
		case 0:
			break; //continue
	}

	iNodeEntry fileNodeEntry;
	if (getINodeEntryFromINodeNumber(inodeNumber, &fileNodeEntry) != 0)
	{
		return -2; //Internal error
	}
	
	// Réinitialiser les drapaux de permissions.
	fileNodeEntry.iNodeStat.st_mode &= ~G_IRWXU;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IRUSR;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IWUSR;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IXUSR;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IRWXG;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IRGRP;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IWGRP;
	fileNodeEntry.iNodeStat.st_mode &= ~G_IXGRP;

	// Affecter les drapeaux des permissions.
	fileNodeEntry.iNodeStat.st_mode |= (G_IRWXU & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IRUSR & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IWUSR & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IXUSR & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IRWXG & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IRGRP & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IWGRP & st_mode);
	fileNodeEntry.iNodeStat.st_mode |= (G_IXGRP & st_mode);

	writeINodeEntry(&fileNodeEntry);
}

int bd_fct_perso(){ //ajuster aussi les paramètres
	return -1;
}