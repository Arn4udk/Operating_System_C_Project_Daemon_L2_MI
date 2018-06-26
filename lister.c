#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <libgen.h>

#define PATH "/tmp/spool/"
#define PATH_V "/tmp/spool/verrou"

int proprietaire_fichier(char *path,char *nom) //le path du fichier à comparer et le nom
{
    struct stat owner_fich;
    int retstat;
    char *proprio;

    retstat = stat(path,&owner_fich); //recup les infos du fichier
    if(retstat == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    struct passwd *pwd;

    pwd=getpwuid(owner_fich.st_uid);

    proprio = pwd->pw_name;

    if(!strcmp(proprio,nom))
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

int lister(int lflag,int uflag,char *utilisateur) //verifier les droits, seul le proprietaire peut lister les jobs
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

    //ici

    DIR *fdd; //fd du dossier des jobs
    struct dirent *struct_dos; //struct du dossier

    fdd = opendir(PATH); //ouverture du dossier contenant les jobs
    if(fdd == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    FILE *fich; //fdd du fichier ou on copie les infos
    char path[1024]; //path du fichier en cours d'analyse

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

        strcpy(path,PATH);
        strcat(path,"i");
        strcat(path,struct_dos->d_name); //reconstruction du path du fichier d'info correspondant

        struct stat owner;
        int retstat_owner;

        retstat_owner = stat(path,&owner);
        if(retstat_owner == -1)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        fich = fopen(path, "rb"); //ouverture du fichier d'infos avec fopen
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

        fclose(fich);

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

        if(lflag && !uflag) //si on a le flag du nom de fichier
        {
            printf("%s d%s %s %s\n",tabinfos[3],tabinfos[0],tabinfos[2],tabinfos[1]);
        }

        if(uflag && !lflag) //si on a le flag d'utilisateur
        {
            if(!strcmp(utilisateur,tabinfos[2]))
            {
                printf("d%s %s %s\n",tabinfos[0],tabinfos[2],tabinfos[1]);
            }
        }

        if(lflag && uflag) //nom de fichier et utilisateur
        {
            if(!strcmp(utilisateur,tabinfos[2]))
            {
                printf("%s d%s %s %s\n",tabinfos[3],tabinfos[0],tabinfos[2],tabinfos[1]);
            }
        }

        if(!lflag && !uflag)
        {
            printf("d%s %s %s\n",tabinfos[0],tabinfos[2],tabinfos[1]);
        }

    }

    closedir(fdd);

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

int main(int argc,char **argv)
{

    int lflag = 0;
    int uflag = 0;
    char *uvalue = NULL;
    int index;
    int c; //jusqu'ici getopt

    opterr = 0;

    while((c = getopt (argc,argv,"lu:")) != -1)
    {
        switch(c)
        {
        case'l':
            lflag = 1;
            break;
        case'u':
            uflag = 1;
            uvalue = optarg;
            break;
        case '?':
            if(optopt == 'u')
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
        printf ("Non-option argument %s\n", argv[index]);

    //verification des droits

    char nom_proc[512];

    strcpy(nom_proc,proprietaire_pro());

    if(lflag && uflag)
    {
        if(proprietaire_fichier(PATH,nom_proc))
        {
            lister(lflag,uflag,uvalue);
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr,"Vous n'etes pas le spool owner\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(lflag && !uflag)
    {
        if(proprietaire_fichier(PATH,nom_proc))
        {
            lister(lflag,0,NULL);
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr,"Vous n'etes pas le spool owner\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(uflag && !lflag)
    {
        if(proprietaire_fichier(PATH,nom_proc))
        {
            lister(0,uflag,uvalue);
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr,"Vous n'etes pas le spool owner\n");
            exit(EXIT_FAILURE);
        }
    }

    lister(0,0,NULL);

    return 0;
}

