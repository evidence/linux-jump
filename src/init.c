#include <stdio.h>

#ifndef NULL_LIB
#include "global.h"
#include "init.h"
#include "mem.h"
#include "assert.h"

extern void initmem();
extern void initsyn();
extern void initcomm();
extern void disable_sigio_sigalrm();         
extern void enable_sigio();       
extern unsigned long jia_current_time();
extern unsigned int get_usecs();

int jump_getline(int *wordc, char wordv[Maxwords][Wordsize]);
void gethosts();
int  mypid();
void copyfiles(char *argv);
void start_slaves(int argc, char **argv);
void jiacreat(int argc, char **argv);
void barrier0();
void redirstdio();
void jia_init(int argc, char **argv);
void clearstat();

extern long Startport;

FILE *config, *fopen();
int jia_pid; 
host_t hosts[Maxhosts+1];

/**
 * Host counter
 *
 * This is the number of hosts as specified by the ~/.jiahosts file.
 */
int hostc;

char argv0[Wordsize];
sigset_t startup_mask;
int jia_lock_index;

#ifdef DOSTAT
jiastat_t jiastat;

/**
 * Reset statistical information.
 */
void clearstat()
{
	memset((char*)&jiastat, 0, sizeof(jiastat));
}
#else
void clearstat(){}
#endif

int jump_getline(int *wordc, char wordv[Maxwords][Wordsize])
{
	char line[Linesize];
	int ch;
	int linei,wordi1,wordi2;
	int note;

	linei=0;
	note=0;
	ch=getc(config);
	if (ch=='#') note=1;
	while ((ch != '\n') && (ch != EOF)) {
		if ((linei < Linesize - 1) && (note == 0)) {
			line[linei] = ch;
			linei++;
		}
		ch = getc(config);
		if (ch == '#') note = 1;
	}
	line[linei] = '\0';

	for (wordi1 = 0; wordi1 < Maxwords; wordi1++)
		wordv[wordi1][0] = '\0';
	wordi1 = 0; linei = 0;
	while ((line[linei] != '\0') && (wordi1 < Maxwords)) {
		while ((line[linei] == ' ') || (line[linei] == '\t'))  
			linei++;
		wordi2 = 0;
		while ((line[linei] != ' ') && (line[linei] != '\t')
				&&(line[linei] != '\0')) {
			if (wordi2 < Wordsize - 1) {
				wordv[wordi1][wordi2] = line[linei];
				wordi2++;
			}
			linei++;
		}
		if (wordi2 > 0) { 
			wordv[wordi1][wordi2] = '\0';  
			wordi1++;
		}
	}

	*wordc = wordi1;
	return(ch == EOF);
}

void gethosts()
{
	int endoffile;
	int wordc, linec, uniquehost;
	char wordv[Maxwords][Wordsize];
	struct hostent *hostp;
	int i;

	if ((config = fopen(".jiahosts", "r")) == 0) {
		printf("Cannot open .jiahosts file\n");
		exit(1);
	}

	endoffile = 0;
	hostc = 0;
	linec = 0;
	while (!endoffile) {
		endoffile = jump_getline(&wordc, wordv);
		linec++;
		ASSERT((wordc == Wordnum) || (wordc == 0),  "Line %4d: incorrect host specification!", linec);
		if (wordc != 0) {
			hostp=gethostbyname(wordv[0]);
			ASSERT((hostp != NULL), "Line %4d: incorrect host %s!", linec, wordv[0]);
			strcpy(hosts[hostc].name, hostp->h_name); 
			memcpy(hosts[hostc].addr, hostp->h_addr, hostp->h_length);
			hosts[hostc].addrlen = hostp->h_length;
			strcpy(hosts[hostc].user, wordv[1]); 
			strcpy(hosts[hostc].passwd, wordv[2]);

			for (i = 0; i < hostc; i++) {
#ifdef NFS
				uniquehost = (strcmp(hosts[hostc].name, hosts[i].name) != 0);
#else  /* NFS */
				uniquehost = ((strcmp(hosts[hostc].addr, hosts[i].addr) != 0) ||
						(strcmp(hosts[hostc].user, hosts[i].user) != 0));
#endif /*NFS */
				ASSERT(uniquehost, "Line %4d: repeated spec of the same host!", linec);
			}
			hostc++; 
		} 
	}
	ASSERT((hostc <= Maxhosts), "Too many hosts!");
	fclose(config);
}


void copyfiles(char *argv)
{
	int hosti, rcpyes;
	char cmd[Linesize];

	printf("******Start to copy system files to slaves!******\n");

	for (hosti = 1; hosti < hostc; hosti++) {
		printf("Copy files to %s@%s.\n", hosts[hosti].user, hosts[hosti].name);

		cmd[0] = '\0';
		strcat(cmd, "rcp .jiahosts ");
		strcat(cmd, hosts[hosti].user);
		strcat(cmd, "@");
		strcat(cmd, hosts[hosti].name);
		strcat(cmd, ":");
		rcpyes = system(cmd);
		ASSERT((rcpyes == 0), "Cannot rcp .jiahosts to %s!\n", hosts[hosti].name);

		cmd[0] = '\0';
		strcat(cmd, "rcp ");
		strcat(cmd, argv);
		strcat(cmd, " ");
		strcat(cmd, hosts[hosti].user);
		strcat(cmd, "@");
		strcat(cmd, hosts[hosti].name);
		strcat(cmd, ":");
		rcpyes = system(cmd);
		ASSERT((rcpyes == 0), "Cannot rcp %s to %s!\n", argv, hosts[hosti].name);
	} 
	printf("Remote copy succeed!\n\n");
}

void start_slaves(int argc, char **argv)
{
	int hosti;
	char cmd[Linesize], *hostname;
	int i;

#ifdef NFS
	char *pwd;
	pwd = getenv("PWD"); 
	ASSERT((pwd != NULL), "Failed to get current working directory");
#endif /* NFS */

	printf("******Start to create processes on slaves!******\n\n");
	Startport = getpid();
	ASSERT( (Startport != -1), "getpid() error");
	Startport = 10000 + (Startport * Maxhosts * Maxhosts * 4) % 10000;

	for (hosti = 1; hosti < hostc; hosti++) {
#ifdef NFS
		sprintf(cmd, "cd %s; %s", pwd, pwd);
#else  
		cmd[0]='\0';
		strcat(cmd, "rsh -l ");
		strcat(cmd, hosts[hosti].user);
#endif 
		hostname = hosts[hosti].name;
		strcat(cmd, " ");
		strcat(cmd, hostname); 
		strcat(cmd, " ");
		for (i = 0;i < argc; i++) {
			strcat(cmd, argv[i]);
			strcat(cmd, " ");
		}

		strcat(cmd, "-P");
		sprintf(cmd, "%s%ld ", cmd, Startport);
		strcat(cmd," &");
		printf("Starting CMD %s on host %s\n", cmd, hosts[hosti].name);
		system(cmd);
		ASSERT((hosts[hosti].riofd != -1),  "Fail to start process on %s!", hosts[hosti].name);
	}
}

int mypid()
{
	char hostname[Wordsize];
	uid_t uid;
	struct passwd *userp;
	struct hostent *hostp;
	int i;

	ASSERT((gethostname(hostname, Wordsize) == 0), "Cannot get hostname!");
	hostp = gethostbyname(hostname);
	ASSERT((hostp != NULL), "Cannot get host address!");
	printf("hostname = %s\n", hostname);
	printf("hostp = %s\n", hostp->h_name);

	uid = getuid();
	userp = getpwuid(uid);
	ASSERT((userp != NULL), "Cannot get user name!");

	i = 0;
	strtok(hostname, ".");
	while ((i<hostc) &&
#ifdef NFS
			(!(strncmp(hosts[i].name, hostname, strlen(hostname)) == 0)))
#else  /* NFS */
		(!((strncmp(hosts[i].name, hostname, strlen(hostname)) == 0) &&
		   (strcmp(hosts[i].user, userp->pw_name) == 0)))) 
#endif /* NFS */
		   i++;

	ASSERT((i < hostc), "Get Process id incorrect");
	return(i);
}

void jiacreat(int argc, char **argv)
{
	gethosts();
	if (hostc == 0) {
		printf("  No hosts specified!\n");
		exit(0);
	}
	jia_pid = mypid();

	if (jia_pid == 0) { 
		sleep(1);
		start_slaves(argc, argv);
	} else {
		int c;
		optind = 1;
		while ((c = getopt(argc, argv, "P:")) != -1) {
			switch (c) {
				case 'P': {
						  Startport = atol(optarg);
						  break;
					  }
			}
		}
		optind = 1;
	}
}

void barrier0()
{
	int hosti;
	char buf[4];

	if (jia_pid == 0) {
		for (hosti = 1; hosti < hostc; hosti++) {
			printf("Poll host %d: stream %4d----", hosti, hosts[hosti].riofd);
			read(hosts[hosti].riofd, buf, 3);
			buf[3] = '\0';
			printf("%s Host %4d arrives!\n", buf, hosti);
#ifdef NFS
			write(hosts[hosti].riofd, "ok!", 3);
#endif
		}
#ifndef NFS
		for (hosti = 1; hosti < hostc; hosti++)
			write(hosts[hosti].riofd, "ok!", 3);
#endif
	} 
	else {
		write(1, "ok?", 3);
		read(0, buf, 3);
	}
}

void redirstdio()
{
	char outfile[Wordsize];

	if (jia_pid != 0) {
		sprintf(outfile,"debug-%d.log", jia_pid);
		freopen(outfile, "w", stdout);
		setbuf(stdout, NULL);
		sprintf(outfile, "debug-%d.err", jia_pid);
		freopen(outfile, "w", stderr);
		setbuf(stderr, NULL);
	}
}

/**
 * This is the first function invoked by the application to set-up the DSM.
 * argc and argv are the same arguments of the application's main().
 */
void jia_init(int argc, char **argv)
{
	struct rlimit rl;

	strcpy(argv0, argv[0]);
	disable_sigio_sigalrm();
	jia_lock_index = 0;  
	jiacreat(argc, argv);
	sleep(2);
	rl.rlim_cur = Maxfileno;
	rl.rlim_max = Maxfileno;
	setrlimit(RLIMIT_NOFILE, &rl);

	rl.rlim_cur = Maxmemsize;
	rl.rlim_max = Maxmemsize;
	setrlimit(RLIMIT_DATA, &rl);

	initmem();
	initsyn();
	initcomm();
	clearstat();
	sleep(2);
	redirstdio();
	enable_sigio();

	jia_current_time();
	if (jia_pid != 0) sleep(1);
}

#else  /* NULL_LIB */

int jia_pid = 0; 
int hostc = 1;

void jia_init(int argc, char **argv)
{
	printf("This is JUMP-NULL\n");
}
#endif /* NULL_LIB */

unsigned int t_start, t_stop = 0;

unsigned int jia_startstat()
{
	clearstat();
	t_start = get_usecs();
	return t_start;
}

unsigned int jia_stopstat()
{
	t_stop = get_usecs();
	return t_stop;
}
