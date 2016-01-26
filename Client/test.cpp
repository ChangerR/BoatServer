#include <dirent.h>  
#include <stdio.h>


const char* findFileExt(const char* filename) {
	const char* p = filename;
	const char* end ;
	while(*p)p++;
	--p;
	end = p;
	while( p >= filename && *p != '.')--p;
	if(*p != '.')
		p = end;
	return p+1;
}

void ls(const char* dirname) {
	DIR* p_dir;
	struct dirent* p_dirent;
	if((p_dir = opendir(dirname)) == NULL) {
		printf("Cannot opendir %s\n",dirname);
		return;
	}
	while((p_dirent = readdir(p_dir))) {
		printf("file %s ext=%s\n",p_dirent->d_name,findFileExt(p_dirent->d_name));
	}
	return;
}

inline char* get_file_from_path(const char* path,char* buf) {
	const char* p1;
	char* p2;

	p2 = buf;
	p1 = path;
	while(*p1)p1++;
	
	while(p1 >= path) {
		if(*p1 == '/')
			break;
		p1--;
	}
	
	if(*p1 == '/')
		p1++;
	
	while(*p1) {
		*p2++ = *p1++;
	}
	*p2 = 0;
	return buf;
}

int main(int argc,char** argv) {
	
	if(argc == 2) {
		ls(argv[1]);
	}
	char buf[64];
	printf("%s\n",get_file_from_path("/c/a/d/b/d/f.g",buf));
	return 0;
}
