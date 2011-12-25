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

int force=0;

unsigned int dev_size=0; /*in sectors*/

struct pt* read_pt(const char* path)
{
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

void print_pt(struct pte* p)
{
//	int c;
//	for(c=0;c<sizeof(struct pt);c++){
//		printf ("%X ",((unsigned char*)p)[c]);
//	}
	printf("\nstatus=%x\nHSC(first-last)=(%x,%x,%x - %x,%x, %x)\ntype=%x,\nLBA_START=%i, LENGTH=%i\n",p->status,p->first_head,p->first_sector,p->first_cylinder,p->last_head,p->last_sector,p->last_cylinder,p->partition,p->LBA,p->size);
}


char* get_parent(const char* path)
{
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

void getdisksize(int fd)
{
	ioctl(fd,BLKGETSIZE,&dev_size);
}

int is_valid(struct pte* e)
{
	if((e->status==0x80||e->status==0) && e->partition!=0)
		return 1;
	return 0;
}

int is_fine(int num,struct pt* p)
{
	int c;
	for (c=0;c<num;c++){
		if(c==num)
			continue;
		if(! is_valid(p->e+c))
			continue;
		if(p->e[c].LBA < p->e[num].LBA && p->e[c].LBA + p->e[c].size > p->e[num].LBA) /*validating p[num] is starting in the middle of the other PT*/
			return 0;
		if(p->e[c].LBA < p->e[num].LBA + p->e[num].size && p->e[c].LBA > p->e[num].LBA) /*other PT is starting in the middle of p[num]*/
			return 0;	
	}
	return 1;
}

unsigned int get_max(int num, struct pt* p)
{
	/*1) validate PT 2) see if there is hole to next partition 3) see if device have free space at the end*/
	int c;
	unsigned int new_size=0;
	assert(dev_size);
	if(p->e[num].size+p->e[num].LBA<32*1024*1024){ 
		/*I don't want to mess with CHS fields up, so partitions with last sector in 32Mb is NOT SUPPORTED*/
		printf("Partition is too small. This utility does not support altering of CHS fields, aborting\n");
		exit(-1);
	}
	if (!is_fine(num,p)){
		printf("overlapping partitions, aborting\n");
		exit(-1);
	}

	if(num<4)
		for(c=num+1;c<4;c++){
			if(!is_valid(p->e+c))
				continue;
			printf("newsize - initial LBA: %u(%i)\n",p->e[c].LBA,c);
			new_size=p->e[c].LBA - p->e[num].LBA;
			break;
		}
	if (new_size==0)
		new_size=dev_size - p->e[num].LBA;

	if(new_size < p->e[num].size){
		printf("something wrong, aborting\n");
		exit(-1);
	}
	printf("old_size=%u sectors, new_size=%u sectors\n",p->e[num].size,new_size);
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
	if (argc<2){
		return printf("no device");
	}
	char* dev=get_parent(argv[1]);
	unsigned int pt_num=get_num(argv[1]);
	unsigned int new_size;
	struct pt* pt=read_pt(dev);
	getdisksize(pt_fd);
	printf("disk size %i sectors\n",dev_size);
	printf ("using device %s,partition %i\n",dev,pt_num);
	if (!pt)
		return printf("unable to find partition table\n");

	print_pt(pt->e+pt_num);
	new_size=get_max(pt_num,pt);
	if(new_size==pt->e[pt_num].size){
		printf("Nowhere to grow, aboring\n");
		exit(0);
	}
	
}
