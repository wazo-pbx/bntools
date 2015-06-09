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

#include "config.h"
#include "res_bnfos.h"
#include "kick.h"

#define BNFOS_CREAD_INT(var, default)					\
{	\
	CONST_1_4 char *h = ast_variable_retrieve(cfg, cat, #var);			\
	if (h)								\
	dev->var = atoi(h);						\
	else								\
	dev->var = default;						\
}

static char* get_val_from_external_config (char *dev, char *name)
{
	struct ast_config *cfg = ast_config_load("/etc/bnfos.conf");
	if (!cfg)
		return NULL;

	CONST_1_4 char *p = ast_variable_retrieve(cfg, dev, name);
	if (!p)
		return NULL;

	char *dup = strdup(p);

	ast_config_destroy(cfg);

	return dup;
}

int bnfos_load_config(void) {
	struct ast_config *cfg = ast_config_load(BNFOS_CFG);

	if (cfg == NULL) {
		ast_log(LOG_ERROR, "Could not load config file "BNFOS_CFG"!\n");
		return -1;
	}

	char *cat = NULL;
	struct res_bnfos_dev_t *dev = NULL;
	while ((cat = ast_category_browse(cfg, cat))) {

		if (dev)
			free(dev);
		dev = malloc(sizeof(struct res_bnfos_dev_t));
		dev->name = cat;

		BNFOS_CREAD_INT(kick_interval, 5);
		if (dev->kick_interval<1) {
			ast_log(LOG_ERROR, "Value of 'kick_interval' on device '%s' out of range!\n", dev->name);
			continue;
		}

		BNFOS_CREAD_INT(kick_start_delay, dev->kick_interval);
		if (dev->kick_start_delay<0) {
			ast_log(LOG_ERROR, "Value of 'kick_start_delay' on device '%s' out of range!\n", dev->name);
			continue;
		}

		{
			bnfos_dev_init(&dev->dev);
			char *mac;
			CONST_1_4 char *p = ast_variable_retrieve(cfg, cat, "dev_mac");
			if (p)
				mac = strdup(p);
			else
				mac = get_val_from_external_config(cat, "mac");
			
			if (!mac) {
				ast_log(LOG_ERROR, "Value of 'dev_mac' on device '%s' not specified!\n", dev->name);
				continue;
			}

			if (strlen(mac) != 17) {
				ast_log(LOG_ERROR, "Invalid mac address format, expecting XX:XX:XX:XX:XX:XX notation on device '%s'!\n", dev->name);
				free(mac);
				continue;
			}

			int inv = 0;
			int j;
			char *tmp = mac;
			for(j=0;(j<6) && (!inv);j++) {
				char *q;
				dev->dev.mac[j] = strtol(tmp, &q, 16);

				if (q-tmp != 2)
					inv = 1;
				else {
					tmp = q;
					if (*q)
						tmp++;
				}
			}

			if (inv) {
				ast_log(LOG_ERROR, "Invalid mac address format at '%s', expecting XX:XX:XX:XX:XX:XX notation on device '%s'!\n", tmp, dev->name);
				free(mac);
				continue;
			}

			free(mac);
		}

		{
			char *ip;
			CONST_1_4 char *p = ast_variable_retrieve(cfg, cat, "dev_ip");
			if (p)
				ip = strdup(p);
			else
				ip = get_val_from_external_config(cat, "host");

			if (ip) {
				if (inet_aton(ip, &dev->dev.address) == -1) {
					struct ast_hostent host;
					if (ast_gethostbyname(ip, &host)) {
						ast_log(LOG_ERROR, "Could not resolve hostname '%s' on device '%s'!\n", ip, dev->name);
						free(ip);
						continue;
					}

					if (inet_aton(host.hp.h_addr, &dev->dev.address) == -1) {
						ast_log(LOG_ERROR, "Invalid host address on device '%s'!\n", dev->name);
						free(ip);
						continue;
					}
				}
				free(ip);
			} else
				inet_aton("255.255.255.255", &dev->dev.address);
		}

		dev->events_cmd = ast_variable_retrieve(cfg, cat, "events_cmd");
		if (dev->events_cmd && (strlen(dev->events_cmd)==0))
			dev->events_cmd = NULL;


		bnfos_devnum++;
		bnfos_devices = realloc(bnfos_devices, sizeof(struct res_bnfos_dev_t *)*bnfos_devnum);
		bnfos_devices[bnfos_devnum-1] = dev;

		ast_log(LOG_NOTICE, "Watchdog '%s' configured.\n", dev->name);

		dev->last_event = BNFOS_EVENTS_STOPPED;
		dev->running = 0;

		{
			CONST_1_4 char *h = ast_variable_retrieve(cfg, cat, "disabled");			

			if (!h || atoi(h)==0)
				bnfos_kick_start(dev);
		}

		dev = NULL;
	}

	if (dev)
		free(dev);

	if (bnfos_devnum)
		return 0;

	return -1;
}

void bnfos_unload_config (void)
{
	int i;

	if (!bnfos_devices)
		return;

	for (i = 0; i < bnfos_devnum; ++i)
		if (bnfos_devices[i])
			free(bnfos_devices[i]);

	free(bnfos_devices);
}
