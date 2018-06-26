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
#include <dirent.h>
#include <getopt.h>
#include <ctype.h>

#define PATH "/tmp/spool/"
#define PATH_V "/tmp/spool/verrou"
#define PATH_I "/tmp/spool/log"

void demon(int dflag)
{
    int fd_verrou;
    struct stat stat_verrou;
    int retstat;
    int retlock;
    int retunlink;
    int code_retour;

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

    //fin du verrou

    //recuperation du path du job

    DIR *fdd; //fdd du dossier des jobs
    struct dirent *struct_dos; //structure du dossier
    FILE *fich; //fdd du fichier du log

    char pathf[1024]; //path du fichier en cours d'analyse
    char pathi[1024]; //path du fichier des infos

    int fd; //fd du fichier de log

    fd = open(PATH_I,O_CREAT|O_TRUNC|O_WRONLY,S_IRWXU); //ouverture/creation du log
    if( fd == -1)
    {
        if(dflag)
            printf("open du log non ok\n");
        perror("open");
        exit(EXIT_FAILURE);
    }

    fdd=opendir(PATH); //ouverture du dossier du spool
    if(fdd == NULL)
    {
        if(dflag)
            printf("opendir du spool non ok\n");
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while((struct_dos = readdir(fdd)) != NULL) //while général de parcours
    {

        if(!strcmp(struct_dos->d_name,"verrou")) //on evite le verrou
            continue;

        if(struct_dos->d_name[0] == 'i') //on evite les fichiers d'info
            continue;

        if(!strcmp(struct_dos->d_name,".") || !strcmp(struct_dos->d_name,"..")) //. et ..
            continue;

        if(!strcmp(struct_dos->d_name,"log")) //on evite le verrou
            continue;

        strcpy(pathf,PATH);
        strcat(pathf,struct_dos->d_name); //reconstruction du path du fichier de données en cours

        strcpy(pathi,PATH);
        strcat(pathi,"i");
        strcat(pathi,struct_dos->d_name); //reconstruction du path du fichier d'info correspondant

        //fork pour le gzip
        pid_t pid;
        int raison;

        switch(pid = fork())
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0: //le fils

            execl("/bin/gzip","gzip",pathf,NULL);
            if(dflag)
                printf("exec non ok\n");
            exit(EXIT_FAILURE);

        default:

            if(wait(&raison) == -1)
            {perror("wait");
                if(dflag)
                    printf("wait non ok\n");
                exit(EXIT_FAILURE);}

            if(WIFEXITED(raison))
            {
                code_retour = WEXITSTATUS(raison);
            }
            else if(WIFSIGNALED(raison))
            {
                code_retour = WTERMSIG(raison);
            }
        }

        //on place dans buffer la date de traitement du zip
        char buffer[2048];

        time_t timestamp = time(NULL);

        strftime(buffer, sizeof(buffer), "%a %b %d %X %Y", localtime(&timestamp));

        fich = fopen(pathi, "rb"); //ouverture du fichier d'infos avec fopen
        if(fich == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char tabtemp[2048]; //tableau qui récupere les infos en vrac

        char tabinfos[5][512]; //tableau qui recupere les infos apres le strtok

        long length = 0;

        fseek(fich,0,SEEK_END);
        length = ftell(fich);
        fseek(fich,0,SEEK_SET);

        int retfread;

        retfread = fread(tabtemp,1,length,fich);
        if(retfread == 0)
        {
            perror("fread");
            exit(EXIT_FAILURE);
        }

        const char s[2] = "!";
        char *token;

        token = strtok(tabtemp,s); //separation des chaines du tableau
        strcpy(tabinfos[0],token);
        strcat(tabinfos[0],"\0");

        int i = 1;

        while(token != NULL && i < 5) //decouper
        {
            strcpy(tabinfos[i++],strtok(NULL,s));
        }

        fclose(fich);
        //ICI ON A RECUP LES INFOS DANS TABTEMP

        int ret_stat;
        struct stat gzip;

        strcat(pathf,".gz"); //on recupere le path du fichier .gz

        ret_stat = stat(pathf,&gzip); //on recupere la taille du fichier .gz avec stat
        if(ret_stat == -1)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        FILE *f = fopen(PATH_I, "ab");

        fprintf(f,"id=%s orgdate=%s user=%s file=%s",tabinfos[0],tabinfos[1],tabinfos[2],tabinfos[3]);
        fprintf(f," orgsize=%s\ncurdate=%s gzipsize=%ld exit=%d \n",tabinfos[4],buffer,gzip.st_size,code_retour);

        fclose(f);

        retunlink = unlink(pathi);
        if(retunlink == -1)
        {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

        retunlink = unlink(pathf);
        if(retunlink == -1)
        {
            perror("unlink");
            exit(EXIT_FAILURE);
        }

    }

    retlock=lockf(fd_verrou,F_ULOCK,stat_verrou.st_size);
    if(retlock == -1)
    {
        perror("lockf");
        exit(EXIT_FAILURE);
    }

    close(fd_verrou);
    close(fd);
    closedir(fdd);

    retunlink = unlink(PATH_V);
    if(retunlink == -1)
    {
        perror("unlink");
        exit(EXIT_FAILURE);

        return;
    }
}

int main(int argc, char *argv[])
{
    int dflag = 0;
    int fflag = 0;
    int iflag = 0;
    char *ivalue = NULL;
    int secondes = 5;
    int index;
    int c; //jusqu'ici getopt

    opterr = 0;

    while((c = getopt (argc,argv,"dfi:")) != -1)
    {
        switch(c)
        {
        case'd':
            dflag = 1;
            break;
        case'f':
            fflag = 1;
            break;
        case'i':
            iflag = 1;
            ivalue = optarg;
            secondes = atoi(ivalue);
            break;
        case '?':
            if(optopt == 'i')
                fprintf(stderr,"Option -%c requiert un argument.\n",optopt);
            else if (isprint (optopt))
                fprintf (stderr,"Option inconnue `-%c'.\n", optopt);
            else
                fprintf (stderr,"Caractere d'option inconnu' `\\x%x'.\n", optopt);
            return 1;

        default:
            abort();
        }
    }

    for (index = optind; index < argc; index++)
    {
        printf ("Non-option argument %s\n", argv[index]);
        exit(EXIT_FAILURE);
    }

    //fin du getopt flags initialisés

    //on place dans buffer la date de depart du daemon
    char bufferd[2048];

    time_t timestamp = time(NULL);

    strftime(bufferd, sizeof(bufferd), "%a %b %d %X %Y", localtime(&timestamp));

    FILE *f = fopen(PATH_I, "wb");
    chmod(PATH_I,S_IRWXU);
    fwrite(bufferd, sizeof(char), sizeof(bufferd), f);
    fclose(f);

    while(1)
    {
        if(!iflag && !fflag)
        {
            pid_t pid;
            int raison;

            switch(pid = fork())
            {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);

            case 0:
                execl("/home/arnaud/Bureau/Projet_SE_2k7/demon","demon",argv[1],argv[2],argv[3],argv[4],NULL);
                perror("exec");
                exit(EXIT_FAILURE);

            default :

                if(wait(&raison) == -1)
                {perror("wait");
                    exit(EXIT_FAILURE);}
            }

        }

        else if(iflag && !fflag)
        {
            pid_t pid;
            int raison;

            switch(pid = fork())
            {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);

            case 0:
                execl("/home/arnaud/Bureau/Projet_SE_2k7/demon","demon",argv[1],argv[2],argv[3],argv[4],NULL);
                perror("exec");

            default :

                if(wait(&raison) == -1)
                {perror("wait");
                    exit(EXIT_FAILURE);}
            }
            sleep(secondes);
        }

        else if(!iflag && fflag)
        {
            demon(dflag);
        }

        else if(iflag && fflag)
        {
            demon(dflag);
            sleep(secondes);
        }
    }

    return 0;
}

