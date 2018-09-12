/*
 * The MIT License
 *
 * Copyright 2018 Schuemi.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 The SDK does not know anything about relative paths. To get the original fMSX sources to compile without modifying them, I had to overload some file operations. 
 */
#include "LibOdroidGo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>


char* buffer;
char* fullCurrentDir;

struct dirent dirInfo;
struct dirent firstDirEntry;
bool haveFirstDirEntry = false;

bool hasExt(const char *file, const char *Ext) {
    
    const char* p = strstr(file,".");
    if (! p) return false;
    int lenFile = strlen(file);
    const char* cExt = Ext;
    
    while(cExt[0] != 0) {
        int lenExtension = strlen(cExt);
        if (lenFile < lenExtension) continue;
        p = file;
        p += lenFile;
        p -= lenExtension;
        if (!strcasecmp(p, cExt)) return true;
        cExt += strlen(cExt) + 2;
        
    }
    
    return false;
}

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
    buffer = malloc(1024);
   
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
    strncpy(buf, fullCurrentDir, size);
    return buf;
}
char* getFullPath(char* buffer, const char* fileName, int bufferLength){
    if (fileName[0] != '/'){
        getcwd(buffer, bufferLength);
        int len = strlen(buffer);
        *(buffer + strlen(buffer)) = '/';
        strncpy((buffer + len + 1), fileName, bufferLength - (len + 1));
        
    } else {
        strncpy(buffer, fileName, 1024);
    }
    return buffer;
}

DIR* _opendir(const char* name)
{
    DIR* d;
    if (!strcmp(name, ".")) {
        d = opendir(getcwd(buffer, 1024));
    } else d = opendir(name);
    

    return d;

}

void _rewinddir(DIR* pdir) {
    rewinddir(pdir);
}
struct dirent* _readdir(DIR* pdir)
{
    if (telldir(pdir) == 0) {
        // the first dir I should send is the ".." dir to go one dir up.
        
        dirInfo.d_ino  = 0;
        dirInfo.d_type = DT_DIR;
        strncpy(dirInfo.d_name, "..", 3);
        
        firstDirEntry = *(readdir(pdir));
        haveFirstDirEntry = true;
        return &dirInfo;
        
    }
    if (haveFirstDirEntry){
        haveFirstDirEntry = false;
        return &firstDirEntry;
        
    }
    return readdir(pdir);
    
}
int _stat( const char *__restrict __path, struct stat *__restrict __sbuf ){

    if (!strcmp(__path, "..")) {
        __sbuf->st_mode = 0x41FF;
        return 0;
    }
    getFullPath(buffer, __path, 1024);
    return stat(buffer, __sbuf);
}

FILE* _fopen(const char *__restrict _name, const char *__restrict _type) {
    // is this a bios file?
    if (!strcmp(_name, "CMOS.ROM") || !strcmp(_name, "KANJI.ROM") || !strcmp(_name, "RS232.ROM") || !strcmp(_name, "MSXDOS2.ROM") || !strcmp(_name, "PAINTER.ROM")  || !strcmp(_name, "FMPAC.ROM") || !strcmp(_name, "MSX.ROM") || !strcmp(_name, "MSX2.ROM") || !strcmp(_name, "MSX2EXT.ROM") || !strcmp(_name, "DISK.ROM") || !strcmp(_name, "MSX2P.ROM") ||  !strcmp(_name, "MSX2PEXT.ROM")) {
        //it's a rom file, open from rom file path
        snprintf(buffer, 1024, "/sd/roms/msx/bios/%s", _name);
        return fopen(buffer, _type);
    }
    
    if (_name[0] != '/') {
        getFullPath(buffer, _name, 1024);
    } else {
        strncpy(buffer, _name, 1024);
    }
    return fopen(buffer, _type);
}
int _fclose(FILE* file) {
    fflush(file);
    return fclose(file);
}
long _telldir(DIR* pdir){
    long r = telldir(pdir);
    if (r > 0) r++;
    return r;
}
void _seekdir(DIR* pdir, long loc){
    if (loc > 0) loc--;
     seekdir(pdir, loc);
    
}
