#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier DisqueVirtuel.dat.orig vers DisqueVirtuel.dat" 
cp DisqueVirtuel.dat.orig DisqueVirtuel.dat

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
./ufs ls /
./ufs ls /doc
./ufs ls /doc/tmp
./ufs ls /doc/tmp/subtmp
./ufs ls /rep
./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "Tester les cas ou ls est fait sur un repertoire non-existant ou un fichier ordinaire"
echo "--------------------------------------------------------------------"
./ufs ls /mauvais
./ufs ls /b.txt

echo
echo
echo "--------------------------------------------------------------------"
echo "Maintenant on verifie que les bons b.txt sont accédés"
echo "Les numéros d'i-nodes doivent être différents"
echo "--------------------------------------------------------------------"
./ufs stat /doc/tmp/subtmp/b.txt 
./ufs stat /b.txt 

echo
echo
echo "--------------------------------------------------------------------"
echo "Tester la commande chmod"
echo "--------------------------------------------------------------------"
./ufs chmod 760 /b.txt
./ufs ls /
./ufs chmod 430 /b.txt
./ufs ls /
./ufs chmod 170 /b.txt
./ufs ls /
./ufs chmod 610 /Bonjour
./ufs ls /


echo
echo
echo "--------------------------------------------------------------------"
echo "    test de lecture d'un repertoire, fichier inexistant ou vide"
echo "--------------------------------------------------------------------"
./ufs read /rep 0 10
./ufs read /toto.txt 0 10
./ufs read /b.txt 0 10

echo
echo
echo "--------------------------------------------------------------------"
echo "    test de tronquage"
echo "--------------------------------------------------------------------"
./ufs truncate /mauvais.txt 5
./ufs truncate /doc 6
./ufs truncate /b.txt 5
./ufs read /b.txt 0 10
echo "On doit libérer un bloc de données pour la prochaine opération"
./ufs truncate /b.txt 0

echo
echo
echo "--------------------------------------------------------------------"
echo "                  test d'ecriture de 40 caracteres"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "1234567890ABCDEFGHIJ1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
./ufs blockused

echo
echo
echo "--------------------------------------------------------------------"
echo "                          tests de lecture"
echo "--------------------------------------------------------------------"

./ufs read /b.txt 0 30
./ufs read /b.txt 0 20
./ufs read /b.txt 0 10
./ufs read /b.txt 10 30
./ufs read /b.txt 10 5

echo
echo
echo "--------------------------------------------------------------------"
echo "      test d'ecriture de 1 caracteres en milieu de fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "-" 14 
./ufs stat /b.txt
./ufs blockused  
./ufs read /b.txt 0 20

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture de 1 caracteres, mais trop loin"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "X" 41 
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "   test d'ecriture exactement après le dernier caractère du fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "+" 40 
./ufs stat /b.txt
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture augmentant la taille du fichier, mais sans saisie de nouveau bloc"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij" 40 
./ufs stat /b.txt
./ufs blockused; N_USEDBLOCK=$?;  
./ufs read /b.txt 0 60

echo
echo
echo "--------------------------------------------------------------------"
echo "  test d'ecriture qui doit provoquer la saisie de 2 nouveaux blocs"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
let "N_USEDBLOCK=$N_USEDBLOCK+2"
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés:"
./ufs blockused  

echo
echo
echo "-----------------------------------------------------------------------"
echo "  test de lecture dans le fichier plus gros, qui chevauche deux blocs"
echo "-----------------------------------------------------------------------"
./ufs read /b.txt 500 30

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs utilisés ne doit pas changer"
./ufs blockused; N_USEDBLOCK=$?;
echo -e "\nDoit réussir:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés:"
./ufs blockused
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande unlink"
echo "--------------------------------------------------------------------"
./ufs unlink /b.txt
./ufs ls /
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés, car inodes toujours détenus par hlnb.txt:"
./ufs blockused
./ufs unlink /hlnb.txt
./ufs ls /
let "N_USEDBLOCK=$N_USEDBLOCK-3"
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés, car 3 inodes ont été libérés:"
./ufs blockused
echo -e "\nDoit échouer avec -1, car /b.txt n'existe plus:"
./ufs unlink /b.txt
echo -e "\nDoit échouer avec -1, car /doc/tmp/b.txt n'existe pas:"
./ufs unlink /doc/tmp/b.txt 
./ufs unlink /doc/tmp/subtmp/b.txt 
./ufs ls /doc/tmp/subtmp
echo -e "\nDoit échouer avec -2, car /doc est un répertoire:"
./ufs unlink /doc

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande rmdir"
echo "--------------------------------------------------------------------"
./ufs blockused; N_USEDBLOCK=$?;
./ufs rmdir /rep
./ufs ls /
let "N_USEDBLOCK=$N_USEDBLOCK-1"
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés, car le fichier répertoire a été libéré:"
./ufs blockused
echo -e "\nDoit échouer avec -3, car /doc n'est pas vide:."
./ufs rmdir /doc
./ufs ls /
echo -e "\nDoit échouer avec -3, car /doc/tmp n'est pas vide:"
./ufs rmdir /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
./ufs create /Doge.wow
./ufs ls /
./ufs create /doc/tmp/new.txt 
./ufs ls /
./ufs ls /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "          Tester la fonction rename sur fichier ordinaire"
echo "--------------------------------------------------------------------"
./ufs rename /Bonjour/LesAmis.txt /Bonjour/OncleG.txt
./ufs ls /Bonjour
./ufs rename /Bonjour/OncleG.txt /DansRoot.txt
./ufs ls /Bonjour
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                Tester la création d'un répertoire"
echo "--------------------------------------------------------------------"
./ufs blockused; N_USEDBLOCK=$?;
./ufs ls /Bonjour
./ufs mkdir /Bonjour/newdir
let "N_USEDBLOCK=$N_USEDBLOCK+1"
echo -e "\nDoit afficher $N_USEDBLOCK blocs utilisés, car le fichier répertoire a utilisé un bloc:"
./ufs blockused
echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
./ufs ls /Bonjour
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction rename sur répertoire"
echo "--------------------------------------------------------------------"
./ufs ls /Bonjour
./ufs ls /doc
./ufs rename /doc/tmp /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien pour /Bonjour augmente de 1 et qu'il diminue de 1 pour /doc:"
./ufs ls /
echo -e "\nOn vérifie que le sous-réperoire tmpmv contient encore subtmp et new.txt:"
./ufs ls /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien vers ce même répertoire n'augmente pas si on répète l'opération:"
./ufs rename /Bonjour/tmpmv /Bonjour/tmpmv2
./ufs rename /Bonjour/tmpmv2 /Bonjour/tmpmv3
./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction format"
echo "--------------------------------------------------------------------"
./ufs ls /
./ufs stat /
./ufs blockused
./ufs format
./ufs ls /
./ufs stat /
./ufs blockused

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction perso"
echo "--------------------------------------------------------------------"

echo "Création de dossiers dans la racine"
./ufs mkdir /bb
./ufs mkdir /aabdgdfg
./ufs create /.aba
./ufs mkdir /babdd
./ufs create /abagg

echo "Visualisation de la racine avant"
./ufs ls /

echo "Trie de la racine (valide)"
./ufs sort /

echo "Visualisation de la racine après tri"
./ufs ls /

echo

echo "Création de dossiers dans un sous-dossier"
./ufs mkdir /bb/bb
./ufs mkdir /bb/aabdgdfg
./ufs create /bb/.aba
./ufs mkdir /bb/babdd
./ufs create /bb/abagg

echo "Visualisation du sous-dossier avant"
./ufs ls /bb

echo "Trie du sous-dossier (valide)"
./ufs sort /bb

echo "Visualisation du sous-dossier après tri"
./ufs ls /bb

echo


echo "Trie d'un fichier existant (non-valide)"
./ufs sort /bb/.aba
echo "Trie d'un fichier non-existant (non-valide)"
./ufs sort /path/existe/pas.sad


