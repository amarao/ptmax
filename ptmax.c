/* ptmax.c -- Partition resize utility.
 *
 * Copyright (C) 2011 George Shuklin (george.shuklin@gmail.com)
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 333 or
 * (at your option) any later version.
 *
 * read4_little_endian() and store4_little_endian() is taken from 
 * fdisk  utility ((C) 1992  A. V. Le Blanc (LeBlanc@mcc.ac.uk)
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>

#define PT_OFFSET 0x1BE
#define PTE_SIZE 16

struct pte {
	unsigned char status;
	unsigned char partition;
	unsigned int  LBA;
	unsigned int  size; /*in sectors*/
};

struct pt{
	struct pte e[4];
};



/*global variables goes here*/
unsigned char *MBR;
struct pte p[4];
int pt_fd=-1;
unsigned int dev_size=0; /*in sectors*/
unsigned int sect_size=0;/*in bytes*/


int is_pt()
{
	if(MBR[510]==0x55 && MBR[511]==0xAA) /*shall I use an 'last bytes, depends on sector size? Don't know*/
		return 1;
	return 0;
}

int read_device(const char* path)
{
	pt_fd=open(path,O_RDWR|O_LARGEFILE|O_NONBLOCK); 
	/*non-block: we can do this, because we don't remove, move or shrinking partitions*/
	if(pt_fd==-1) {
		printf("failed to open device %s: %s\n",path,strerror(errno));
		return 0;
	}
	if( ioctl(pt_fd,BLKGETSIZE,&dev_size)==-1 || ioctl(pt_fd,BLKSSZGET,&sect_size) == -1){
		printf("unable to get device size or sector size: %s, aborting\n",strerror(errno));
		return 0;
	}
	assert(dev_size||sect_size);
//	printf("dev_size=%u, sect_size=%u\n",dev_size,sect_size);
	MBR=malloc(sect_size);
	assert(MBR);
	if(read(pt_fd,MBR,sect_size)!=sect_size){
		printf("unable to read 1st sector: %s, aborting\n",strerror(errno));
		return 0;
	}
	return 1;
}

int write_device(){
	if(pt_fd==-1)
		return 0;
	if(lseek(pt_fd,0,SEEK_SET)==-1){
		printf("error seeking: %s, write aborted\n",strerror(errno));
		exit(-1);
	}
	if(write(pt_fd,MBR,sect_size)!=sect_size){
		printf("error writing 1st sector: %s\n",strerror(errno));
		exit(-1);
	}
	printf("write successfull (%d bytes)\nReread partition table or reboot to see result\n",sect_size);
	close(pt_fd);
}


/*snached from fdisk code*/
static unsigned int
read4_little_endian(const unsigned char *cp) {
        return (unsigned int)(cp[0]) + ((unsigned int)(cp[1]) << 8)
                + ((unsigned int)(cp[2]) << 16)
                + ((unsigned int)(cp[3]) << 24);
}



void read_pte(int num)
{
        p[num].status=MBR[PT_OFFSET+PTE_SIZE*num+0];
        p[num].partition=MBR[PT_OFFSET+PTE_SIZE*num+4];
	p[num].LBA=read4_little_endian(MBR+PT_OFFSET+PTE_SIZE*num+8);
        p[num].size=read4_little_endian(MBR+PT_OFFSET+PTE_SIZE*num+0xC); /*in sectors*/
}

static void store4_little_endian(unsigned char *cp, unsigned int val) {
        cp[0] = (val & 0xff);
        cp[1] = ((val >> 8) & 0xff);
        cp[2] = ((val >> 16) & 0xff);
        cp[3] = ((val >> 24) & 0xff);
}



void write_pte(int num)
{
/*hack - we need to change only size field*/
	printf("was %d\n",read4_little_endian(MBR+PT_OFFSET+PTE_SIZE*num+0xC));
	store4_little_endian(MBR+PT_OFFSET+PTE_SIZE*num+0xC,p[num].size);
	printf("now %d\n",read4_little_endian(MBR+PT_OFFSET+PTE_SIZE*num+0xC));
}

int read_pt(){
	int c;
	for (c=0;c<4;c++){
		read_pte(c);
	}
}

void print_pt()
{
//	int c;
//	for(c=0;c<sizeof(struct pt);c++){
//		printf ("%X ",((unsigned char*)p)[c]);
//	}
	int c;
	for(c=0;c<4;c++)
		printf("\nstatus=%x\ntype=%x,\nLBA_START=%i, LENGTH=%i\n",p[c].status,p[c].partition,p[c].LBA,p[c].size);
}


char* get_parent(const char* path)
{
    /* try to find parent device name via sysfs */
    if (strstr(path, "/dev/")==path) {
        int len=strlen(path)-5;
        char name[PATH_MAX] = "/sys/class/block/";
        char *t;
        const char *p;
        for(t = name+strlen(name), p = path+5; *p; p++,t++) {
            if(*p == '/') *t = '!';
            else *t = *p;
        }
        if(readlink(name, name, sizeof(name))>0) {
            char *suf = name+strlen(name);
            while(suf>=name && *suf != '/') suf--;
            if(suf > name) {
                char *end = suf--;
                while(suf>=name && *suf != '/') suf--;
                if(suf > name) {
                    char * newpath = malloc(5/*"/dev/"*/+(end-suf)+1);
                    strcpy(newpath, "/dev/");
                    for(t=newpath+5/*"/dev/"*/, suf++; *suf && *suf!='/'; suf++,t++) {
                        if(*suf == '!') *t = '/';
                        else *t = *suf;
                    }
                    *t = 0;
                    return newpath;
                }
            }
        }
    }
    /* else try to find parent device using guess */
    {
        /*we supports only for pirmary partitions, feel free to add more*/
        int l=strlen(path)-1;
        char *newpath;
        if (path[l]<'1' || path[l] >'4') 
            return NULL;
        // For /dev/cciss/c0d0p0 we should cut "p0", not only "0"
            if (strstr(path, "/cciss/")!=NULL && path[l-1]=='p')
            l--;
        newpath=malloc(l);
        assert(newpath);
        memcpy(newpath,path,l);
        return newpath;
    }
}

int is_valid(int num)
{
	if((p[num].status==0x80||p[num].status==0) && p[num].partition!=0)
		return 1;
	return 0;
}

int is_fine(int num)
{
	int c;
	for (c=0;c<num;c++){
		if(c==num)
			continue;
		if(! is_valid(c))
			continue;
		if(p[c].LBA < p[num].LBA && p[c].LBA + p[c].size > p[num].LBA) /*validating p[num] is starting in the middle of the other patition*/
			return 0;
		if(p[c].LBA < p[num].LBA + p[num].size && p[c].LBA > p[num].LBA) /*other paritition is starting in the middle of p[num]*/
			return 0;	
	}
	return 1;
}

unsigned int get_max(int num)
{
	/*1) validate PT 2) see if there is hole to next partition 3) see if device have free space at the end*/
	int c;
	unsigned int new_size=0;
	if(p[num].size+p[num].LBA<256*256*256*sect_size||!p[num].size){ 
		/*I don't want to mess with CHS fields up, so partitions with last sector in the first 32Mb is NOT SUPPORTED*/
		printf("Partition is too small. This utility does not supports altering of CHS fields, aborting\n");
		exit(-1);
	}
	if (!is_fine(num)){
		printf("overlapping partitions, aborting\n");
		exit(-1);
	}

	if(num<4)
		for(c=num+1;c<4;c++){
			if(!is_valid(c))
				continue;
			printf("newsize - initial LBA: %u(%i)\n",p[c].LBA,c);
			new_size=p[c].LBA - p[num].LBA; /*we maximizing up to the next parition start*/
			break;
		}
	if (new_size==0)
		new_size=dev_size - p[num].LBA;/*nothing interesing found - trying to grow to the disk end*/

	if(new_size < p[num].size){
		printf("something wrong, aborting\n");
		exit(-1);
	}
	printf("old_size=%u sectors, new_size=%u sectors\n",p[num].size,new_size);
	return new_size;
}

int get_num(const char* path)
{
/*we supports only for pirmary partitions*/
	int l=strlen(path);
	if (path[l-1]<'1' || path[l-1] >'4') 
		return -1;
	return path[l-1]-'1'; /*1-4 allowed values*/
}

int main(int argc, char* argv[])
{
	if(argc<2 || !strcmp(argv[1],"--help")){
		printf("Usage: ptmax /dev/xxxN (f.e. /dev/sda2, /dev/xvdb4, etc)\n");
		exit(0);
	}
	if (!strcmp(argv[1],"--version")){
		printf("ptmax, version 1.0\n");
		exit(0);
	}
	char* dev=get_parent(argv[1]);
	unsigned int pt_num=get_num(argv[1]);
	unsigned int new_size;
	if(!dev){
		printf("unable to get parent device (use /dev/xx1 instead /dev/xx)\n");
		exit(-1);
	}
	if (!read_device(dev)){
		exit(-1);
	}
	if(!is_pt()){
		printf("1st sector not an MBR, aborting\n");
		exit(-1);
	}
	read_pt();
//	printf("disk size %i sectors\n",dev_size);
//	printf ("using device %s,partition %i\n",dev,pt_num);
//	print_pt(pt->e+pt_num);
	print_pt();
	new_size=get_max(pt_num);
	if(new_size==p[pt_num].size){
		printf("Nowhere to grow, aboring\n");
		exit(0);
	}
	printf("applying changes to parition #%d\n",pt_num);
	p[pt_num].size=new_size;
	write_pte(pt_num);
	write_device();
}
