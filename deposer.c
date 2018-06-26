#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <pwd.h>

#define PATH "/tmp/spool/"
#define PATH_V "/tmp/spool/verrou"
#define tailleBuf 2048

char *deposerd (char *nom1)
{
    int fd_verrou;
    struct stat stat_verrou;
    int retstat;
    int retlock;
    int retunlink;
    int retchmod;
    char buffer[tailleBuf];

    fd_verrou = open(PATH_V,O_CREAT|O_RDWR,S_IRWXU); //creation du fichier verrou
    if(fd_verrou == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    retstat = stat(PATH_V,&stat_verrou); //stat pour le verrou
    if(retstat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    retlock=lockf(fd_verrou,F_LOCK,stat_verrou.st_size); //application du verrou
    if(retlock == -1)
    {
        perror("lockf");
        exit(EXIT_FAILURE);
    }

    //ouverture premier fichier en lecture

    struct stat buf; //taille en off_t (int)

    static char pathfich[2048];

    if(getcwd(pathfich,UCHAR_MAX) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    strcat(pathfich,"/");
    strcat(pathfich,nom1); //creation du path du fichier à copier

    int ret_stat = stat (pathfich,&buf); //recup du stat
    if(ret_stat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    int fic1;

    fic1=open(pathfich,O_RDONLY);
    if (fic1==-1)
    {
        perror("open");
        exit(1);
    }

    //traitement du fichier de données arrivée

    //Creation des templates pour mkstemp
    char template[2048];

    int fdd; //fd des des données fichier d'arrivée de la copie

    strcpy(template,PATH);
    strcat(template,"d");
    strcat(template,"XXXXXX"); //template pour les données

    fdd = mkstemp(template); //creation du fichier d'arrivée
    retchmod = chmod(template,S_IRWXU); //definition des droits
    if(retchmod == -1)
    {
        perror("chmod");
        exit(EXIT_FAILURE);
    }

    strcat(pathfich,"/");
    strcat(pathfich,basename(template)); //copie le nom du nouveau fichier

    int n;
    int p;

    while ((n=read(fic1,&buffer,1)) > 0)
    {
        p = write(fdd,&buffer,n);
        if(p == -1)
        {perror("write");
            exit(EXIT_FAILURE);}
    }

    close(fic1);
    close(fdd);

    retlock=lockf(fd_verrou,F_ULOCK,stat_verrou.st_size);
    if(retlock == -1)
    {
        perror("lockf");
        exit(EXIT_FAILURE);
    }

    retunlink = unlink(PATH_V);
    if(retunlink == -1)
    {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    return pathfich;
}

void deposeri(char *nom2)
{
    char path[2048]; //path des infos
    char pathd[2048]; //path des données
    struct stat stat_verrou;
    int fd_verrou;
    int retstat;
    int retlock;
    int retunlink;
    int retchmod;

    fd_verrou = open(PATH_V,O_CREAT|O_RDWR,S_IRWXU); //creation du fichier verrou
    if(fd_verrou == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    retstat = stat(PATH_V,&stat_verrou); //stat pour le verrou
    if(retstat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    retlock=lockf(fd_verrou,F_LOCK,stat_verrou.st_size); //application du verrou
    if(retlock == -1)
    {
        perror("lockf");
        exit(EXIT_FAILURE);
    }

    strcpy(path,PATH);
    strcat(path,"i");
    strcat(path,basename(nom2)); //path du fichier dans lequel ecrire (infos)

    strcpy(pathd,PATH);
    strcat(pathd,basename(nom2));

    char tabinfo[2048]; //tableau qui regroupe les infos sur le job

    struct passwd *pws;
    pws = getpwuid(geteuid());

    //debut de l'ajout des infos

    char tabtemp[512];

    strcpy(tabtemp,basename(pathd)); //on copie le nom job du fichier sans le 'd'
    ssize_t taille_log;

    taille_log = strlen(tabtemp); //on recupere la taille (en nb carac) du nom du fich

    int i,j;

    for(i=1,j=0;i<taille_log && j<taille_log-1;i++,j++)
    {
        tabinfo[j] = tabtemp[i]; //on copie en ometant le 'd'
    }

    strcat(tabinfo,"!");

    char buffer[2048];

    time_t timestamp = time(NULL);

    strftime(buffer, sizeof(buffer), "%a %b %d %X %Y", localtime(&timestamp));

    strcat(tabinfo,buffer); //copie de la date dans tabinfo

    strcat(tabinfo,"!"); //separateur (strtok)

    strcat(tabinfo,pws->pw_name); //on copie l'id du propriétaire dans info
    strcat(tabinfo,"!");

    strcat(tabinfo,basename(dirname(nom2)));//on copie le nom du fichier réel dans les infos
    strcat(tabinfo,"!");

    int ret_stat;
    struct stat  buffer_stat;

    ret_stat = stat(pathd,&buffer_stat);
    if(ret_stat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    long int taille = buffer_stat.st_size;

    sprintf(tabtemp,"%ld", taille); //copie de la taille dans un tableau temporaire

    strcat(tabinfo,tabtemp);
    strcat(tabinfo,"!");

    FILE *fp;

    fp = fopen(path,"w+");
    retchmod = chmod(path,S_IRWXU);
    if(retchmod == -1)
    {
        perror("chmod");
        exit(EXIT_FAILURE);
    }


    fputs(tabinfo,fp); //copie dans le fichier.

    fclose(fp);

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

    return;
}

int main(int argc,char *argv[])
{
    static char nom[2048];

    if(argc == 1)
    {
        fprintf(stderr,"erreur sur le nombre d'arguments, au moins 1 fichier à deposer\n");
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
            strcpy(nom,deposerd(argv[1]));
            printf("%s\n",basename(nom));
            deposeri(nom);
        }
    }
    else
    {
        int i = 1;

        while(argv[i] != NULL)
        {
            strcpy(nom,deposerd(argv[i++]));
            printf("%s\n",basename(nom));
            deposeri(nom);
        }
    }

    return 0;
}

