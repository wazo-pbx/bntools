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

/*! \file res_bnfos.c
 *
 * \brief bero*fos integration
 *
 * \author Thomas Liske <tl@beronet.com>
 */

#include "res_bnfos.h"
#include "config.h"
#include "kick.h"
#include "cli.h"

struct res_bnfos_dev_t **bnfos_devices = NULL;
int bnfos_devnum = 0;


#ifdef AST_MODULE_INFO
#define __static__ static
#else
#define __static__
#define AST_MODULE_INFO(lkey, foo, descr, ...)	\
  char *key(void) {				\
    return lkey;				\
  }						\
  int usecount(void) {				\
    return 0;					\
  }						\
  char *description(void) {			\
    return descr;				\
  }						
#endif

__static__ int load_module (void) {

	int err = 0;

  if (bnfos_init() != BNFOS_RET_OK) {
    ast_log(LOG_ERROR, "Could not initialize libbnfos!\n");
    return AST_MODULE_LOAD_DECLINE;
  }

  bnfos_register_cli();
  err = bnfos_load_config();

  if (err) {
	  bnfos_unregister_cli();
	  bnfos_destroy();
	  return AST_MODULE_LOAD_DECLINE;
  }
  
  return 0;
}

__static__ int unload_module (void) {

  int i;
  for (i=0; i<bnfos_devnum; i++)
    bnfos_kick_stop(bnfos_devices[i]);

  bnfos_unregister_cli();

  bnfos_destroy();

  bnfos_unload_config();
  return 0;
}

__static__ int reload (void) {
  return -1;
}

#ifdef AST_VERSION_1_4
#define AST_MODULE "res_bnfos"
#endif
AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_GLOBAL_SYMBOLS, "bero*fos integration",
		.load = load_module,
		.unload = unload_module,
		.reload = reload,
		);
