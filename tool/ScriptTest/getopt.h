#ifndef _SLGETOPT_H_
#define _SLGETOPT_H_

#ifdef __cplusplus
extern "C" {
#endif
	extern char* optarg;
	extern int optind;
	extern int opterr;
	extern int optopt;
	int getopt(int argc, char** argv, char* optstr);
#ifdef __cplusplus
}
#endif

#endif
