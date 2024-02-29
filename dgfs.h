#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>


int DGFS_create(char* file);
int DGFS_add(char* filename);
int DGFS_ls();
int DGFS_remove(char* filename);
int DGFS_extract(char* filename, char* output_filename);
