all : file_dedup DGFS DGFS_improved

DGFS : dgfs.c 
	gcc -o DGFS dgfs.c -lcrypto

file_dedup : file_dedup.c
	gcc -o file_dedup file_dedup.c -lcrypto

DGFS_improved: dgfs_improved.c
	gcc -o DGFS_improved dgfs_improved.c -lcrypto

clean:
	rm file_dedup DGFS
