#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>

#define PATH "/tmp/spool/"
#define PATH_V "/tmp/spool/verrou"

int proprietaire_fichier(char *path,char *nom) //le path du fichier à comparer et le nom
{
    struct stat owner_fich;
    int retstat;

    retstat = stat(path,&owner_fich); //recup les infos du fichier
    if(retstat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    struct passwd *pwd;

    pwd=getpwuid(owner_fich.st_uid);

    if(!strcmp(pwd->pw_name,nom))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

char *proprietaire_pro() //recupere le nom de l'utilisateur qui execute le programme
{
    static char nom[512];

    struct passwd *owner;

    owner = getpwuid(getuid());

    strcpy(nom,owner->pw_name);

    return nom;
}

int retirer (char *nom1)
{
    int fd_verrou;
    struct stat stat_verrou;
    int retstat;
    int retlock;
    int retunlink;

    fd_verrou = open(PATH_V,O_CREAT|O_RDWR,S_IRWXU); //creation du verrou
    if(fd_verrou == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    retstat = stat(PATH_V,&stat_verrou);
    if(retstat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    retlock=lockf(fd_verrou,F_LOCK,stat_verrou.st_size);
    if(retlock == -1)
    {
        perror("lockf");
        exit(EXIT_FAILURE);
    }

    char pathfich[2048]; //path du fichier de données

    char pathfichi[2048]; //path du fichier des infos

    strcpy(pathfich,PATH);
    strcat(pathfich,nom1);

    strcpy(pathfichi,PATH);
    strcat(pathfichi,"i");
    strcat(pathfichi,nom1);

    struct stat stat_fich;
    struct stat stat_fichi;
    char nom_p[512];

    if(stat(pathfich,&stat_fich) == 0) //si le stat est ok
    {

        strcpy(nom_p,proprietaire_pro()); //recuperation du nom du proprio du programme lancé

        if(proprietaire_fichier(PATH,nom_p) || proprietaire_fichier(pathfich,nom_p))
        {
            retunlink = unlink(pathfich);
            if(retunlink == -1)
            {
                perror("unlink");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fprintf(stderr,"Vous n'etes pas le proprietaire\n");
        }
    }
    else
    {
        fprintf(stderr,"Fichier non reconnu\n");
        exit(EXIT_FAILURE);
    }


    if(stat(pathfichi,&stat_fichi) == 0)
    {

        strcpy(nom_p,proprietaire_pro()); //recuperation du nom du proprio du programme lancé

        if(proprietaire_fichier(PATH,nom_p) || proprietaire_fichier(pathfich,nom_p))
        {
            retunlink = unlink(pathfichi);
            if(retunlink == -1)
            {
                perror("unlink");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fprintf(stderr,"Vous n'etes pas le proprietaire du fichier\n");
        }
    }
        else
        {
            fprintf(stderr,"Impossible de trouver le fichier d'information i%s\n",nom1);
        }

        retlock=lockf(fd_verrou,F_ULOCK,stat_verrou.st_size);
        if(retlock == -1)
        {
            perror("lockf");
            exit(EXIT_FAILURE);
        }

         close(fd_verrou);

        retunlink = unlink(PATH_V);
        if(retunlink == -1)
        {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

        return 0;
    }

    int main(int argc,char *argv[])
    {

        if(argc == 1)
        {
            fprintf(stderr,"erreur sur le nombre d'arguments, au moins 1 fichier à retirer\n");
            exit(EXIT_FAILURE);
        }
        else  if(argc == 2)
        {
            if(argv[1] == NULL)
            {
                fprintf(stderr,"Path non reconnu ou invalide\n");
            }
            else
            {
                retirer(argv[1]);
            }
        }
        else
        {
            int i = 1;

            while(argv[i] != NULL)
            {
                retirer(argv[i++]);
            }
        }
        return 0;
    }

