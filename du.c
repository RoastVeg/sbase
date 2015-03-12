/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

static size_t maxdepth = SIZE_MAX;
static size_t blksize  = 512;

static int aflag = 0;
static int sflag = 0;
static int hflag = 0;
static int ret   = 0;

static void
printpath(size_t n, const char *path)
{
	if (hflag)
		printf("%s\t%s\n", humansize(n * blksize), path);
	else
		printf("%zu\t%s\n", n, path);
}

static size_t
nblks(blkcnt_t blocks)
{
	return (512 * blocks + blksize - 1) / blksize;
}

void
du(const char *path, int depth, void *total)
{
	struct stat st;
	size_t subtotal = 0;

	if (lstat(path, &st) < 0) {
		if (!depth || errno != ENOENT)
			weprintf("stat %s:", path);
		if (!depth)
			ret = 1;
		return;
	}

	if (S_ISDIR(st.st_mode))
		recurse(path, du, depth, &subtotal);
	*((size_t *)total) += subtotal + nblks(st.st_blocks);

	if (!sflag && depth <= maxdepth && (S_ISDIR(st.st_mode) || aflag))
		printpath(subtotal + nblks(st.st_blocks), path);
}

static void
usage(void)
{
	eprintf("usage: %s [-a | -s] [-d depth] [-h] [-k] [-H | -L | -P] [-x] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int kflag = 0, dflag = 0;
	size_t n = 0;
	char *bsize;

	ARGBEGIN {
	case 'a':
		aflag = 1;
		break;
	case 'd':
		dflag = 1;
		maxdepth = estrtonum(EARGF(usage()), 0, MIN(LLONG_MAX, SIZE_MAX));
		break;
	case 'h':
		hflag = 1;
		break;
	case 'k':
		kflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'x':
		recurse_samedev = 1;
		break;
	case 'H':
	case 'L':
	case 'P':
		recurse_follow = ARGC();
		break;
	default:
		usage();
	} ARGEND;

	if ((aflag && sflag) || (dflag && sflag))
		usage();

	bsize = getenv("BLOCKSIZE");
	if (bsize)
		blksize = estrtonum(bsize, 0, MIN(LLONG_MAX, SIZE_MAX));
	if (kflag)
		blksize = 1024;

	if (!argc) {
		du(".", 0, &n);
		if (sflag && !ret)
			printpath(nblks(n), ".");
	} else {
		for (; *argv; argc--, argv++) {
			du(argv[0], 0, &n);
			if (sflag && !ret)
				printpath(n, argv[0]);
		}
	}

	return ret;
}
