#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

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

struct pt* read_pt(const char* path){
	int fd=-1;
	int a;
	fd=open(path,O_RDONLY|O_LARGEFILE|O_NONBLOCK);
	if(fd==-1) return NULL;
	printf("ok1,%d\n",fd);
	a=read(fd,(void*)sector,512);
	if(a!=SECTOR_SIZE){
		goto bad;
	}
	printf("ok2, %i\n",a);
	if(sector[510]!=0x55 || sector[511]!=0xAA)
		goto bad;

	close(fd);
	printf("ok3");
	return (struct pt*)(sector+MBR_OFFSET);
	bad: /* we failing, free, close and return*/
		close(fd);
		return NULL;
}

void print_pt(struct pte* p){
//	int c;
//	for(c=0;c<sizeof(struct pt);c++){
//		printf ("%X ",((unsigned char*)p)[c]);
//	}
	printf("\nstatus=%x\nHSC(first-last)=(%x,%x,%x - %x,%x, %x)\ntype=%x,\nLBA_START=%x, LENGTH=%x\n",p->status,p->first_head,p->first_sector,p->first_cylinder,p->last_head,p->last_sector,p->last_cylinder,p->partition,p->LBA,p->size);
}

int main(int argc, char* argv[]){
	if (argc<2){
		return printf("no device");
	}
	struct pt* pt=read_pt(argv[1]);
	int c;
	if (!pt)
		return printf("unable to find partition table\n");
	for(c=0;c<sizeof(struct pt);c++)
		printf ("%X ",((unsigned char*)pt)[c]);

	for(c=0;c<4;c++){
		print_pt(pt->e+c);
		printf("------------\n");
	}

	
}
