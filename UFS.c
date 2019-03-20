#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

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


/* ----------------------------------------------------------------------------------------
	C'est votre partie, bon succès!
	3e6a98197487be5b26d0e4ec2051411f
   ---------------------------------------------------------------------------------------- */
int obtentionNoInodeLibre()
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	int compteur = 1;
	while(compteur<32){
		
		if (InodeFreeBitmap[compteur]!=0)
			return compteur;
		compteur++;
	}
	return -1;
	
}	

int obtenirIndiceNouveauBloc()
{
	char BlocFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlocFreeBitmap);
	int compteur = 8;
	while(compteur<255){
		
		if (BlocFreeBitmap[compteur]!=0)
			return compteur;
		compteur++;
	}
	return -1;
	
}

int saisieBloc(UINT16 BlockNum)
{
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	BlockFreeBitmap[BlockNum]=0;
	printf("Saisie block %d \n",BlockNum);
	WriteBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	return 1;
}

int SaisieFreeInode(UINT16 InodeNum)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	InodeFreeBitmap[InodeNum]=0;
	printf("Saisie i-node %d \n",InodeNum);
	WriteBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	return 1;
}

void LibererBloc(UINT16 BlockNum)
{
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	BlockFreeBitmap[BlockNum]=1;
	printf("Relache block %d \n",BlockNum);
	WriteBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
}

int LibererInode(UINT16 InodeNum)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	InodeFreeBitmap[InodeNum]=1;
	printf("Relache i-node %d \n",InodeNum);
	WriteBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
}
		 
// Renvoie le nombre de blocs utilise
int bd_countusedblocks(void) {
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	int compteurBlocNonLibre = 0;
	for (int i=0;i<BLOCK_SIZE;i++){
		if (BlockFreeBitmap[i]==0)
			compteurBlocNonLibre++;
	}
	return compteurBlocNonLibre;
}

int bd_stat(const char *pFilename, gstat *pStat) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	for (i=1;i<j;i++)
	{
		int estPresent =-1;
		for (int k=0;k<N_BLOCK_PER_INODE;k++) 
		{
			char DataBlockDirEntry[BLOCK_SIZE];
			ReadBlock(pINodeEntry.Block[k],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
			if (estPresent!=-1) break;
		}
		if (estPresent==-1) return -1;
	}

	*pStat=pINodeEntry.iNodeStat;

	return 0;
}

int bd_create(const char *pFilename) {
	
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		for (int k=0;k<N_BLOCK_PER_INODE;k++) 
		{
			ReadBlock(pINodeEntry.Block[k],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
			if (estPresent!=-1) break;
		}
		if (estPresent==-1 && i!=j-1) return -1;
	}
	if (estPresent!=-1) return -2;
	
	UINT16 noInodeLibre = obtentionNoInodeLibre();
	SaisieFreeInode(noInodeLibre);
	// Initialisation de l'inode (peut etre en faire une fonction)
	// d'abord on ajoute au repertoire parent le fichier
	for (int i=0;i<N_BLOCK_PER_INODE;i++)
	{
		ReadBlock(pINodeEntry.Block[i],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		if ((pINodeEntry.iNodeStat.st_size/sizeof(DirEntry))<(BLOCK_SIZE/sizeof(DirEntry)))
		{
			pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].iNode=noInodeLibre;
			strcpy(pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].Filename,maTab[j-1]);
			pINodeEntry.iNodeStat.st_size+=sizeof(DirEntry);
			WriteBlock(pINodeEntry.Block[i],(char *)pDE);
			break;																														
		}
	}																																						
	// on initialise l'inode du nouveau fichier
	ReadBlock(BASE_BLOCK_INODE+(noInodeLibre/NUM_INODE_PER_BLOCK),InodesBlockEntry);																														
	iNodeEntry *pINENewInode = (iNodeEntry *) InodesBlockEntry;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_ino=noInodeLibre;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |=G_IFREG;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink=1;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_size=0;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_blocks=0;
	WriteBlock(BASE_BLOCK_INODE+(noInodeLibre/NUM_INODE_PER_BLOCK),(char *)pINENewInode);																														
	
	return 0;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		for (int k=0;k<N_BLOCK_PER_INODE;k++) 
		{
			ReadBlock(pINodeEntry.Block[k],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
			if (estPresent!=-1) break;
		}
		if (estPresent==-1)
		{
			printf("Le fichier %s est inexistant! \n",maTab[j-1]);
			return -1;
		}	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFDIR)) 
	{
		printf("Le fichier %s est un repertoire! \n",maTab[j-1]);
		return -2;
	}
	if (offset>pINodeEntry.iNodeStat.st_size) 
	{
		return 0;
	}
	int n = offset / BLOCK_SIZE;
	int nBlocLecture = pINodeEntry.Block[n];
	int offset_block = offset % BLOCK_SIZE;
	ReadBlock(pINodeEntry.Block[n],DataBlockDirEntry);
	char *pDELecture = DataBlockDirEntry;
	int index_buffer;
	for (index_buffer=0;index_buffer<numbytes;index_buffer++)
	{
		buffer[index_buffer]=pDELecture[offset_block];
		offset_block++;
		if (offset_block>pINodeEntry.iNodeStat.st_size)
			return index_buffer;
		if (offset_block>BLOCK_SIZE)
		{	
			n++;
			ReadBlock(pINodeEntry.Block[n],pDELecture);
			offset_block=1;
		}
	}
	return index_buffer;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		for (int k=0;k<N_BLOCK_PER_INODE;k++) 
		{
			ReadBlock(pINodeEntry.Block[k],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
			if (estPresent!=-1) break;
		}
		if (estPresent==-1)
		{
			printf("Le fichier %s est inexistant! \n",maTab[j-1]);
			return -1;
		}	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFDIR)) 
	{
		printf("Le fichier %s est un repertoire! \n",maTab[j-1]);
		return -2;
	}
	if (offset>pINodeEntry.iNodeStat.st_size) 
	{
		printf("L'offset est trop grand! \n");
		return -3;
	}
	if (offset>=255) 
	{
		return -4;
	}

	if (pINodeEntry.iNodeStat.st_size==0)
	{
		int n_nouveauBloc = obtenirIndiceNouveauBloc();
		saisieBloc(n_nouveauBloc);
		pINodeEntry.Block[0]=n_nouveauBloc;
		
	}
	int n = offset / BLOCK_SIZE;
	int nBlocLecture = pINodeEntry.Block[n];
	int offset_block = offset % BLOCK_SIZE;
	ReadBlock(pINodeEntry.Block[n],DataBlockDirEntry);
	char *pDEcriture = DataBlockDirEntry;
	int buffer_index = 0;
	int octet_ecrit = 0;
	while ( pINodeEntry.iNodeStat.st_size < N_BLOCK_PER_INODE*BLOCK_SIZE && buffer_index!=strlen(buffer))
	{
		if (pINodeEntry.iNodeStat.st_size<=(offset_block+n*BLOCK_SIZE))
		{
			pINodeEntry.iNodeStat.st_size++;
			offset++;
		}
		pDEcriture[offset_block]=buffer[buffer_index];
		buffer_index++;
		offset_block++;
		if (offset_block>=BLOCK_SIZE)
		{
			WriteBlock(pINodeEntry.Block[n],pDEcriture);
			offset_block=0;
			n++;
			int n_nouveauBloc = obtenirIndiceNouveauBloc();
			saisieBloc(n_nouveauBloc);
			pINodeEntry.Block[n]=n_nouveauBloc;
			pINodeEntry.iNodeStat.st_blocks++;
			ReadBlock(pINodeEntry.Block[n],pDEcriture);
		} 
	}
	
	WriteBlock(pINodeEntry.Block[n],pDEcriture);
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINEcriture = (iNodeEntry *) InodesBlockEntry;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK] = pINodeEntry;			
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char*)pINEcriture);
	return buffer_index;
}

int bd_mkdir(const char *pDirName) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pDirName[i] != '\0')
	{
    		if(pDirName[i] == '/')
    		{
          		 strncpy(maTab[j],pDirName + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pDirName + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{	
			if (strcmp(maTab[i],pDE[l].Filename)==0)
			{
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
			
			if (estPresent!=-1) break;
		}
		if (estPresent==-1 && i!=j-1) return -1;
	}
	if (estPresent!=-1) return -2;

	UINT16 noInodeLibre = obtentionNoInodeLibre();
	SaisieFreeInode(noInodeLibre);
	// Initialisation de l'inode (peut etre en faire une fonction)
	// d'abord on ajoute au repertoire parent le fichier et on increment le nombre de lien à 1
	pINodeEntry.iNodeStat.st_nlink++;
	for (int i=0;i<N_BLOCK_PER_INODE;i++)
	{
		ReadBlock(pINodeEntry.Block[i],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		if ((pINodeEntry.iNodeStat.st_size/sizeof(DirEntry))<(BLOCK_SIZE/sizeof(DirEntry)))
		{
			pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].iNode=noInodeLibre;
			strcpy(pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].Filename,maTab[j-1]);
			pINodeEntry.iNodeStat.st_size+=sizeof(DirEntry);
			WriteBlock(pINodeEntry.Block[i],(char *)pDE);
			break;																														
		}
	}
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);																																						
	iNodeEntry *pINEParent = (iNodeEntry *) InodesBlockEntry;
	pINEParent[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK]=pINodeEntry;
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char *)pINEParent);
	
	// on initialise l'inode du nouveau fichier
	// On doit également saisir un nouveau bloc repertoire
	UINT16 noBlocLibreRepertoire = obtenirIndiceNouveauBloc();
	saisieBloc(noBlocLibreRepertoire);
	ReadBlock(noBlocLibreRepertoire,DataBlockDirEntry);																														
	DirEntry *pDENouveauBloc = (DirEntry *) DataBlockDirEntry;
	pDENouveauBloc[0].iNode=noInodeLibre;
	strcpy(pDENouveauBloc[0].Filename,".");
	pDENouveauBloc[1].iNode=pINodeEntry.iNodeStat.st_ino;
	strcpy(pDENouveauBloc[1].Filename,"..");
	WriteBlock(noBlocLibreRepertoire,(char *)pDENouveauBloc);
	ReadBlock(BASE_BLOCK_INODE+(noInodeLibre/NUM_INODE_PER_BLOCK),InodesBlockEntry);																														
	iNodeEntry *pINENewInode = (iNodeEntry *) InodesBlockEntry;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_ino=noInodeLibre;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode|=G_IFDIR;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink=1;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_size=2*sizeof(DirEntry);
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_blocks=1;
	WriteBlock(BASE_BLOCK_INODE+(noInodeLibre/NUM_INODE_PER_BLOCK),(char *)pINENewInode);																														
	return 0;

}
	
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	
	// Partie une on s'occupe de récuperer l'inodeEntry du pathExistant
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pPathExistant[i] != '\0')
	{
    		if(pPathExistant[i] == '/')
    		{
          		 strncpy(maTab[j],pPathExistant + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pPathExistant + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{				
			if (strcmp(maTab[i],pDE[l].Filename)==0)
			{
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
			
			if (estPresent!=-1) break;
		}
		if (estPresent==-1) return -1;
	}
	
	if (pINodeEntry.iNodeStat.st_mode & G_IFDIR) return -3;
	iNodeEntry pINodeEntryPathExistant = pINodeEntry;

	// Partie 2 On s'occupe du pPathNouveauLien
	
	
	// Obtention de l'i-node de   la racine.
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE2 = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry2 = pINE2[1];
	debut = 0;
	i = 0;
	j = 0;
	char maTab2[50][200];
	while(pPathNouveauLien[i] != '\0')
	{
    		if(pPathNouveauLien[i] == '/')
    		{
          		 strncpy(maTab2[j],pPathNouveauLien + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab2[j],pPathNouveauLien + debut,i-debut);
          	 	j++;
	estPresent=-1;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry2.Block[0],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab2[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE2 = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry2 = pINE2[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
		if (estPresent==-1 && i!=j-1) return -1;
	}
	if (estPresent!=-1) return -2;

	ReadBlock(pINodeEntry2.Block[0],DataBlockDirEntry);
	DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
	if ((pINodeEntry2.iNodeStat.st_size/sizeof(DirEntry))<(BLOCK_SIZE/sizeof(DirEntry)))
	{
		pDE[pINodeEntry2.iNodeStat.st_size/sizeof(DirEntry)].iNode=pINodeEntryPathExistant.iNodeStat.st_ino;
		strcpy(pDE[pINodeEntry2.iNodeStat.st_size/sizeof(DirEntry)].Filename,maTab2[j-1]);
		pINodeEntry2.iNodeStat.st_size+=sizeof(DirEntry);
		WriteBlock(pINodeEntry2.Block[0],(char *)pDE);																														
	}
		
	pINodeEntryPathExistant.iNodeStat.st_nlink++;
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntryPathExistant.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINModification = (iNodeEntry *) InodesBlockEntry;
	pINModification[pINodeEntryPathExistant.iNodeStat.st_ino%NUM_INODE_PER_BLOCK]=pINodeEntryPathExistant;
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntryPathExistant.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char *)pINModification);
	return 1;
}

int bd_unlink(const char *pFilename) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	int positionDansRepertoire;
	DirEntry  *pDEntryRepParent;
	int noBlocRepertoire;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{	
			if (strcmp(maTab[i],pDE[l].Filename)==0)
			{
				if (strcmp(maTab[j-1],pDE[l].Filename)==0) 
				{ 
					positionDansRepertoire = l;
					pDEntryRepParent=pDE;
					noBlocRepertoire=pINodeEntry.Block[0];
				}
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
		}
		if (estPresent==-1)
		{
			return -1;
		}	
	}
	if (estPresent!=-1 && !(pINodeEntry.iNodeStat.st_mode & G_IFREG)) 
	{
		return -2;
	}
	
	strcpy(pDEntryRepParent[positionDansRepertoire].Filename,"");
	pDEntryRepParent[positionDansRepertoire].iNode=0;
	WriteBlock(noBlocRepertoire,(char *)pDEntryRepParent);
	
	pINodeEntry.iNodeStat.st_nlink--;
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINodeFinal = (iNodeEntry *) InodesBlockEntry;
	pINodeFinal[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK]=pINodeEntry;
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char *)pINodeFinal);
	if (pINodeEntry.iNodeStat.st_nlink==0)
	{
		for (i=0;i<pINodeEntry.iNodeStat.st_blocks;i++)
		{
			LibererBloc(pINodeEntry.Block[i]);
		}
		LibererInode(pINodeEntry.iNodeStat.st_ino);
	}
	return 0;
}

int bd_rmdir(const char *pFilename) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	iNodeEntry *lesInodesDuBloc;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{	
			if (strcmp(maTab[i],pDE[l].Filename)==0)
			{
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
				lesInodesDuBloc=pINE;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
			
			if (estPresent!=-1) break;
		}
		if (estPresent==-1) return -1;	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFREG)) 
		return -2;
	if (pINodeEntry.iNodeStat.st_size!=2*sizeof(DirEntry))
		return -3;
	LibererBloc(pINodeEntry.Block[0]);
	LibererInode(pINodeEntry.iNodeStat.st_ino);
	return 1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
	return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	char DataBlockDirEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	if (strlen(pDirLocation)==1)
	{
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry * pDRepertoireACopier = (DirEntry *) DataBlockDirEntry;
		(*ppListeFichiers) = (DirEntry*) malloc(pINodeEntry.iNodeStat.st_size);
		for (int i=0;i<pINodeEntry.iNodeStat.st_size/sizeof(DirEntry);i++)
		{
		strcpy((*ppListeFichiers)[i].Filename,pDRepertoireACopier[i].Filename);
		}
		return pINodeEntry.iNodeStat.st_size/sizeof(DirEntry);
	}
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pDirLocation[i] != '\0')
	{
    		if(pDirLocation[i] == '/')
    		{
          		 strncpy(maTab[j],pDirLocation + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pDirLocation + debut,i-debut);
          	 	j++;
	int estPresent;
	iNodeEntry *lesInodesDuBloc;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
		DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{	
			if (strcmp(maTab[i],pDE[l].Filename)==0)
			{
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
				lesInodesDuBloc=pINE;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
			
			if (estPresent!=-1) break;
		}
		if (estPresent==-1) return -1;	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFREG)) return -1;
	ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
	DirEntry * pDRepertoireACopier = (DirEntry *) DataBlockDirEntry;
	(*ppListeFichiers) = (DirEntry*) malloc(pINodeEntry.iNodeStat.st_size);
	for (i=0;i<pINodeEntry.iNodeStat.st_size/sizeof(DirEntry);i++)
	{
		strcpy((*ppListeFichiers)[i].Filename,pDRepertoireACopier[i].Filename);
	}
	return pINodeEntry.iNodeStat.st_size/sizeof(DirEntry);
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_formatdisk() {
	return -1;
}

int bd_chmod(const char *pFilename, UINT16 st_mode) {
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry = pINE[1];
	int debut = 0;
	int i = 0;
	int j = 0;
	char maTab[50][200];
	while(pFilename[i] != '\0')
	{
    		if(pFilename[i] == '/')
    		{
          		 strncpy(maTab[j],pFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strncpy(maTab[j],pFilename + debut,i-debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		for (int k=0;k<N_BLOCK_PER_INODE;k++) 
		{
			ReadBlock(pINodeEntry.Block[k],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
			{	
				if (strcmp(maTab[i],pDE[l].Filename)==0)
				{
					estPresent=i;
					ReadBlock(BASE_BLOCK_INODE+(pDE[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
					iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry;
					pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
					break;
				}
			}
			if (estPresent!=-1) break;
		}
		if (estPresent==-1)
		{
			printf("Le fichier %s est inexistant! \n",maTab[j-1]);
			return -1;
		}	
	}
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINEcriture = (iNodeEntry *) InodesBlockEntry;
	if (st_mode & G_IRWXU)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRWXU;
	if (st_mode & G_IRUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRUSR;
	if (st_mode & G_IWUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IWUSR;
	if (st_mode & G_IXUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IXUSR;
	if (st_mode & G_IRWXG)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRWXG;
	if (st_mode & G_IRGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRGRP;
	if (st_mode & G_IWGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IWGRP;
	if (st_mode & G_IXGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IXGRP;
			
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char*)pINEcriture);
	
	return 0;
}

int bd_fct_perso(){ //ajuster aussi les paramètres
	return -1;
}


