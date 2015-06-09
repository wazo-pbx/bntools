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

#ifndef	_RES_BNFOS_H
#define	_RES_BNFOS_H	1

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <asterisk/config.h>
#include <asterisk/module.h>
#include <asterisk/logger.h>
#include <asterisk/lock.h>
#include <asterisk/utils.h>
#include <asterisk/cli.h>
#include <asterisk/term.h>
#include <beronet/bnfos.h>

#ifdef AST_MODULE_INFO
#define AST_VERSION_1_4
#define CONST_1_4 const
#else
#define AST_VERSION_1_2
#define CONST_1_4
#endif

#define BNFOS_CFG           "bnfos.conf"

struct res_bnfos_dev_t {
  int kick_interval;
  int kick_start_delay;

  bnfos_dev_t dev;

  CONST_1_4 char *events_cmd;

  pthread_t kt;
  char *name;

  int running;

  char *last_event;
};

extern struct res_bnfos_dev_t **bnfos_devices;
extern int bnfos_devnum;

#endif /* res_bnfos.h  */
