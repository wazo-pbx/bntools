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

#ifndef	_KICK_H
#define	_KICK_H	1

#include "res_bnfos.h"

#define BNFOS_EVENTS_STARTING    "STARTING"
#define BNFOS_EVENTS_RUNNING     "RUNNING"
#define BNFOS_EVENTS_WDOG_OK     "WDOG_OK"
#define BNFOS_EVENTS_WDOG_FAILED "WDOG_FAILED"
#define BNFOS_EVENTS_STOPPED     "STOPPED"

void bnfos_kick_start(struct res_bnfos_dev_t *dev);
void bnfos_kick_stop(struct res_bnfos_dev_t *dev);

#endif /* kick.h  */
