#include "apue.h"
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>



/* static functions */
typedef int Myfunc(const char *, const struct stat *, int);

static int dopath(Myfunc *);
static int myftw(char *, Myfunc *);
static Myfunc myfunc, myfunc1, myfunc2;
static void getRealpath(const char *pathname, char *realpath, size_t len);

static long n4096, nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;


/* for function: ftw */
#define FTW_F 1
#define FTW_D 2
#define FTW_DNR 3
#define FTW_NS 4


/* for function: myfunc */
static char *fullpath;
static size_t pathlen;


/* for function: myfunc1 */
static long filesize;
static char *filebuf, *comparebuf;


/* for function: myfunc2 */
static int arglen;
static char **argvs;



int main(int argc, char *argv[])
{
    /* check argcs */
    if (!(argc == 2 || (argc == 4 && !strcmp(argv[2], "-comp")) || (argc >= 4 && !strcmp(argv[2], "-name"))))
    {
        puts("usage:  myfind <pathname> [-comp <filename> | -name <str>...]");
        exit(0);
    }


    /* argc == 2: myfind <pathname> */
    if (argc == 2)
    {
        myftw(argv[1], myfunc);

        ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
        if (!ntot) ntot = 1;

        printf(" ------------------------------------------\n");
        printf("|      TYPES       |  NUMNERS  |  PERCENT  |\n");
        printf(" ------------------------------------------\n");
        printf("|  REGULAR FILES   |  %-7ld  |  %-5.2f %%  |\n", nreg, nreg * 100.0 / ntot);
        printf(" ------------------------------------------\n");

        if (!nreg) nreg = 1;

        printf("|  <=  4KB FILES   |  %-7ld  |  %5.2f %%  |\n", n4096, n4096 * 100.0 / nreg);
        printf(" ------------------------------------------\n");
        printf("|   DIRECTORIES    |  %-7ld  |  %5.2f %%  |\n", ndir, ndir * 100.0 / ntot);
        printf(" ------------------------------------------\n");
        printf("|  BLOCK SPECIAL   |  %-7ld  |  %5.2f %%  |\n", nblk, nblk * 100.0 / ntot);
        printf(" ------------------------------------------\n");
        printf("|  CHAR  SPECIAL   |  %-7ld  |  %5.2f %%  |\n", nchr, nchr * 100.0 / ntot);
        printf(" ------------------------------------------\n");
        printf("|      FIFO        |  %-7ld  |  %5.2f %%  |\n", nfifo, nfifo * 100.0 / ntot);
        printf(" ------------------------------------------\n");
        printf("|  SYMBOLIC LINK   |  %-7ld  |  %5.2f %%  |\n", nsock, nsock * 100.0 / ntot);
        printf(" ------------------------------------------\n");
    }


    /* argc == 4 && argv[2] == "-comp": myfind  <pathname>  -comp  <filename> */
    if (argc == 4 && strcmp(argv[2], "-comp") == 0)
    {
        int fd;
        struct stat statbuf;

        if (lstat(argv[3], &statbuf) < 0) err_quit("lstat error: %s\n", argv[3]);
        if (!S_ISREG(statbuf.st_mode)) err_quit("that is not a regular file: %s\n", argv[3]);

        /* read <filename> content */
        filesize = statbuf.st_size;

        if ((fd = open(argv[3], O_RDONLY)) == -1)
            err_sys("can't open the file '%s'\n", argv[3]);

        /* malloc for <filename> */
        if ((filebuf = (char *)malloc(sizeof(char) * filesize)) == NULL)
            err_sys("malloc error\n");

        /* malloc for pathname */
        if ((comparebuf = (char *)malloc(sizeof(char) * filesize)) == NULL)
            err_sys("malloc error\n");

        if (read(fd, filebuf, filesize) != filesize)
            err_sys("read error '%s'\n", argv[3]);

        close(fd);

        myftw(argv[1], myfunc1);
    }


    /* argc >= 4 && argv[2] == "-name": myfind  <pathname>  -name str...      */
    if (argc >= 4 && strcmp(argv[2], "-name") == 0)
    {
        arglen = argc - 3;
        argvs = &argv[3];

        myftw(argv[1], myfunc2);
    }


    return 0;
}



/* get filename according to pathname */
static void getfilename(const char* pathname, char* filename)
{
    for (int i = strlen(pathname) - 1; i >= 0; i--)
    {
        if (pathname[i] == '/')
        {
            strcpy(filename, pathname + i + 1);
            return;
        }
    }

    strcpy(filename, pathname);
            
    return;
}



/* get realpath according to pathname */
static void getrealpath(const char* pathname, char* realpath, size_t len)
{
    const char* path;

    /* realpath, don't need modify */
	if (pathname[0] == '/')
	{
		strcpy(realpath, pathname);
		return;
	}

    /* need modify, joint cwd and filename */
	getcwd(realpath, len);

	len = strlen(realpath);
	if (realpath[len - 1] != '/')
		realpath[len++] = '/';

	if (pathname[0] == '.')
	{
		path = pathname + 2;
		strcat(&realpath[len], path);
        return;
	}
	else
    {
		strcat(&realpath[len], pathname);
        return;
    }
}



static int myftw(char *pathname, Myfunc *func)
{
    fullpath = path_alloc(&pathlen);

    if (pathlen <= strlen(pathname))
    {
        pathlen = strlen(pathname) * 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL)
        {
            puts("realloc failed");
            exit(0);
        }
    }

    strcpy(fullpath, pathname);

    return (dopath(func));
}



static int dopath(Myfunc *func)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	int ret, n;

	if (lstat(fullpath, &statbuf) < 0) /* stat error */
		return (func(fullpath, &statbuf, FTW_NS));
	if (S_ISDIR(statbuf.st_mode) == 0) /* not a directory */
		return (func(fullpath, &statbuf, FTW_F));
	/*
	 * It's a directory.  First call func() for the directory,
	 * then process each filename in the directory.
	 */
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return (ret);

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen)
	{ /* expand path buffer */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	if (fullpath[n - 1] != '/')
		fullpath[n++] = '/';
	fullpath[n] = 0;

	if ((dp = opendir(fullpath)) == NULL) /* can't read directory */
		return (func(fullpath, &statbuf, FTW_DNR));

	while ((dirp = readdir(dp)) != NULL)
	{
		if (strcmp(dirp->d_name, ".") == 0 ||
			strcmp(dirp->d_name, "..") == 0)
			continue;						/* ignore dot and dot-dot */
		strcpy(&fullpath[n], dirp->d_name); /* append name after "/" */
		if ((ret = dopath(func)) != 0)		/* recursive */
			break;							/* time to leave */
	}
	fullpath[n - 1] = 0; /* erase everything from slash onward */

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return (ret);
}



static int myfunc(const char *pathname, const struct stat *statptr, int type)
{
    switch (type)
    {
    /* file other than directory */
    case FTW_F:
        switch (statptr->st_mode & S_IFMT)
        {
        case S_IFREG:
            nreg++;
            if (statptr->st_size <= 4096)
                n4096++;
            break;
        case S_IFBLK:
            nblk++;
            break;
        case S_IFCHR:
            nchr++;
            break;
        case S_IFIFO:
            nfifo++;
            break;
        case S_IFLNK:
            nslink++;
            break;
        case S_IFSOCK:
            nsock++;
            break;
        }
        break;

    /* directory */
    case FTW_D:
        ndir++;
        break;

    /* directory that can't be read */
    case FTW_DNR:
        printf("can't read directory %s", pathname);
        break;

    /* file that we can't stat */
    case FTW_NS:
        printf("stat error for %s", pathname);
        break;

    default:
        printf("unknown type %d for pathname %s", type, pathname);
        break;
    }

    return 0;
}



static int myfunc1(const char *pathname, const struct stat *statptr, int type)
{
    /* check the type and size */
    if (type == FTW_F && (statptr->st_mode & S_IFMT) == S_IFREG && statptr->st_size == filesize)
    {
        int fd;
        char realpath[1024] = {0};

        if ((fd = open(pathname, O_RDONLY)) == -1)
        {
            puts("open error!");
            exit(0);
        }

        if (read(fd, comparebuf, filesize) != filesize)
        {
            puts("read error!");
            exit(0);
        }

        close(fd);

        /* check content */
        if (strcmp(filebuf, comparebuf) == 0)
        {
            getrealpath(pathname, realpath, sizeof(realpath));
            printf("%s\n", realpath);
        } 
    }

    return 0;
}



static int myfunc2(const char *pathname, const struct stat *statptr, int type)
{
    char filename[64] = {0};
    char realpath[1024] = {0};

    /* check file type */
    if (type == FTW_F)
    {
        getfilename(pathname, filename);

        for (int i = 0; i < arglen; i++)
        {
            if (strcmp(argvs[i], filename) == 0)
            {
                getrealpath(pathname, realpath, sizeof(realpath));
                printf("%s\n", realpath);
            }
        }
    }

    return 0;
}
