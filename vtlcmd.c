/*
 * vxcmd - A utility to send a message queue to the vxtape/vxlibrary
 *	   userspace daemons
 *
 * $Id: vtlcmd.c,v 1.1.2.1 2006-08-06 07:58:44 markh Exp $
 *
 * Copyright (C) 2005 Mark Harvey markh794 at gmail dot com
 *                                mark_harvey at symantec dot com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *	FIXME: Make into user friendly utility :-)
 *
 */

static const char *Version = "$Id: vtlcmd.c,v 1.15 2007-09-26 07:58:44 markh Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include "q.h"

void
usage(char *progname) {
	fprintf(stderr, "Usage: %s DriveNo. <command>\n", progname);
	fprintf(stderr, "       %s\n", Version);
	fprintf(stderr, "       Where 'DriveNo' is the messageQ number"
			" associated with tape daemon\n");
	fprintf(stderr, "       Where 'command' can be:\n");
	fprintf(stderr, "           verbose   -> To enable verbose logging\n");
	fprintf(stderr, "           debug     -> To enable debug logging\n");
	fprintf(stderr, "           load ID   -> To 'load' media ID\n");
	fprintf(stderr, "           unload ID -> To 'unload' media ID\n");
	fprintf(stderr, "           TapeAlert # "
			"-> 64bit TapeAlert mask\n");
	fprintf(stderr, "           exit      -> To shutdown drive daemon\n");
	fprintf(stderr, "\nUsage: %s library <command>\n", progname);
	fprintf(stderr, "       Where 'command' can be:\n");
	fprintf(stderr, "           verbose   -> To enable verbose logging\n");
	fprintf(stderr, "           debug     -> To enable debug logging\n");
	fprintf(stderr, "           online    -> To enable library\n");
	fprintf(stderr, "           offline   -> To take library offline\n");
	fprintf(stderr, "           list map  -> To list map contents\n");
	fprintf(stderr, "           empty map # "
			"-> To remove media from map number #\n");
	fprintf(stderr, "           TapeAlert # "
			"-> 64bit TapeAlert mask\n");
	fprintf(stderr, "           exit      -> To shutdown library daemon\n");
}

/*
 * Poll messageQ and return:
 * - -1 on error
 * - -2 Can not open messageQ
 * -  0 No message.
 * - >0 Message size
 */
int
checkMessageQ(struct q_entry *r_entry, int q_prority, int * r_qid) {
	int mlen = 0;

	mlen = msgrcv(*r_qid, r_entry, MAXOBN, q_prority, IPC_NOWAIT);
	if (mlen > 0) {
		r_entry->mtext[mlen] = '\0';
	} else if (mlen < 0) {
		if ((*r_qid = init_queue()) == -1) {
			syslog(LOG_DAEMON|LOG_ERR,
				"Can not open message queue: %m");
			mlen = -2;
		}
	}
return mlen;
}

void
displayResponse(void) {
	int	ret;
	struct	q_entry	r_entry;
	int	r_qid = 0;

	if ((r_qid = init_queue()) == -1) {
		syslog(LOG_DAEMON|LOG_ERR,"Could not initialise message queue");
		return;
	}

	while((ret = checkMessageQ(&r_entry, LIBRARY_Q + 1, &r_qid)) < 0) {
		if (ret == -2)
			break;	// Opening messageQ problem..
		sleep(1);
	}

	printf("Contents: %s\n", r_entry.mtext);

}

int
main(int argc, char **argv) {
	int driveNo;
	int count;
	char buf[1024];
	char *p;

	if ((argc < 2) || (argc > 6)) {
		usage(argv[0]);
		exit(1);
	}

	if (! strncmp(argv[1], "-h", 2)) {
		usage(argv[0]);
		exit(1);
	}
	if (! strncmp(argv[1], "-help", 5)) {
		usage(argv[0]);
		exit(1);
	}

	p = buf;
	buf[0] = '\0';
	if (! strncmp(argv[1], "library", 7)) {
		driveNo = LIBRARY_Q;
	} else {
		driveNo = atoi(argv[1]);
		if ((driveNo <= 0) || (driveNo > LIBRARY_Q)) {
			printf("Invalid drive number\n");
			exit(1);
		}
	}

	/* Concat all args into one string */

	p = buf;
	buf[0] = '\0';

	for (count = 2; count < argc; count++) {
		strcat(p, argv[count]);
		p += strlen(argv[count]);
		strcat(p, " ");
		p += strlen(" ");
	}

	if (send_msg(buf, driveNo) < 0) {
		printf("Enter failure\n");
		exit(1);
	}

	if (! strncmp(argv[2], "list", 4))
		displayResponse();

exit(0);
}

int
send_msg(char *objname, int priority) {
	int len, s_qid;
	struct q_entry s_entry;	/* Structure to hold message */

	/* Validate name length, priority level */
	if ((len = strlen(objname)) > MAXOBN) {
		printf("Name too long\n");
		return (-1);
	}

	if (priority > LIBRARY_Q || priority < 0) {
		printf("Invalid priority level\n");
		return(-1);
	}

	/* Initialize message queue as nessary */
	if ((s_qid = init_queue()) == -1)
		return (-1);

	/* Initialize s_entry */
	s_entry.mtype = (long)priority;
	strncpy(s_entry.mtext, objname, MAXOBN);

	/* Send message, waiting if nessary */
	if (msgsnd(s_qid, &s_entry, len, 0) == -1) {
		perror("msgsnd failed");
		return (-1);
	} else {
		return (0);
	}
}
