/*
 * Copyright (C) 2010 Timo Teras <timo.teras@iki.fi>
 *
 * This is free software, licensed under the GNU General Public License v2.
 */
#include <sys/file.h>
#include "libbb.h"

int flock_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int flock_main(int argc UNUSED_PARAM, char **argv)
{
	int mode, opt, fd;
	enum {
		OPT_s = (1 << 0),
		OPT_x = (1 << 1),
		OPT_n = (1 << 2),
		OPT_u = (1 << 3),
		OPT_c = (1 << 4),
	};

#if ENABLE_LONG_OPTS
        static const char getopt_longopts[] ALIGN1 =
		"shared\0"      No_argument       "s"
		"exclusive\0"   No_argument       "x"
		"unlock\0"      No_argument       "u"
		"nonblock\0"    No_argument       "n"
		;
	applet_long_options = getopt_longopts;
#endif
	opt_complementary = "-1";

	opt = getopt32(argv, "+sxnu");
	argv += optind;

	if (argv[1]) {
		fd = open(argv[0], O_RDONLY|O_NOCTTY|O_CREAT, 0666);
		if (fd < 0 && errno == EISDIR)
		        fd = open(argv[0], O_RDONLY|O_NOCTTY);
		if (fd < 0)
			bb_perror_msg_and_die("can't open '%s'", argv[0]);
		//TODO? close_on_exec_on(fd);
	} else {
		fd = xatoi_u(argv[0]);
	}
	argv++;

	/* If it is "flock FILE -c PROG", then -c isn't caught by getopt32:
	 * we use "+" in order to support "flock -opt FILE PROG -with-opts",
	 * we need to remove -c by hand.
	 * TODO: in upstream, -c 'PROG ARGS' means "run sh -c 'PROG ARGS'"
	 */
	if (argv[0]
	 && argv[0][0] == '-'
	 && (  (argv[0][1] == 'c' && !argv[0][2])
	    || (ENABLE_LONG_OPTS && strcmp(argv[0] + 1, "-command") == 0)
	    )
	) {
		argv++;
	}

	if (OPT_s == LOCK_SH && OPT_x == LOCK_EX && OPT_n == LOCK_NB && OPT_u == LOCK_UN) {
		/* With suitably matched constants, mode setting is much simpler */
		mode = opt & (LOCK_SH + LOCK_EX + LOCK_NB + LOCK_UN);
		if (!(mode & ~LOCK_NB))
			mode |= LOCK_EX;
	} else {
		if (opt & OPT_u)
			mode = LOCK_UN;
		else if (opt & OPT_s)
			mode = LOCK_SH;
		else
			mode = LOCK_EX;
		if (opt & OPT_n)
			mode |= LOCK_NB;
	}

	if (flock(fd, mode) != 0) {
		if (errno == EWOULDBLOCK)
			return EXIT_FAILURE;
		bb_perror_nomsg_and_die();
	}

	if (argv[0])
		return spawn_and_wait(argv);

	return EXIT_SUCCESS;
}
