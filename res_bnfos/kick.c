/*
 * res_bnfos -- The bero*fos Asterisk module.
 *
 * Copyright (C) 2006, beroNet GmbH
 *
 * Thomas Liske <thomas.liske@beronet.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 *
 */

#include "kick.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

static void bnfos_events_run(struct res_bnfos_dev_t *dev, char *event)
{
	dev->last_event = event;
	ast_log(LOG_NOTICE, "[%s]: %s\n", dev->name, event);

	if (!dev->events_cmd)
		return;

	int pid = fork();

	switch(pid) {
	case -1:
		ast_log(LOG_ERROR, "Could not fork events_cmd for bero*fos '%s': %s\n", dev->name, strerror(errno));
		break;
	case 0:
		execlp(dev->events_cmd, dev->name, event, NULL);
		ast_log(LOG_ERROR, "Could not run events_cmd for bero*fos '%s': %s\n", dev->name, strerror(errno));

		exit(-1);
	}

	waitpid(pid, NULL, 0);
}

void *bnfos_kicker(void *p)
{
	struct res_bnfos_dev_t *dev = p;

	bnfos_events_run(dev, BNFOS_EVENTS_STARTING);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if (dev->kick_start_delay) {
		struct timeval to;
		int i = dev->kick_start_delay;
		int j;
		for (; i > 0; --i) {
			for (j = 0; j < 10; ++j) {
				if (!dev->running)
					return NULL;
				to.tv_sec = 0;
				to.tv_usec = 100000;
				select(0, NULL, NULL, NULL, &to);
			}
		}
	}

	bnfos_events_run(dev, BNFOS_EVENTS_RUNNING);

	int now = time(NULL);
	int last_kick = now;
	int next_kick = dev->kick_interval;

	while (dev->running) {
		if (next_kick > 0) {
			struct timeval to;
			int i;

			for (; next_kick > 0; --next_kick) {
				for (i = 0; i < 10; ++i) {
					if (!dev->running)
						return NULL;
					to.tv_sec = 0;
					to.tv_usec = 100000;
					select(0, NULL, NULL, NULL, &to);
				}
			}
		}

		now = time(NULL);
		next_kick = dev->kick_interval + last_kick - now;

		if (next_kick < 1) {
			last_kick = now;

			if (bnfos_kick_wdog(&dev->dev) != BNFOS_RET_OK) {
				ast_log(LOG_WARNING, "Failed to kick bero*fos '%s'!\n", dev->name);

				if (dev->last_event != BNFOS_EVENTS_WDOG_FAILED) {
					dev->last_event = BNFOS_EVENTS_WDOG_FAILED;
					bnfos_events_run(dev, BNFOS_EVENTS_WDOG_FAILED);
				}
			}
			else {
				if (dev->last_event == BNFOS_EVENTS_WDOG_FAILED)
					ast_log(LOG_NOTICE, "bero*fos '%s' successfully kicked\n", dev->name);

				if (dev->last_event != BNFOS_EVENTS_WDOG_OK) {
					dev->last_event = BNFOS_EVENTS_WDOG_OK;
					bnfos_events_run(dev, BNFOS_EVENTS_WDOG_OK);
				}
			}
		}
	}

	return NULL;
}

void bnfos_kick_start(struct res_bnfos_dev_t *dev)
{
	if (dev->running)
		return;

	dev->running = 1;

	ast_pthread_create(&dev->kt, NULL, bnfos_kicker, dev);
}

void bnfos_kick_stop(struct res_bnfos_dev_t *dev)
{
	if (!dev->running)
		return;

	dev->running = 0;

	pthread_join(dev->kt, NULL);

	bnfos_events_run(dev, BNFOS_EVENTS_STOPPED);
}
