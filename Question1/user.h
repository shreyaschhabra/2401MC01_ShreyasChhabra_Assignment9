// user.h — xv6 user-space system call declarations
// CHANGES FOR ASSIGNMENT 9:
//   Added vmprint()   (Question 1)
//   Added getfaults() (Question 2)

struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
// Q1: print the calling process's page table
int vmprint(void);
// Q2: return the number of page faults handled for this process
int getfaults(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
// In xv6, printf and fprintf both take a file-descriptor as first argument.
// printf(1, "msg") writes to stdout (fd=1), printf(2, "msg") to stderr (fd=2).
void fprintf(int, const char*, ...);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
