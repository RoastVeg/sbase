/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static int    rflag = 0;
static int    fflag = 'P';
static uid_t  uid = -1;
static gid_t  gid = -1;
static int    ret = 0;
static char *chown_f_name = "chown";
static int (*chown_f)(const char *, uid_t, gid_t) = chown;

static void
chownpwgr(const char *path, int fflag)
{
	if (chown_f(path, uid, gid) < 0) {
		weprintf("%s %s:", chown_f_name, path);
		ret = 1;
	}
	if (rflag)
		recurse(path, chownpwgr, fflag);
}

static void
usage(void)
{
	eprintf("usage: %s [-hRr] [owner][:[group]] file...\n", argv0);
}

int
main(int argc, char *argv[])
{
	char *owner, *group, *end;
	struct passwd *pw;
	struct group *gr;

	ARGBEGIN {
	case 'h':
		chown_f_name = "lchown";
		chown_f = lchown;
		break;
	case 'R':
	case 'r':
		rflag = 1;
		break;
	case 'H':
	case 'L':
	case 'P':
		fflag = ARGC();
		break;
	default:
		usage();
	} ARGEND;

	if (argc == 0)
		usage();

	owner = argv[0];
	argv++;
	argc--;
	if ((group = strchr(owner, ':')))
		*group++ = '\0';

	if (owner && *owner) {
		errno = 0;
		pw = getpwnam(owner);
		if (pw) {
			uid = pw->pw_uid;
		} else {
			if (errno != 0)
				eprintf("getpwnam %s:", owner);
			uid = strtoul(owner, &end, 10);
			if (*end != '\0')
				eprintf("getpwnam %s: no such user\n", owner);
		}
	}
	if (group && *group) {
		errno = 0;
		gr = getgrnam(group);
		if (gr) {
			gid = gr->gr_gid;
		} else {
			if (errno != 0)
				eprintf("getgrnam %s:", group);
			gid = strtoul(group, &end, 10);
			if (*end != '\0')
				eprintf("getgrnam %s: no such group\n", group);
		}
	}
	for (; argc > 0; argc--, argv++)
		chownpwgr(argv[0], fflag);

	return ret;
}
