/*
 * libbnrps -- The bero*rps API library.
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

#ifndef _BNRPS_BNRPS_H
#define _BNRPS_BNRPS_H 1

#include <sys/types.h>
#include <netinet/in.h>
#include <beronet/gbl4.h>
#include <curl/curl.h>

typedef enum {
  BNRPS_RET_OK = 0,
  BNRPS_RET_CONT = 0,
  BNRPS_RET_ERR = -1,
  BNRPS_RET_ABRT = -1,
} bnrps_ret_e;

typedef int (* bnrps_scanfunc_t)(void *usr_data, const uint8_t mac[6],
				 const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
				 const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
				 const struct in_addr address, const struct in_addr netmask,
				 const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
				 const uint8_t options, const uint16_t http_port);
typedef void (* bnrps_logfunc_t)(const int prio, const char *format, ...);

typedef enum {
  BNRPS_KEY_PP1,
  BNRPS_KEY_PP2,
  BNRPS_KEY_PP3,
  BNRPS_KEY_PP4,
  BNRPS_KEY_PP5,
  BNRPS_KEY_PP6,
  BNRPS_KEY_PP7,
  BNRPS_KEY_PP8,

  BNRPS_KEY_HOSTNAME,
  BNRPS_KEY_ADDRESS,
  BNRPS_KEY_NETMASK,
  BNRPS_KEY_GATEWAY,
  BNRPS_KEY_DHCP,
  BNRPS_KEY_PORT,
  BNRPS_KEY_PWD,
  BNRPS_KEY_APWD,
  BNRPS_KEY_UPWD,

  BNRPS_KEY_SYSLOG,
  BNRPS_KEY_SYSLOG_SERVER,
  BNRPS_KEY_SYSLOG_PORT,

  BNRPS_MAX_KEYS,
} bnrps_key_e;

typedef struct {
  bnrps_key_e key;
  char *val;
} bnrps_set_t;

typedef struct {
  uint8_t mac[6];
  struct in_addr address;
  int port;
  char *usrpwd;

  char *_url;
  CURL *_curl;

  int _kv;
  int _kvmagic;
  char *_kvl[BNRPS_MAX_KEYS];
  char *_db;
  int _dbnum;
} bnrps_dev_t;

#define BNRPS_PHY_PROBE_10MBIT  GBL4_PHY_PROBE_10MBIT
#define BNRPS_PHY_PROBE_100MBIT GBL4_PHY_PROBE_100MBIT
#define BNRPS_PHY_PROBE_AUTO    GBL4_PHY_PROBE_AUTO

#define BNRPS_PHY_LINK_BEAT     GBL4_PHY_LINK_BEAT
#define BNRPS_PHY_LINK_100MBIT  GBL4_PHY_LINK_100MBIT
#define BNRPS_PHY_LINK_FDX      GBL4_PHY_LINK_FDX

#define BNRPS_OPT_DHCP          GBL4_OPT_DHCP
#define BNRPS_OPT_HTTP_AUTH     GBL4_OPT_HTTP_AUTH
#define BNRPS_OPT_IP_ACL        GBL4_OPT_IP_ACL

#define BNRPS_DEFAULT_TO        GBL4_TO

/* initialize libbnrps */
int bnrps_init();

/* free resources allocated by bnrps_init() */
void bnrps_destroy();

/* initialize device structure */
int bnrps_dev_init(bnrps_dev_t *dev);

/* free resources allocated by bnrps_dev_init */
void bnrps_dev_destroy(bnrps_dev_t *dev);

/* set timeout for operation on a device */
void bnrps_set_timeout(bnrps_dev_t *dev, const unsigned short to);

/* duplicate a device structure */
bnrps_dev_t *bnrps_dev_dup(const bnrps_dev_t *dev);

/* retrieve IP address from a device structure  */
const char *bnrps_dev_get_addr(const bnrps_dev_t *dev);

/* retrieve HTTP port from a device structure  */
int bnrps_dev_get_port(const bnrps_dev_t *dev);

/* retrieve MAC address from a device structure  */
const uint8_t *bnrps_dev_get_mac(const bnrps_dev_t *dev);

/* scan for devices - for each device found scanfunc is called back */
int bnrps_scan(const char *addr, bnrps_scanfunc_t scanfunc, void *usr_data);

/* set network configuration of a device (flash mode only) */
int bnrps_netconf(const bnrps_dev_t *dev,
		  const struct in_addr address, const struct in_addr netmask,
		  const struct in_addr gateway, const uint8_t phy_conf,
		  const uint8_t options, const uint16_t http_port);

/* prepare changing a configuration value */
int bnrps_key_set_prep(bnrps_set_t *set, const bnrps_key_e key, const char *val, char **err);

/* do the configuration value change */
int bnrps_key_set_do(bnrps_dev_t *dev, bnrps_set_t *set);

/* returns all configuration key-value-pairs of a device */
int bnrps_key_dump(bnrps_dev_t *dev, char **keyvals[BNRPS_MAX_KEYS]);

/* register handler function for log messages */
void bnrps_log_sethandler(bnrps_logfunc_t logfunc);

/* ping a device */
int bnrps_ping(const bnrps_dev_t *dev);

/* update firmware on a device (flash mode only) */
int bnrps_flash(const bnrps_dev_t *dev, int fd);

/* erase EEPROM (firmware and settings) of a device (flash mode only) */
int bnrps_reset(const bnrps_dev_t *dev);

#endif /* bnrps.h */
