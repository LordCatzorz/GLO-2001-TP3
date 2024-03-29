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
	printf("GLOFS: Saisie bloc %d \n",BlockNum);
	WriteBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	return 1;
}

int SaisieFreeInode(UINT16 InodeNum)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	InodeFreeBitmap[InodeNum]=0;
	printf("GLOFS: Saisie i-node %d \n",InodeNum);
	WriteBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	return 1;
}

void LibererBloc(UINT16 BlockNum)
{
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	BlockFreeBitmap[BlockNum]=1;
	printf("GLOFS: Relache bloc %d \n",BlockNum);
	WriteBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
}

int LibererInode(UINT16 InodeNum)
{
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	InodeFreeBitmap[InodeNum]=1;
	printf("GLOFS: Relache i-node %d \n",InodeNum);
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
	if (strlen(pFilename)==1)
	{
		*pStat=pINodeEntry.iNodeStat;
		return 0;
	}
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
	strcpy(maTab[j],pFilename + debut);
          	 	j++;
	for (i=1;i<j;i++)
	{
		int estPresent =-1;
		char DataBlockDirEntry[BLOCK_SIZE];
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
	strcpy(maTab[j],pFilename + debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	iNodeEntry * pINERepParent;
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
				pINERepParent = pINE;
				pINodeEntry = pINE[pDE[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
		}
		if (estPresent==-1 && i!=j-1) return -1;
	}
	if (estPresent!=-1) return -2;
	
	UINT16 noInodeLibre = obtentionNoInodeLibre();
	SaisieFreeInode(noInodeLibre);
	// Initialisation de l'inode (peut etre en faire une fonction)
	// d'abord on ajoute au repertoire parent le fichier
	ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
	DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
	pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].iNode=noInodeLibre;
	strcpy(pDE[pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)].Filename,maTab[j-1]);
	//pINodeEntry.iNodeStat.st_size+=sizeof(DirEntry);
	WriteBlock(pINodeEntry.Block[0],(char *)pDE);
	ReadBlock(BASE_BLOCK_INODE+(pDE[0].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINEParent =(iNodeEntry *) InodesBlockEntry;
	pINEParent[pDE[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size+=sizeof(DirEntry);
	WriteBlock(BASE_BLOCK_INODE+(pDE[0].iNode/NUM_INODE_PER_BLOCK),(char *) pINEParent);
	//pINERepParent[pDE[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size+=sizeof(DirEntry);
	//WriteBlock(BASE_BLOCK_INODE+(pDE[0].iNode/NUM_INODE_PER_BLOCK),(char *)pINERepParent);
																														
																																						
	// on initialise l'inode du nouveau fichier
	ReadBlock(BASE_BLOCK_INODE+(noInodeLibre/NUM_INODE_PER_BLOCK),InodesBlockEntry);																														
	iNodeEntry *pINENewInode = (iNodeEntry *) InodesBlockEntry;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_ino=noInodeLibre;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |=G_IFREG;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |=G_IRWXU;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |=G_IRWXG;
	
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &=~G_IFDIR;
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
	strcpy(maTab[j],pFilename + debut);
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
			printf("Le fichier %s est inexistant! \n", pFilename);
			return -1;
		}	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFDIR)) 
	{
		printf("Le fichier %s est un repertoire! \n", pFilename);
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
		if (offset_block>=BLOCK_SIZE)
		{	
			n++;
			ReadBlock(pINodeEntry.Block[n],pDELecture);
			offset_block=0;
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
	strcpy(maTab[j],pFilename + debut);
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
			printf("Le fichier %s est inexistant! \n",pFilename);
			return -1;
		}	
	}
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFDIR)) 
	{
		printf("Le fichier %s est un repertoire! \n",pFilename);
		return -2;
	}
	if (offset>pINodeEntry.iNodeStat.st_size) 
	{
		printf("L'offset demande est trop grand pour %s!\n", pFilename);
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
		pINodeEntry.iNodeStat.st_blocks++;
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
	strcpy(maTab[j],pDirName + debut);
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
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink=2;
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_size=2*sizeof(DirEntry);
	pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].iNodeStat.st_blocks=1;
    pINENewInode[noInodeLibre%NUM_INODE_PER_BLOCK].Block[0] = noBlocLibreRepertoire;
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
	strcpy(maTab[j],pPathExistant + debut);
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
	strcpy(maTab2[j],pPathNouveauLien + debut);
          	 	j++;
	estPresent=-1;
	DirEntry * pDERepertoireDestination;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry2.Block[0],DataBlockDirEntry);
			DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
			pDERepertoireDestination=pDE;
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

	ReadBlock(BASE_BLOCK_INODE+(pDERepertoireDestination[0].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry * pINRepertoire = (iNodeEntry *) InodesBlockEntry;
	pINRepertoire[pDERepertoireDestination[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size+=sizeof(DirEntry);
	WriteBlock(BASE_BLOCK_INODE+(pDERepertoireDestination[0].iNode/NUM_INODE_PER_BLOCK),(char *)pINRepertoire);

	ReadBlock(pINodeEntry2.Block[0],DataBlockDirEntry);
	DirEntry *pDE = (DirEntry *) DataBlockDirEntry;
	pDE[pINodeEntry2.iNodeStat.st_size/sizeof(DirEntry)].iNode=pINodeEntryPathExistant.iNodeStat.st_ino;
	strcpy(pDE[pINodeEntry2.iNodeStat.st_size/sizeof(DirEntry)].Filename,maTab2[j-1]);
	pINodeEntry2.iNodeStat.st_size+=sizeof(DirEntry);
	WriteBlock(pINodeEntry2.Block[0],(char *)pDE);	

	pINodeEntryPathExistant.iNodeStat.st_nlink++;
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntryPathExistant.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINModification = (iNodeEntry *) InodesBlockEntry;
	pINModification[pINodeEntryPathExistant.iNodeStat.st_ino%NUM_INODE_PER_BLOCK]=pINodeEntryPathExistant;
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntryPathExistant.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char *)pINModification);
	return 0;
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
	strcpy(maTab[j],pFilename + debut);
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
	
	char InodesBlockEntryParent[BLOCK_SIZE];
	ReadBlock(BASE_BLOCK_INODE+(pDEntryRepParent[0].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntryParent);
	iNodeEntry *pINEParent = (iNodeEntry *) InodesBlockEntryParent;
				
	for (int h=positionDansRepertoire;h<pINEParent[pDEntryRepParent[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size/sizeof(DirEntry);h++)
	{
		strcpy(pDEntryRepParent[h].Filename,pDEntryRepParent[h+1].Filename);
		pDEntryRepParent[h].iNode=pDEntryRepParent[h+1].iNode;
	}
	pINEParent[pDEntryRepParent[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size -=sizeof(DirEntry);
	WriteBlock(BASE_BLOCK_INODE+(pDEntryRepParent[0].iNode/NUM_INODE_PER_BLOCK),(char *)pINEParent);
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
	strcpy(maTab[j],pFilename + debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	iNodeEntry *lesInodesDuBloc;
	DirEntry  *pDEntryRepParent;
	int noBlocRepertoire;
	int positionDansRepertoire;
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

	char InodesBlockEntryParent[BLOCK_SIZE];
	ReadBlock(BASE_BLOCK_INODE+(pDEntryRepParent[0].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntryParent);
	iNodeEntry *pINEParent = (iNodeEntry *) InodesBlockEntryParent;
				
	for (int h=positionDansRepertoire;h<pINEParent[pDEntryRepParent[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size/sizeof(DirEntry);h++)
	{
		strcpy(pDEntryRepParent[h].Filename,pDEntryRepParent[h+1].Filename);
		pDEntryRepParent[h].iNode=pDEntryRepParent[h+1].iNode;
	}
	pINEParent[pDEntryRepParent[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_size -=sizeof(DirEntry);
	pINEParent[pDEntryRepParent[0].iNode%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink--;
	WriteBlock(BASE_BLOCK_INODE+(pDEntryRepParent[0].iNode/NUM_INODE_PER_BLOCK),(char *)pINEParent);
	WriteBlock(noBlocRepertoire,(char *)pDEntryRepParent);

	LibererBloc(pINodeEntry.Block[0]);
	LibererInode(pINodeEntry.iNodeStat.st_ino);
	return 0;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
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
	strcpy(maTab[j],pFilename + debut);
          	 	j++;
	int estPresent;
	char DataBlockDirEntry[BLOCK_SIZE];
	int positionDansRepertoireOrigine;
	DirEntry  *pDEntryRepParentOrigine;
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
					positionDansRepertoireOrigine = l;
					pDEntryRepParentOrigine=pDE;
					noBlocRepertoire=pINodeEntry.Block[0];
				}
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
	
	iNodeEntry pINodeEntryFichier = pINodeEntry;
	int nINodeFichier = pDEntryRepParentOrigine[positionDansRepertoireOrigine].iNode;
	int nInodeRepOrigine = pDEntryRepParentOrigine[0].iNode;
	// Partie 2 On s'occupe du pPathNouveauLien
	
	// Obtention de l'i-node de   la racine.
	ReadBlock(4,InodesBlockEntry);
	iNodeEntry *pINE2 = (iNodeEntry *) InodesBlockEntry;
	iNodeEntry pINodeEntry2 = pINE2[1];
	debut = 0;
	i = 0;
	j = 0;
	char maTab2[50][200];
	while(pDestFilename[i] != '\0')
	{
    		if(pDestFilename[i] == '/')
    		{
          		 strncpy(maTab2[j],pDestFilename + debut,i-debut);//copie seulement i-debut octet à partir du debut ième caractère de fileName.
          	 	j++;
          		debut = i+1;
    		}

   		 i++;
	}
	strcpy(maTab2[j],pDestFilename + debut);
          	 	j++;
	estPresent=-1;
	DirEntry * pDERepertoireDestination;
	for (i=1;i<j;i++)
	{
		estPresent =-1;
		ReadBlock(pINodeEntry2.Block[0],DataBlockDirEntry);
		DirEntry *pDEDestination = (DirEntry *) DataBlockDirEntry;
		pDERepertoireDestination=pDEDestination;
		for (int l=0;l<(BLOCK_SIZE)/sizeof(DirEntry);l++)
		{	
			if (strcmp(maTab2[i],pDEDestination[l].Filename)==0)
			{
				estPresent=i;
				ReadBlock(BASE_BLOCK_INODE+(pDEDestination[l].iNode/NUM_INODE_PER_BLOCK),InodesBlockEntry);
				iNodeEntry *pINE2 = (iNodeEntry *) InodesBlockEntry;
				pINodeEntry2 = pINE2[pDEDestination[l].iNode%NUM_INODE_PER_BLOCK];
				break;
			}
		}
		if (estPresent==-1 && i!=j-1) return -1;
	}
	if (estPresent!=-1) return -1;

// Dans le repertoire de destination
	int noInodeRepDestination = pDERepertoireDestination[0].iNode;
	ReadBlock(BASE_BLOCK_INODE+(noInodeRepDestination/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *iNodeEntryRepDestination=(iNodeEntry *) InodesBlockEntry;
	// Ajout de l'entry du fichier
	int positionAjout = iNodeEntryRepDestination[noInodeRepDestination%NUM_INODE_PER_BLOCK].iNodeStat.st_size/sizeof(DirEntry);
	pDERepertoireDestination[positionAjout].iNode=nINodeFichier;
	strcpy(pDERepertoireDestination[positionAjout].Filename,maTab2[j-1]);
	// Augmentation taille du repertoire de destination
	iNodeEntryRepDestination[noInodeRepDestination%NUM_INODE_PER_BLOCK].iNodeStat.st_size+=sizeof(DirEntry);

	// Si le fichier à copier est repertoire alors changer son ".." et on  augmente le link du parent
	if (pINodeEntryFichier.iNodeStat.st_mode & G_IFDIR)	
	{
		char newDataBlockDirEntry[BLOCK_SIZE];
		ReadBlock(pINodeEntryFichier.Block[0],newDataBlockDirEntry);
		DirEntry * pDEFichier = (DirEntry*)newDataBlockDirEntry;
		pDEFichier[1].iNode=noInodeRepDestination;
		WriteBlock(pINodeEntryFichier.Block[0],(char *)pDEFichier);
		iNodeEntryRepDestination[noInodeRepDestination%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink++;
	}

	// Sauvegarder les changements
	WriteBlock(BASE_BLOCK_INODE+(noInodeRepDestination/NUM_INODE_PER_BLOCK),(char *)iNodeEntryRepDestination);
	WriteBlock(iNodeEntryRepDestination[noInodeRepDestination%NUM_INODE_PER_BLOCK].Block[0],(char *)pDERepertoireDestination);

// Dans le repertoire d'origine
	ReadBlock(BASE_BLOCK_INODE+(nInodeRepOrigine/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *iNodeEntryRepOrigine=(iNodeEntry *) InodesBlockEntry;
	ReadBlock(iNodeEntryRepOrigine[nInodeRepOrigine%NUM_INODE_PER_BLOCK].Block[0],DataBlockDirEntry);
	DirEntry * pDEOrigine = (DirEntry*)DataBlockDirEntry;
	int tailleRepOrigine =iNodeEntryRepOrigine[nInodeRepOrigine%NUM_INODE_PER_BLOCK].iNodeStat.st_size;

	// Si le fichier renommé est un rep , diminuer linkage du parent
	if (pINodeEntryFichier.iNodeStat.st_mode & G_IFDIR)	
	{
		iNodeEntryRepOrigine[nInodeRepOrigine%NUM_INODE_PER_BLOCK].iNodeStat.st_nlink--;
	}
	
	//Supprimer l'entrée du fichier déplacé
	int nbEntreeRepertoireOrigine = tailleRepOrigine/sizeof(DirEntry);
	for (int h=positionDansRepertoireOrigine;h<nbEntreeRepertoireOrigine;h++)
	{
		strcpy(pDEOrigine[h].Filename,pDEOrigine[h+1].Filename);
		pDEOrigine[h].iNode=pDEOrigine[h+1].iNode;
	}
	
	// Diminuer la taille du parent
	iNodeEntryRepOrigine[nInodeRepOrigine%NUM_INODE_PER_BLOCK].iNodeStat.st_size-=sizeof(DirEntry);
	
	// Sauvegarder Changements
	WriteBlock(BASE_BLOCK_INODE+(nInodeRepOrigine/NUM_INODE_PER_BLOCK),(char *)iNodeEntryRepOrigine);
	WriteBlock(iNodeEntryRepOrigine[nInodeRepOrigine%NUM_INODE_PER_BLOCK].Block[0],(char *)pDEOrigine);
	
	return 0;
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
			(*ppListeFichiers)[i].iNode=pDRepertoireACopier[i].iNode;
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
	strcpy(maTab[j],pDirLocation + debut);
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
		(*ppListeFichiers)[i].iNode=pDRepertoireACopier[i].iNode;
	}
	return pINodeEntry.iNodeStat.st_size/sizeof(DirEntry);
}

int bd_truncate(const char *pFilename, int NewSize) {
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
	strcpy(maTab[j],pFilename + debut);
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
	if (estPresent!=-1 && (pINodeEntry.iNodeStat.st_mode & G_IFDIR)) 
		return -2;
	if (pINodeEntry.iNodeStat.st_size<=NewSize)
		return pINodeEntry.iNodeStat.st_size;
	pINodeEntry.iNodeStat.st_size=NewSize;
	if (NewSize==0)
	{
		for (int i=0;i<pINodeEntry.iNodeStat.st_blocks;i++)
		{
			LibererBloc(pINodeEntry.Block[i]);
		}
		pINodeEntry.iNodeStat.st_blocks=0;
	}
	lesInodesDuBloc[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK]=pINodeEntry;
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char *)lesInodesDuBloc);
	
	return NewSize;
}

int bd_formatdisk() {
	// Initialiser Inodes
	char InodeFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	for (int i =0;i<BLOCK_SIZE;i++)
		InodeFreeBitmap[i]=1;
	WriteBlock(FREE_INODE_BITMAP,InodeFreeBitmap);
	// Initialiser Blocs
	char BlockFreeBitmap[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	for (int i =8;i<BLOCK_SIZE;i++)
		BlockFreeBitmap[i]=1;
	WriteBlock(FREE_BLOCK_BITMAP,BlockFreeBitmap);
	SaisieFreeInode(1);
	saisieBloc(8);
	int TailleDepartRepRacine = 2*sizeof(DirEntry);

	char InodesBlockEntry[BLOCK_SIZE];
	ReadBlock(BASE_BLOCK_INODE,InodesBlockEntry);
	iNodeEntry *pINE = (iNodeEntry *) InodesBlockEntry; 
	pINE[1].iNodeStat.st_size=TailleDepartRepRacine;
	pINE[1].iNodeStat.st_nlink=2;
	WriteBlock(BASE_BLOCK_INODE,(char *)pINE);
	return 1;
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
		}
		if (estPresent==-1)
		{
			printf("Le fichier %s est inexistant! \n",pFilename);
			return -1;
		}	
	}
	ReadBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),InodesBlockEntry);
	iNodeEntry *pINEcriture = (iNodeEntry *) InodesBlockEntry;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IRWXU;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IRUSR;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IWUSR;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IXUSR;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IRWXG;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IRGRP;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IWGRP;
	pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode &= ~G_IXGRP;
	/*if (st_mode & G_IRWXU)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRWXU;
	*/if (st_mode & G_IRUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRUSR;
	if (st_mode & G_IWUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IWUSR;
	if (st_mode & G_IXUSR)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IXUSR;
	/*if (st_mode & G_IRWXG)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRWXG;
	*/if (st_mode & G_IRGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IRGRP;
	if (st_mode & G_IWGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IWGRP;
	if (st_mode & G_IXGRP)
		pINEcriture[pINodeEntry.iNodeStat.st_ino%NUM_INODE_PER_BLOCK].iNodeStat.st_mode |= G_IXGRP;
			
	WriteBlock(BASE_BLOCK_INODE+(pINodeEntry.iNodeStat.st_ino/NUM_INODE_PER_BLOCK),(char*)pINEcriture);
	
	return 0;
}
/* Cette fonction va réordonner le répertoire en ordre alphabétique 
	retourne 
		0 si aucun changement
		1 si changement
	   -1 si erreur*/
int bd_fct_perso(const char* pDirName){ //ajuster aussi les paramètres
	// Obtention de l'i-node de   la racine.
	char InodesBlockEntry[BLOCK_SIZE];
	char DataBlockDirEntry[BLOCK_SIZE];
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
	for (i=1;i<j && strlen(pDirName) > 1;i++)
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
		}
		if (estPresent==-1)
		{
			printf("Le dossier %s est inexistant! \n",pDirName);
			return -1;
		} else if(pINodeEntry.iNodeStat.st_mode & G_IFREG) {
			printf("%s n'est pas un dossier! \n",pDirName);
		}
	}
	ReadBlock(pINodeEntry.Block[0],DataBlockDirEntry);
	DirEntry * pDRepertoireACopier = (DirEntry *) DataBlockDirEntry;
	DirEntry tempEntry;
	int hasChanged = 0;
	// Bubble sort
	for(int i = 2; i < pINodeEntry.iNodeStat.st_size/sizeof(DirEntry); i++){
		for(int j = pINodeEntry.iNodeStat.st_size/sizeof(DirEntry)-1; j > i; j--){
			int solved = 0;
			for(int k = 0; solved != 1 && k < min(strlen(pDRepertoireACopier[i].Filename), strlen(pDRepertoireACopier[j].Filename)); k++)
			{
				char letterI = pDRepertoireACopier[i].Filename[k];
				char letterJ = pDRepertoireACopier[j].Filename[k];

				//Considérer les minuscules comme des majuscules
				if (letterI >= 'a' && letterI <= 'z') {
					letterI = letterI - 'a' + 'A';
				}
				if (letterJ >= 'a' && letterJ <= 'z') {
					letterJ = letterJ - 'a' + 'A';
				}

				if (letterI > letterJ) {
					tempEntry = pDRepertoireACopier[j];
					pDRepertoireACopier[j] = pDRepertoireACopier[i];
					pDRepertoireACopier[i] = tempEntry;
					solved = 1;
					hasChanged = 1;
				} else if (letterI < letterJ) {
					solved = 1;
				}
			}
			if (solved == 0 && (strlen(pDRepertoireACopier[i].Filename) > strlen(pDRepertoireACopier[j].Filename))) {
				// Fichier même texte, mais un plus long.
				tempEntry = pDRepertoireACopier[j];
				pDRepertoireACopier[j] = pDRepertoireACopier[i];
				pDRepertoireACopier[i] = tempEntry;
				solved = 1;
				hasChanged = 1;
			}
		}
	}
	if (hasChanged == 1) {
		WriteBlock(pINodeEntry.Block[0],(char*) pDRepertoireACopier);
	}
	return hasChanged;
}


