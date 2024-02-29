#include "dgfs_improved.h"
#define supbid 0
#define intbid 1024
#define datbid 2048
#define inbmid 133120
#define hstbid 4327424
#define datbmid 25298944
// define data address
/**
 * Create DGFS file with superblock, data/inode bitmap, and inode region. 
 * @param	file	Name of DGFS file to create
 * @return  Return 0 in case of success and return 1 in case of failure
 */

int store(FILE *dfp, int *addr, int *dedup, void*data,int sizeofdata){// store data, if dedup -> dedup+1(same hash, same data), 
	unsigned char md5hash[17] = {0,},md5nodehash[17] = {0,};
	char data_table[131072];
	char tempdata1[4097] = {0,};
	char *tempdata2;
	char tempchar;
	fseek(dfp,datbid,SEEK_SET);
	fread(data_table,sizeof(char),131072,dfp);//load data bitmap
	MD5_CTX con;
	MD5_Init(&con);
	MD5_Update(&con, data, 4096);
	MD5_Final(md5hash, &con);
	md5hash[16] = '\0';
	for(int j = 0; j < 1024*128; j++){
		for(int k = 7; k >= 0; k--){ //analyze databmap by bit
			if((data_table[j]&(1<<k))!=0){
				int dnum = 7-k + j*8;
				int linked;
				fseek(dfp,hstbid+dnum*20,SEEK_SET);
				fread(&linked,sizeof(int),1,dfp);
				fseek(dfp,hstbid+dnum*20+4,SEEK_SET); // load number of link and hash
 				fread(md5nodehash,sizeof(char),16,dfp);
				if(strcmp(md5hash,md5nodehash) == 0){// if same hash
					fseek(dfp,datbmid + dnum*4096,SEEK_SET);
					tempdata2 = data;
					tempdata2[4096] = '\0';
					fread(tempdata1,sizeof(char),4096,dfp);
		
					if(strcmp(tempdata2,tempdata1) == 0){  // compare data, if same
						linked += 1;
						fseek(dfp,hstbid+dnum*20,SEEK_SET);
						fwrite(&linked,sizeof(int),1,dfp);
						*dedup += 1;
						*addr = datbmid + dnum*4096;  // hashnode link += 1, store address of data
						return 0;
					}
				}
			}
		}
	}
	
	for(int j = 0; j < 1024*128; j++){  // not same hash
		for(int k = 7; k >= 0; k--){
			if((data_table[j]&(1<<k))==0){
				
				int dnum = 7-k + j*8;
				int linked = 1;
				fseek(dfp,hstbid+dnum*20,SEEK_SET);
				fwrite(&linked,sizeof(int),1,dfp);
				fwrite(md5hash,sizeof(char),16,dfp);
				int temp = datbid+j;
				fseek(dfp,temp,SEEK_SET);
				fread(&tempchar,sizeof(char),1,dfp);
				tempchar += 1<<k;
				
				fseek(dfp,temp,SEEK_SET);
				fwrite(&tempchar,sizeof(char),1,dfp);
				*addr = datbmid + dnum*4096;
				fseek(dfp,*addr,SEEK_SET);
				fwrite(data,sizeof(char),sizeofdata,dfp);  // store data in data bm == 0, bm = 1, store(link = 1 ,hash)
				return 0;
				
			}
		}
	}
} 
char DGFSname[20]; // name of dgfs file

int DGFS_create(char* file)
{
	if(access(file, F_OK) != -1){
		printf("%s already exist\n",file);
		return(1);
	}
	FILE *fp;
	fp = fopen(file, "w");
	if(fp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	fseek(fp, supbid, SEEK_SET);
	fwrite(file,sizeof(char),strlen(file),fp);  
	char tempc = '\0';
	fwrite(&tempc,sizeof(char),1,fp); // store name of dgfs file
	fseek(fp, datbmid-1, SEEK_SET);
	fwrite(&tempc,sizeof(char),1,fp);
	fclose(fp);
	printf("DFGS create: %s\n",file); //reset inodebmap, databmap, inode table, hash table 0;
	return(0);
	
}

/**
 * Add file in DGFS. If there is block with same contents in DGFS,
 * DGFS will deduplicate this block and print the number of deduplicated blocks.
 * @param	filename	Name of file to add in DGFS
 * @return  Return 0 in case of success and return 1 in case of failure
 */
int DGFS_add(char* filename)
{
	FILE *fp;
	FILE *dfp;
	int fnum, dnum, linked, targetadr, temp, dedup = 0, addr;
	
	long long fsize;
	char inode_table[1024], tempchar;
	char data_table[131072];

	int point[12] = {0,};
	int indpoint[1024]= {0,};
	int dobpoint[1024][1024]= {0,};
	int dobpointaddr[1024] = {0,};


	char fname[128] = {0,};
	char fnamec[128] = {0,};
	fp = fopen(filename, "r");
	dfp = fopen(DGFSname, "r+"); // open file r+ -> fix data simply
	if(fp == NULL || dfp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	strcpy(fname,filename);
	fseek(dfp, intbid, SEEK_SET);
	fread(inode_table,sizeof(char),1024,dfp);
	for(int i = 0; i < 1024; i++){
		for(int j = 7; j >= 0; j--){
			if((inode_table[i]&(1<<j))!=0){
				fseek(dfp, inbmid+(7-j + i*8)*512, SEEK_SET);
				fread(fnamec,sizeof(char),128,dfp);
				if(strcmp(fnamec, fname)==0){ //compare file name, if same exit
					printf("Duplicate file name!\n");
					return(1);
				}
			}
		}
	}
	
	for(int i = 0; i < 1024; i++){
		for(int j = 7; j >= 0; j--){
			if((inode_table[i]&(1<<j))==0){ // goto empty inode
				fnum = 7-j + i*8;
				goto brk1;
			}
		}
	}

	printf("no capacity\n");
	return(1);
	brk1:
	//inode table change
	temp = intbid+fnum/8;
	fseek(dfp, temp, SEEK_SET);
	fread(&tempchar,sizeof(char),1,dfp);
	tempchar += 1<<(7-(fnum%8));
	fseek(dfp, temp, SEEK_SET);
	fwrite(&tempchar,sizeof(char),1,dfp);
	//inode change
	strcpy(fname,filename);
	fseek(dfp,inbmid+fnum*512,SEEK_SET);
	fwrite(fname,sizeof(char),128,dfp);
	fseek(fp,0,SEEK_END);
	fsize = ftell(fp);
	fwrite(&fsize,sizeof(long long),1,dfp); // store file name(128b), size(8)
	fseek(fp,0,SEEK_SET);
	for(int i = 0; i <= (fsize-1)/(1024*4); i++){
		
		char data[4097] = {0,};
		fseek(fp,i*4096,SEEK_SET);
		fread(data,sizeof(char),4096,fp);
		if(i == (fsize-1)/(1024*4)){
			store(dfp, &addr, &dedup, data,fsize - 1024*4*i); // store data
		}
		else{store(dfp, &addr, &dedup, data,4096);}
		if(i<10){
			point[i] = addr;
		}else if(i < 10 + 1024){
			indpoint[i-10] = addr;
		}else if(i < 10 + 1024 + 1024*1024){
			dobpoint[(i-10-1024)/1024][(i-10-1024)%1024] = addr;
		}else{
			printf("out of memory\n");
			return(1);
		}// store address
	}
	int deduptemp = 0;
	int ffsize;
	if((fsize-1)/(1024*4)>=10){
		ffsize = 4096;
		for(int asd = 1023; asd >= 0; asd--){
			if(indpoint[asd] < datbmid){ffsize -= 4;}
			else break;
		}
		store(dfp, &(point[10]), &deduptemp, indpoint,ffsize); // store indirect pointer
	}
	if((fsize-1)/(1024*4) >= 1034){
		for(int i = 0; i <= ((fsize-1)/(1024*4) - 1034)/1024; i++){
			ffsize = 4096;
			for(int asd = 1023; asd >= 0; asd--){
				if(dobpoint[i][asd] < datbmid){ffsize -= 4;}
				else break;
			}
			store(dfp, &(dobpointaddr[i]), &deduptemp, dobpoint[i],ffsize); // store (deirected)indirect pointer
		}
		ffsize = 4096;
		for(int asd = 1023; asd >= 0; asd--){
			if(dobpointaddr[asd] < datbmid){ffsize -= 4;}
			else break;
		}
		store(dfp, &(point[11]), &deduptemp, dobpointaddr,ffsize); // store (double)indirect pointer
	}
	fseek(dfp,inbmid+fnum*512+128+8,SEEK_SET);

	fwrite(point,sizeof(int),12,dfp); // store pointers > inode
	fclose(fp);
	fclose(dfp);	
	printf("DGFS add: %s\n",filename);
	if(dedup > 0){
		printf("%d block(s) deduplicated\n",dedup);
	}
	return 0;
}

/**
 * Print list of files in DGFS. After printing list of files, 
 * print total size of files and number of data blocks in use.
 * @return  Return 0 in case of success and return 1 in case of failure
 */
int DGFS_ls()
{
	FILE *fp;
	char inode_table[1024];
	char fname[128] = {0,};
	long long size;
	long long totalsize = 0;
	int blocks = 0;
	fp = fopen(DGFSname, "r");
	if(fp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	printf("%-32s%s\n","Filename","Size");
	fseek(fp, intbid, SEEK_SET);
	fread(inode_table,sizeof(char),1024,fp);
	for(int i = 0; i < 1024; i++){
		for(int j = 7; j >= 0; j--){
			if((inode_table[i]&(1<<j))!=0){ // for all inode
				fseek(fp, inbmid+512*(7-j+8*i), SEEK_SET);
				
				fread(fname,sizeof(char),128,fp);
				printf("%-32s",fname);
				
				fread(&size,sizeof(long long),1,fp);
				printf("%lld\n",size);
				totalsize += size; // print inode data
			}
		}
	}
	char tempchar; 
	for(int i = 0; i < 1024*128; i++){
		fseek(fp, datbid+i, SEEK_SET);
		fread(&tempchar,sizeof(char),1,fp);
		for(int j = 0; j < 8; j++){
			if((tempchar&(1<<j))!=0){
				blocks += 1;
			}
		} //calculate numbers of allocated blocks
	}
	printf("Total size: %lld\n",totalsize);
	printf("Allocated blocks: %d\n",blocks);
	fclose(fp);
	return 0;
}

/**
 * Remove file with filename in DGFS.
 * @param	filename	Name of file to remove in DGFS
 * @return  Return 0 in case of success and return 1 in case of failure
 */
int DGFS_remove(char* filename)
{
	FILE *fp;
	char inode_table[1024];
	char fname[128] = {0,};
	char fnamec[128] = {0,};
	char tempedchar;
	long long size;
	int blocks = 0, fnum;
	fp = fopen(DGFSname, "r+");
	if(fp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	strcpy(fname,filename);
	int point[12] = {0,};
	int indpoint[1024]= {0,};
	int indpointnum = 0;
	int dobpoint[1024][1024]= {0,};
	int dobpointnum[1024] = {0,};
	int dobpointaddrnum = 0;
	int dobpointaddr[1024] = {0,};
	fseek(fp, intbid, SEEK_SET);
	fread(inode_table,sizeof(char),1024,fp);
	for(int i = 0; i < 1024; i++){
		for(int j = 7; j >= 0; j--){
			if((inode_table[i]&(1<<j))!=0){  // for exist inode
				fseek(fp, inbmid+(7-j + i*8)*512, SEEK_SET);
				fread(fnamec,sizeof(char),128,fp);
				if(strcmp(fnamec, fname)==0){  // if same name
					fnum = 7-j+i*8;
					fseek(fp, intbid+i, SEEK_SET);
					fread(&tempedchar,sizeof(char),1,fp);
					tempedchar -= tempedchar&(1<<j);
					fseek(fp, intbid+i, SEEK_SET);
					fwrite(&tempedchar,sizeof(char),1,fp); // inode bmap -1

					fseek(fp, inbmid+fnum*512+128, SEEK_SET);
					fread(&size,sizeof(long long),1,fp);
					fseek(fp, inbmid+fnum*512+128+8,SEEK_SET);
					fread(point,sizeof(int),12,fp); //load pointers to change link of hashnode
					goto brk3;

				}
			}
		}
	}
	printf("%s not found!\n",filename);
	return(1);
	brk3:
	if((size-1)/(1024*4)>=10){
		fseek(fp, point[10], SEEK_SET);
		fread(indpoint,sizeof(int),1024,fp);
	}
	if((size-1)/(1024*4) >= 1034){
		fseek(fp, point[11], SEEK_SET);
		fread(dobpointaddr,sizeof(int),1024,fp);
		for(int i = 0; i <= ((size-1)/(1024*4) - 1034)/1024; i++){
			fseek(fp, dobpointaddr[i], SEEK_SET);
			fread(dobpoint[i],sizeof(int),1024,fp);
		}
	} // load pointer data
	
	int k = 0, temped;
	int linked;
	int ssize = size;
	int retaddr;
	char data[4096];
	while(1 == 1){
		if(k < 10){
			retaddr = (point[k]-datbmid)/4096;
		}else if(k < 10 + 1024){
			if(indpointnum == 0){
				retaddr = (point[10]-datbmid)/4096;
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fread(&linked,sizeof(int),1,fp);
				linked -= 1;
				if(linked <= 0){
					int temp = datbid+retaddr/8;
					int rest = retaddr%8;
					char tempchar;
					fseek(fp,temp,SEEK_SET);
					fread(&tempchar,sizeof(char),1,fp);
					tempchar -= (tempchar&(1<<(7-rest)));
					fseek(fp,temp,SEEK_SET);
					fwrite(&tempchar,sizeof(char),1,fp);
					linked = 0;
				}
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fwrite(&linked,sizeof(int),1,fp);
				indpointnum = 1;
			}

			retaddr = (indpoint[k-10]-datbmid)/4096;
		}else if(k < 10 + 1024 + 1024*1024){
			if(dobpointaddrnum == 0){
				retaddr = (point[11]-datbmid)/4096;
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fread(&linked,sizeof(int),1,fp);
				linked -= 1;
				if(linked <= 0){
					int temp = datbid+retaddr/8;
					int rest = retaddr%8;
					char tempchar;
					fseek(fp,temp,SEEK_SET);
					fread(&tempchar,sizeof(char),1,fp);
					tempchar -= (tempchar&(1<<(7-rest)));
					fseek(fp,temp,SEEK_SET);
					fwrite(&tempchar,sizeof(char),1,fp);
					linked = 0;
				}
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fwrite(&linked,sizeof(int),1,fp);
				indpointnum = 1;
			}
			if(dobpointnum[(k-10-1024)/1024] == 0){
				retaddr = (dobpointaddr[(k-10-1024)/1024]-datbmid)/4096;
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fread(&linked,sizeof(int),1,fp);
				linked -= 1;
				if(linked <= 0){
					int temp = datbid+retaddr/8;
					int rest = retaddr%8;
					char tempchar;
					fseek(fp,temp,SEEK_SET);
					fread(&tempchar,sizeof(char),1,fp);
					tempchar -= (tempchar&(1<<(7-rest)));
					fseek(fp,temp,SEEK_SET);
					fwrite(&tempchar,sizeof(char),1,fp);
					linked = 0;
				}
				fseek(fp,hstbid+retaddr*20,SEEK_SET);
				fwrite(&linked,sizeof(int),1,fp);
				indpointnum = 1;
			}
			retaddr = (dobpoint[(k-10-1024)/1024][(k-10-1024)%1024]-datbmid)/4096;
		}else{
			printf("out of memory\n");
			return(1);
		}
		fseek(fp,hstbid+retaddr*20,SEEK_SET);
		fread(&linked,sizeof(int),1,fp);
		linked -= 1;
		if(linked <= 0){
			int temp = datbid+retaddr/8;
			int rest = retaddr%8;
			char tempchar;
			fseek(fp,temp,SEEK_SET);
			fread(&tempchar,sizeof(char),1,fp);
			tempchar -= (tempchar&(1<<(7-rest)));
			fseek(fp,temp,SEEK_SET);
			fwrite(&tempchar,sizeof(char),1,fp);
			linked = 0;
		}
		fseek(fp,hstbid+retaddr*20,SEEK_SET);
		fwrite(&linked,sizeof(int),1,fp);
		// link -= 1 for all data node(include indirect, double indirect, linked indirect pointer)
		// if link == 0, -> databmap 1 -> 0

		fseek(fp, retaddr, SEEK_SET);
		if(ssize <= 0){
			break;
		}else if(ssize < 4096){
			temped = ssize;
		}else{
			temped = 4096;
		}
		k+=1;
		ssize -= temped;
		// calculate size
	}
	fclose(fp);
	printf("DGFS remove: %s\n",filename);
	return 0;
}

/**
 * Extract contents of file in DGFS and save them outside of DGFS system
 * @param	filename		Name of file in DGFS to extract
 * @param	output_filename	Name of file to be extracted outside of DGFS
 * @return  Return 0 in case of success and return 1 in case of failure
 */
int DGFS_extract(char* filename, char* output_filename)
{
	FILE *fp;
	FILE *wfp;
	char inode_table[1024];
	char fname[128] = {0,};
	char fnamec[128] = {0,};
	long long size;
	int blocks = 0, fnum;
	fp = fopen(DGFSname, "r");
	if(fp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	strcpy(fname,filename);
	int point[12] = {0,};
	int indpoint[1024]= {0,};
	int dobpoint[1024][1024]= {0,};
	int dobpointaddr[1024] = {0,};
	fseek(fp, intbid, SEEK_SET);
	fread(inode_table,sizeof(char),1024,fp);
	for(int i = 0; i < 1024; i++){
		for(int j = 7; j >= 0; j--){
			if((inode_table[i]&(1<<j))!=0){ // for all inode
				fseek(fp, inbmid+(7-j + i*8)*512, SEEK_SET);
				fread(fnamec,sizeof(char),128,fp);
				if(strcmp(fnamec, fname)==0){
					fnum = 7-j+i*8;
					fseek(fp, inbmid+fnum*512+128, SEEK_SET);
					fread(&size,sizeof(long long),1,fp);
					fseek(fp, inbmid+fnum*512+128+8,SEEK_SET);
					fread(point,sizeof(int),12,fp);
					goto brk2; //if same name, store datas

				}
			}
		}
	}
	printf("%s not exist!\n",filename); // no same name
	brk2:
	if((size-1)/(1024*4)>=10){
		fseek(fp, point[10], SEEK_SET);
		fread(indpoint,sizeof(int),1024,fp);
	}
	if((size-1)/(1024*4) >= 1034){
		fseek(fp, point[11], SEEK_SET);
		fread(dobpointaddr,sizeof(int),1024,fp);
		for(int i = 0; i <= ((size-1)/(1024*4) - 1034)/1024; i++){
			fseek(fp, dobpointaddr[i], SEEK_SET);
			fread(dobpoint[i],sizeof(int),1024,fp);
		}
	} // load pointer data
	
	int k = 0, temped;
	int ssize = size;
	int retaddr;
	char data[4096];
	wfp = fopen(output_filename, "w");
	if(wfp  == NULL){
		perror("fopen error\n");
		return(1);
	}
	while(1 == 1){
		if(k < 10){
			retaddr = point[k];
		}else if(k < 10 + 1024){
			retaddr = indpoint[k-10];
		}else if(k < 10 + 1024 + 1024*1024){
			retaddr = dobpoint[(k-10-1024)/1024][(k-10-1024)%1024];
		}else{
			printf("out of memory\n");
			return(1);
		}
		fseek(fp, retaddr, SEEK_SET);
		if(ssize <= 0){
			break;
		}else if(ssize < 4096){
			temped = ssize;
		}else{
			temped = 4096;
		}
		fread(data,sizeof(char),temped,fp);
		fwrite(data,sizeof(char),temped,wfp);
		k+=1;
		ssize -= temped;	
		//store data sized temped, (temped  = min(ssize, 4096))
	}
	fclose(fp);
	fclose(wfp);
	printf("DGFS extract: %s into %s\n",filename, output_filename);
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc == 1) {
		printf("Usage: ./DGFS create DGFS_file or DGFS_file (add/extract/remove/ls)\n");
		return 0;
	}
	if (strcmp(argv[1], "create") == 0) {
		if (argc != 3) {
			printf("Usage: ./DGFS create filename\n");
			return 0;
		}
		DGFS_create(argv[2]);
		return 0;
	}
	strcpy(DGFSname, argv[1]);// name of dgfs file
	if (strcmp(argv[2], "add") == 0) {
		if (argc != 4) {
			printf("Usage: ./DGFS DGFS_file add filename\n");
			return 0;
		}
		DGFS_add(argv[3]);
		return 0;
	}
	else if (strcmp(argv[2], "ls") == 0) {
		if (argc != 3) {
			printf("Usage: ./DGFS DGFS_file ls\n");
			return 0;
		}
		DGFS_ls(argv[1]);
		return 0;
	}
	else if (strcmp(argv[2], "extract") == 0) {
		if (argc != 5) {
			printf("Usage: ./DGFS DGFS_file extract in_file out_file\n");
			return 0;
		}
		DGFS_extract(argv[3], argv[4]);
		return 0;
	}
	else if (strcmp(argv[2], "remove") == 0) {
		if (argc != 4) {
			printf("Usage: ./DGFS DGFS_file remove filename\n");
			return 0;
		}
		DGFS_remove(argv[3]);
		return 0;
	}
	else {
		printf("Usage: ./DGFS create DGFS_file or DGFS_file (add/extract/remove/ls)\n");
		return 0;
	}

	return 0;
}
