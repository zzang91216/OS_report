#include <stdio.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * Compare contents of file_1 and file_2 and make hard link between two files
 * if the contents of two files are indentical
 * @param file_1    Name of file
 * @param file_2    Name of file
 * @param hash      Indicates methods of file contents comparision
 * @return  Return 0 in case of success and return 1 in case of failure
 */
int file_dedup(char* file_1, char* file_2, int hash)
{
	int fd1, fd2, diff = 0;
	fd1 = open(file_1, O_RDONLY, 0666);
	fd2 = open(file_2, O_RDONLY, 0666);
	if(fd1 == -1 || fd2 == -1){
		perror("fopen error");
		return(0);
	}
	if(hash == 0){
		int fsize;
		char aa;
		char bb;
		fsize = lseek(fd1,0,SEEK_END);
		if(fsize != lseek(fd2,0,SEEK_END)){
			diff = 1;
		}else{
			lseek(fd1,0,SEEK_SET);
			lseek(fd2,0,SEEK_SET);
			
			for(int i = 0; i < fsize; i++){
				read(fd1,&aa,1);
				read(fd2,&bb,1);
				if(aa != bb){
					diff = 1;
					break;
				}
			}
		}
		if(diff == 1){
			printf("Binary files %s and %s differ\n",file_1,file_2);
		}else{
			remove(file_2);
			link(file_1,file_2);
		}
	}
	else if (hash == 1){
		unsigned char md5hash1[17] = {0,},md5hash2[17] = {0,};
		int fsize1, fsize2;
		fsize1 = lseek(fd1,0,SEEK_END);
		fsize2 = lseek(fd2,0,SEEK_END);
		if(fsize1 != fsize2){
			printf("Binary files %s and %s differ\n",file_1,file_2);
			return(0);
		}
		char *ff1;
		char *ff2;
		ff1 = (char*)malloc((fsize1+1)*sizeof(char));
		ff2 = (char*)malloc((fsize2+1)*sizeof(char));
		lseek(fd1,0,SEEK_SET);
		lseek(fd2,0,SEEK_SET);
		for(int i = 0; i < fsize1; i++){
			read(fd1,ff1+i,sizeof(char));
		}
		for(int i = 0; i < fsize2; i++){
			read(fd2,ff2+i,sizeof(char));
		}
		ff1[fsize1] = '\0';
		ff2[fsize2] = '\0';
		MD5_CTX con1;
		MD5_CTX con2;
		MD5_Init(&con1);
		MD5_Update(&con1, ff1, fsize1);
		MD5_Final(md5hash1, &con1);
		MD5_Init(&con2);
		MD5_Update(&con2, ff2, fsize2);
		MD5_Final(md5hash2, &con2);
		free(ff1);
		free(ff2);
		for(int i = 0; i < 16; i++){
			printf("%02x", md5hash1[i]);
		}
		printf("  %s\n",file_1);
		for(int i = 0; i < 16; i++){
			printf("%02x", md5hash2[i]);
		}
		printf("  %s\n",file_2);
		if(strcmp(md5hash1,md5hash2)!=0){
			printf("Binary files %s and %s differ\n",file_1,file_2);
		}else{
			remove(file_2);
			link(file_1,file_2);
		}

	}
	close(fd1);
	close(fd2);
	return 0;
}

int main(int argc, char* argv[])
{
	int result;

	if (argc != 4) {
		printf("Usage: ./file_dedup hash/byte file_1 file_2\n");
		return 0;
	}

	if (strcmp(argv[1], "byte") == 0) {
		file_dedup(argv[2], argv[3], 0);
	}
	else if (strcmp(argv[1], "hash") == 0) {
		file_dedup(argv[2], argv[3], 1);
	}

    	return 0;
}
