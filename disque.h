#ifndef DISQUE_H
#define DISQUE_H

// Les fonctions d'acces au disque dur. Les fonctions bd_*
// utilisent ces fonctions pour lire et ecrire sur le disque. 3e6a98197487be5b26d0e4ec2051411f
int ReadBlock(UINT16 BlockNum, char *pBuffer);
int WriteBlock(UINT16 BlockNum, const char *pBuffer);

#endif
