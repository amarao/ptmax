#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#define SECTOR_SIZE 512
#define PT_SIZE 64
#define MBR_OFFSET 0x1BE

unsigned char sector[512];

struct pte {
	unsigned char status;
	unsigned char first_head;
	unsigned char first_sector;
	unsigned char first_cylinder;
	unsigned char partition;
	unsigned char last_head;
	unsigned char last_sector;
	unsigned char last_cylinder;
	unsigned int  LBA;
	unsigned int  size; /*in sectors*/
};

struct pt{
	struct pte e[4];
};

int pt_fd=-1;

struct pt* read_pt(const char* path){
	int a;
	pt_fd=open(path,O_RDONLY|O_LARGEFILE|O_NONBLOCK);
	if(pt_fd==-1) return NULL;
	a=read(pt_fd,(void*)sector,512);
	if(a!=SECTOR_SIZE){
		goto bad;
	}
	if(sector[510]!=0x55 || sector[511]!=0xAA)
		goto bad;

	return (struct pt*)(sector+MBR_OFFSET);
	bad: /* we failing, free, close and return*/
		close(pt_fd);
		pt_fd=-1;
		return NULL;
}

void print_pt(struct pte* p){
//	int c;
//	for(c=0;c<sizeof(struct pt);c++){
//		printf ("%X ",((unsigned char*)p)[c]);
//	}
	printf("\nstatus=%x\nHSC(first-last)=(%x,%x,%x - %x,%x, %x)\ntype=%x,\nLBA_START=%i, LENGTH=%i\n",p->status,p->first_head,p->first_sector,p->first_cylinder,p->last_head,p->last_sector,p->last_cylinder,p->partition,p->LBA,p->size);
}


char* get_parent(const char* path){
/*we supports only for pirmary partitions*/
	int l=strlen(path);
	char *newpath;
	if (path[l-1]<'1' || path[l-1] >'4') 
		return NULL;
	newpath=malloc(l-1);
	assert(newpath);
	memcpy(newpath,path,l-1);
	return newpath;
	
}

off64_t getdisksize(int fd){
	return 0; /*stub*/
}
int get_num(const char* path){
/*we supports only for pirmary partitions*/
	int l=strlen(path);
	if (path[l-1]<'1' || path[l-1] >'4') 
		return -1;
	return path[l-1]-'1'; /*1-4 allowed values*/
}

int main(int argc, char* argv[]){
	if (argc<2){
		return printf("no device");
	}
	char* dev=get_parent(argv[1]);
	int pt_num=get_num(argv[1]);
	struct pt* pt=read_pt(dev);
	printf ("using device %s,partition %i\n",dev,pt_num);
	if (!pt)
		return printf("unable to find partition table\n");

	print_pt(pt->e+pt_num);

	
}
