/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001  Miklos Szeredi (mszeredi@inf.bme.hu)

    This program can be distributed under the terms of the GNU LGPL.
    See the file COPYING.LIB.
*/

#include "fuse.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <signal.h>

static struct fuse *fuse;

struct fuse *fuse_get(void)
{
    return fuse;
}

static void usage(char *progname)
{
    fprintf(stderr,
            "usage: %s mountpoint [options] [-- [fusermount options]]\n"
            "Options:\n"
            "    -d      enable debug output\n"
            "    -s      disable multithreaded operation\n"
            "    -h      print help\n"
            "\n"
            "Fusermount options:\n"
            "            see 'fusermount -h'\n",
            progname);
    exit(1);
}

static void invalid_option(char *argv[], int argctr)
{
    fprintf(stderr, "invalid option: %s\n", argv[argctr]);
    usage(argv[0]);
}

static void exit_handler()
{
    if(fuse != NULL)
        fuse_exit(fuse);
}

static void set_signal_handlers()
{
    struct sigaction sa;

    sa.sa_handler = exit_handler;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) == -1 || 
	sigaction(SIGINT, &sa, NULL) == -1 || 
	sigaction(SIGTERM, &sa, NULL) == -1) {
	
	perror("Cannot set exit signal handlers");
        exit(1);
    }

    sa.sa_handler = SIG_IGN;
    
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
	perror("Cannot set ignored signals");
        exit(1);
    }
}

void fuse_main(int argc, char *argv[], const struct fuse_operations *op)
{
    int argctr;
    int flags;
    int multithreaded;
    int fuse_fd;
    char *fuse_mountpoint = NULL;
    char **fusermount_args = NULL;
    char *newargs[3];
    char *basename;
    
    flags = 0;
    multithreaded = 1;
    for(argctr = 1; argctr < argc && !fusermount_args; argctr ++) {
        if(argv[argctr][0] == '-') {
            if(strlen(argv[argctr]) == 2)
                switch(argv[argctr][1]) {
                case 'd':
                    flags |= FUSE_DEBUG;
                    break;
                    
                case 's':
                    multithreaded = 0;
                    break;
                    
                case 'h':
                    usage(argv[0]);
                    break;
                    
                case '-':
                    fusermount_args = &argv[argctr+1];
                    break;
                    
                default:
                    invalid_option(argv, argctr);
                }
            else
                invalid_option(argv, argctr);
        } else if(fuse_mountpoint == NULL)
            fuse_mountpoint = strdup(argv[argctr]);
        else
            invalid_option(argv, argctr);
    }

    if(fuse_mountpoint == NULL) {
        fprintf(stderr, "missing mountpoint\n");
        usage(argv[0]);
    }
    if(fusermount_args != NULL)
        fusermount_args -= 2; /* Hack! */
    else {
        fusermount_args = newargs;
        fusermount_args[2] = NULL;
    }
    
    basename = strrchr(argv[0], '/');
    if(basename == NULL)
        basename = argv[0];
    else if(basename[1] != '\0')
        basename++;

    fusermount_args[0] = "-n";
    fusermount_args[1] = basename;

    fuse_fd = fuse_mount(fuse_mountpoint, (const char **) fusermount_args);
    if(fuse_fd == -1)
        exit(1);

    set_signal_handlers();

    fuse = fuse_new(fuse_fd, flags, op);
    if(fuse == NULL)
        exit(1);

    if(multithreaded)
        fuse_loop_mt(fuse);
    else
        fuse_loop(fuse);

    close(fuse_fd);
    fuse_unmount(fuse_mountpoint);
}

