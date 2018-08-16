/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "LibOdroidGo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


char* fullCurrentDir;


char* cutExtension(char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '.') {
            file[i] = 0;
            return file;
        }
        i--;
    }
    return file;
}
const char* getFileName(const char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '/') {
            return &file[i+1];
        }
        i--;
    }
    return file;
}


char* getPath(char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '/') {
            file[i] = 0;
            return file;
        }
        i--;
    }
    return file;
}



int initFiles(){
    fullCurrentDir = malloc(612);
    if (!fullCurrentDir){ printf("malloc fullCurrentDir failed!\n"); return 0; }
    
    strncpy(fullCurrentDir, FMSX_ROOT_GAMESDIR, 612);
    
}

int chdir(const char *path)
{
    if (path == 0) return -1;
    
    if (!strcmp(path, "..")){
        if (strlen(fullCurrentDir) > strlen(FMSX_ROOT_GAMESDIR)) getPath(fullCurrentDir);
        return 0;
    }
    if (path[0] == '/'){
        return -1;
    }
    
    int len = strlen(fullCurrentDir);
    if (len > 610) return -1;
    fullCurrentDir[len] = '/';
    fullCurrentDir[len+1] = 0;
    
    strncpy((fullCurrentDir + strlen(fullCurrentDir)), path, 610 - strlen(fullCurrentDir));
    
    return 0;
}

char *getcwd(char *buf, size_t size)
{
    snprintf(buf, size, fullCurrentDir);
    return buf;
}





