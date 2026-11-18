#include "macos_savefile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void writefile(const char *p,const char *v){FILE *f=fopen(p,"wb");assert(f);assert(fwrite(v,1,strlen(v),f)==strlen(v));assert(fclose(f)==0);}
static void check(const char *p,const char *v){char b[64]={0};FILE *f=fopen(p,"rb");assert(f);fread(b,1,63,f);fclose(f);assert(!strcmp(b,v));}
int main(){char dir[]="/tmp/kfx-save-transaction-XXXXXX";assert(mkdtemp(dir));char path[1024],temp[1024],backup[1100];snprintf(path,sizeof(path),"%s/game.sav",dir);snprintf(temp,sizeof(temp),"%s/pending",dir);
 writefile(temp,"one");assert(kfx_commit_save(temp,path));check(path,"one");
 const char *versions[]={"two","three","four","five"};for(int i=0;i<4;i++){writefile(temp,versions[i]);assert(kfx_commit_save(temp,path));}
 check(path,"five");snprintf(backup,sizeof(backup),"%s.backup1",path);check(backup,"four");snprintf(backup,sizeof(backup),"%s.backup2",path);check(backup,"three");snprintf(backup,sizeof(backup),"%s.backup3",path);check(backup,"two");
 assert(!kfx_commit_save(temp,path));check(path,"five");writefile(temp,"");assert(!kfx_commit_save(temp,path));check(path,"five");
 snprintf(backup,sizeof(backup),"%s.protected",path);writefile(backup,"favourite");writefile(temp,"blocked");assert(kfx_save_is_protected(path));assert(!kfx_commit_save(temp,path));check(path,"five");unlink(backup);assert(kfx_commit_save(temp,path));check(path,"blocked");
 writefile(temp,"uncommitted");check(path,"blocked");puts("PASS: atomic install, three-version backup rotation, missing/empty temporary saves preserve original, uncommitted write preserves original");}
