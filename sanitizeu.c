// sanitize file and folder names by stripping unwanted characters
// won't touch any files/folders starting with .
// v2.1 by Antoni Sawicki <as@tenoware.com>

#define STRIPCFG "anu_-./+"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <libgen.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

unsigned int files=0, folders=0, changed=0;

void error(char *msg, ...) {
    va_list ap;
    char buff[1024]={0};

    va_start(ap, msg);
    vsnprintf(buff, sizeof(buff), msg, ap);
    va_end(ap);

    perror(buff);

    exit(1);
}

int strip(char *str, int len, char *allow) {
    int n,a;
    int alpha=0, number=0, spctou=0;
    char *dst;

    if(!str || !strlen(str) || !allow || !strlen(allow))
        return -1;

    if(*allow == 'a') {
        alpha=1;
        allow++;
    }
    if(*allow == 'n') {
        number=1;
        allow++;
    }
    if(*allow == 'u') {
        spctou=1;
        allow++;
    }
    if(*allow == '!') {
        allow++;
    }

    dst=str;

    for(n=0; n<len && *str!='\0'; n++, str++) {
        if(alpha && isalpha(*str))
            *(dst++)=*str;
        else if(number && isdigit(*str))
            *(dst++)=*str;
        else if(spctou && *str==' ')
            *(dst++)='_';
        else if(strlen(allow))
            for(a=0; a<strlen(allow); a++) 
                if(*str==allow[a])
                    *(dst++)=*str;
    }


    *dst='\0';

    return 0;
}

int recurse(char *name) {
    DIR *dir;
    struct dirent *e;
    struct stat f;
    struct stat t; 
    char *n;
    char *m;
    int l=0;

    n=calloc(1, FILENAME_MAX);
    if(!n)
        error("Unable to allocate memory at %s", name);

    m=calloc(1, FILENAME_MAX);
    if(!n)
        error("Unable to allocate memory at %s", name);

    dir=opendir(name);
    if(!dir)
        error("%s", name);

    while((e=readdir(dir))!=NULL) { 
        if(e->d_name[0]=='.')
            continue;

        snprintf(n, FILENAME_MAX, "%s/%s", name, e->d_name);

        if(stat(n, &f)==0) {
            strlcpy(m, n, FILENAME_MAX);
            strip(m, FILENAME_MAX, STRIPCFG);
            
            if(strcmp(n,m)!=0) {
                changed++;
                printf("Renaming [%s] to [%s]\n", n, m);
                if(rename(n,m)!=0) {
                    if(stat(m, &t)==0) { // retry with different name
                        l=strlen(m); m[l]='_'; m[l+1]='\0';
                        printf("...exists, rather to [%s]\n", m);
                        if(rename(n,m)!=0)
                            error("[%s]->[%s]", n,m);
                    }
                    else {
                        error("[%s]->[%s]", n,m);
                    }
                }
            }

            if(f.st_mode & S_IFDIR) {
                folders++;
                recurse(m);
            }
            else {
                files++;
            }
        }
    }

    free(e);
    free(n);
    free(m);
    closedir(dir);

    return 0;
}

int main(int argc, char **argv) {

    if(argc!=2)
        error("Usage: %s <topdir>\n\n", argv[0]);

    recurse(argv[1]);

    printf("Done.\n"
           "Files  : %u\n"
           "Folders: %u\n"
           "Changed: %u\n",
           files, folders, changed);
                    
    return 0;
}
