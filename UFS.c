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
      0 Si ok et retourne un tableau de dirEntry dans le paramètre outDirTable.*/
int getDirBlockFromBlockNumber(const UINT16 blockNumber, DirEntry** outDirTable)
{
	if ((blockNumber > MAX_BLOCK_INODE) && (blockNumber <= N_BLOCK_ON_DISK))
	{
		char dataRawBlock[BLOCK_SIZE];
		if (ReadBlock(blockNumber, dataRawBlock) > 0)
		{
			*outDirTable = (DirEntry*)dataRawBlock;
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
        0 Si ok et retourne un tableau d'InodeEntry dans le paramètre de sortie.*/
int getINodeBlockFromBlockNumber(const UINT16 blockNumber, iNodeEntry** outINodeTable)
{
	if ((blockNumber >= BASE_BLOCK_INODE) && (blockNumber <= MAX_BLOCK_INODE))
	{
		char iNodeRawBlock[BLOCK_SIZE];
		if (ReadBlock(blockNumber, iNodeRawBlock) > 0)
		{
			*outINodeTable = (iNodeEntry*)iNodeRawBlock;
			return 0;
		} else {
			return -1; //Bad read
		}
	} else {
		return -2; // Out of bound blockNumber.
	}
	
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
	
	char iNodeBlockNumber = iNodeNumber / NUM_INODE_PER_BLOCK;
	char iNodeInBlockOffset = iNodeNumber % NUM_INODE_PER_BLOCK;

	iNodeEntry* iNodeTable;
	switch (getINodeBlockFromBlockNumber(BASE_BLOCK_INODE + iNodeBlockNumber, &iNodeTable))
	{
		case -2:
			return -1; // Invalid iNode
		case -1:
			return -3; // Bad read
		default:
			break;//Continue
	}
	*outiNodeEntry = iNodeTable[iNodeInBlockOffset];
	return 0;
}

/* Cette fonction prend en paramètre un chemin de dossier/fichier.
   Elle retourne:
       0 Si le iNode a bien été trouvé, et affecte ce numéro d'inode dans le paramètre iNodeNumber
      -1 Si le chemin est invalide.
*/
int getINodeNumberOfPath(const char *pPath, int* iNodeNumber){
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

	//Faire magie avec parent INode et SlashString pour trouver l'iNode du fichier en cours. (Via le DirEntry)
	DirEntry* dirEntryTable;
	switch (getDirBlockFromBlockNumber(parentInode.Block[0], &dirEntryTable))
	{
		case -1:
		case -2:
			return -2; // Invalid read
		default:
			break;//Continue
	}
	for (size_t i = 0; i < NumberofDirEntry(BLOCK_SIZE); i++)
	{
		if (strcmp(dirEntryTable[i].Filename, fileString) ==0) {
			*iNodeNumber = dirEntryTable[i].iNode;
			return 0;
		}
	}
	return -1;
}


/* ----------------------------------------------------------------------------------------
	C'est votre partie, bon succès!
	3e6a98197487be5b26d0e4ec2051411f
   ---------------------------------------------------------------------------------------- */

int bd_countusedblocks(void) {
	int usedblock = 0;
	char bitmapFreeBlock[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, bitmapFreeBlock);
	for(UINT16 i = 0; i < N_BLOCK_ON_DISK; i++)
	{
		usedblock += bitmapFreeBlock[i] == 0 ? 1 : 0;
	}
	return usedblock;
}

int bd_stat(const char *pFilename, gstat *pStat) {
	int iNodeNumber = -1;
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
	return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
	return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
	return -1;
}

int bd_mkdir(const char *pDirName) {
	return -1;
}
	
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	return -1;
}

int bd_unlink(const char *pFilename) {
	return -1;
}

int bd_rmdir(const char *pFilename) {
	return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
	return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	return -1;
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_formatdisk() {
	return -1;
}

int bd_chmod(const char *pFilename, UINT16 st_mode) {
	return -1;
}

int bd_fct_perso(){ //ajuster aussi les paramètres
	return -1;
}