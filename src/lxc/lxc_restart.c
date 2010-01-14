/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2010
 *
 * Authors:
 * Daniel Lezcano <dlezcano at fr.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "log.h"
#include "lxc.h"
#include "conf.h"
#include "config.h"
#include "confile.h"
#include "arguments.h"

lxc_log_define(lxc_restart_ui, lxc_restart);

static int my_checker(const struct lxc_arguments* args)
{
	if (!args->statefile) {
		lxc_error(args, "no statefile specified");
		return -1;
	}

	return 0;
}

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	switch (c) {
	case 'd': args->statefile = arg; break;
	case 'f': args->rcfile = arg; break;
	case 'p': args->flags = LXC_FLAG_PAUSE; break;
	}

	return 0;
}

static const struct option my_longopts[] = {
	{"directory", required_argument, 0, 'd'},
	{"rcfile", required_argument, 0, 'f'},
	{"pause", no_argument, 0, 'p'},
	LXC_COMMON_OPTIONS
};

static struct lxc_arguments my_args = {
	.progname = "lxc-restart",
	.help     = "\
--name=NAME --directory STATEFILE\n\
\n\
lxc-restart restarts from STATEFILE the NAME container\n\
\n\
Options :\n\
  -n, --name=NAME      NAME for name of the container\n\
  -p, --pause          do not release the container after the restart\n\
  -d, --directory=STATEFILE for name of statefile\n\
  -f, --rcfile=FILE Load configuration file FILE\n",
	.options  = my_longopts,
	.parser   = my_parser,
	.checker  = my_checker,
};

int main(int argc, char *argv[])
{
	char *rcfile = NULL;
	struct lxc_conf *conf;

	if (lxc_arguments_parse(&my_args, argc, argv))
		return -1;

	if (lxc_log_init(my_args.log_file, my_args.log_priority,
			 my_args.progname, my_args.quiet))
		return -1;

	/* rcfile is specified in the cli option */
	if (my_args.rcfile)
		rcfile = (char *)my_args.rcfile;
	else {
		if (!asprintf(&rcfile, LXCPATH "/%s/config", my_args.name)) {
			SYSERROR("failed to allocate memory");
			return -1;
		}

		/* container configuration does not exist */
		if (access(rcfile, F_OK)) {
			free(rcfile);
			rcfile = NULL;
		}
	}

	conf = lxc_conf_init();
	if (!conf) {
		ERROR("failed to initialize configuration");
		return -1;
	}

	if (rcfile && lxc_config_read(rcfile, conf)) {
		ERROR("failed to read configuration file");
		return -1;
	}

	return lxc_restart(my_args.name, my_args.statefile, conf,
			   my_args.flags);
}
