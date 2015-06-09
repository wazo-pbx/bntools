/*
 * libbnfos -- The bero*fos API library.
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

#ifndef _BNFOS_BNFOS_H
#define _BNFOS_BNFOS_H 1

#include <sys/types.h>
#include <netinet/in.h>
#include <beronet/gbl4.h>
#include <curl/curl.h>

typedef enum {
  BNFOS_RET_OK = 0,
  BNFOS_RET_CONT = 0,
  BNFOS_RET_ERR = -1,
  BNFOS_RET_ABRT = -1,
} bnfos_ret_e;

typedef int (* bnfos_scanfunc_t)(void *usr_data, const uint8_t mac[6],
				 const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
				 const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
				 const struct in_addr address, const struct in_addr netmask,
				 const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
				 const uint8_t options, const uint16_t http_port);
typedef void (* bnfos_logfunc_t)(const int prio, const char *format, ...);

typedef enum {
  BNFOS_KEY_SCENARIO = 0,

  BNFOS_KEY_MODE,
  BNFOS_KEY_MODE_DEF,

  BNFOS_KEY_PP1,
  BNFOS_KEY_PP1_DEF,
  BNFOS_KEY_PP2,
  BNFOS_KEY_PP2_DEF,

  BNFOS_KEY_HOSTNAME,
  
  BNFOS_KEY_ADDRESS,
  BNFOS_KEY_NETMASK,
  BNFOS_KEY_GATEWAY,
  BNFOS_KEY_DNS,
  BNFOS_KEY_DHCP,
  BNFOS_KEY_PORT,
  BNFOS_KEY_PWD,
  BNFOS_KEY_APWD,

  BNFOS_KEY_SMTP_SERVER,
  BNFOS_KEY_SMTP_FROM,
  BNFOS_KEY_SMTP_TO,
  BNFOS_KEY_SMTP_TEST,

  BNFOS_KEY_SYSLOG,
  BNFOS_KEY_SYSLOG_SERVER,
  BNFOS_KEY_SYSLOG_PORT,

  BNFOS_KEY_WDOG,
  BNFOS_KEY_WDOG_DEF,
  BNFOS_KEY_WDOG_STATE,
  BNFOS_KEY_WDOG_ITIME,
  BNFOS_KEY_WDOG_AUDIO,
  BNFOS_KEY_WDOG_SMTP,
  BNFOS_KEY_WDOG_RTIME,

  BNFOS_MAX_KEYS,
} bnfos_key_e;

typedef struct {
  bnfos_key_e key;
  char *val;
} bnfos_set_t;

typedef struct {
  uint8_t mac[6];
  struct in_addr address;
  int port;
  char *usrpwd;

  char *_url;
  CURL *_curl;

  int _kv;
  int _kvmagic;
  char *_kvl[BNFOS_MAX_KEYS];
  char *_db;
  int _dbnum;
} bnfos_dev_t;

#define BNFOS_PHY_PROBE_10MBIT  GBL4_PHY_PROBE_10MBIT
#define BNFOS_PHY_PROBE_100MBIT GBL4_PHY_PROBE_100MBIT
//#define BNFOS_PHY_PROBE_AUTO    GBL4_PHY_PROBE_AUTO
#define BNFOS_PHY_PROBE_AUTO    (GBL4_PHY_PROBE_10MBIT | GBL4_PHY_PROBE_100MBIT)

#define BNFOS_PHY_LINK_BEAT     GBL4_PHY_LINK_BEAT
#define BNFOS_PHY_LINK_100MBIT  GBL4_PHY_LINK_100MBIT
#define BNFOS_PHY_LINK_FDX      GBL4_PHY_LINK_FDX

#define BNFOS_OPT_DHCP          GBL4_OPT_DHCP
#define BNFOS_OPT_HTTP_AUTH     GBL4_OPT_HTTP_AUTH
#define BNFOS_OPT_IP_ACL        GBL4_OPT_IP_ACL

#define BNFOS_DEFAULT_TO        GBL4_TO

/* initialize libbnfos */
int bnfos_init();

/* free resources allocated by bnfos_init() */
void bnfos_destroy();

/* initialize device structure */
int bnfos_dev_init(bnfos_dev_t *dev);

/* free resources allocated by bnfos_dev_init */
void bnfos_dev_destroy(bnfos_dev_t *dev);

/* set timeout for operation on a device */
void bnfos_set_timeout(bnfos_dev_t *dev, const unsigned short to);

/* duplicate a device structure */
bnfos_dev_t *bnfos_dev_dup(const bnfos_dev_t *dev);

/* retrieve IP address from a device structure  */
const char *bnfos_dev_get_addr(const bnfos_dev_t *dev);

/* retrieve HTTP port from a device structure  */
int bnfos_dev_get_port(const bnfos_dev_t *dev);

/* retrieve MAC address from a device structure  */
const uint8_t *bnfos_dev_get_mac(const bnfos_dev_t *dev);

/* scan for devices - for each device found scanfunc is called back */
int bnfos_scan(const char *addr, bnfos_scanfunc_t scanfunc, void *usr_data);

/* set network configuration of a device (flash mode only) */
int bnfos_netconf(const bnfos_dev_t *dev,
		  const struct in_addr address, const struct in_addr netmask,
		  const struct in_addr gateway, const uint8_t phy_conf,
		  const uint8_t options, const uint16_t http_port);

/* prepare changing a configuration value */
int bnfos_key_set_prep(bnfos_set_t *set, const bnfos_key_e key, const char *val, char **err);

/* do the configuration value change */
int bnfos_key_set_do(bnfos_dev_t *dev, bnfos_set_t *set);

/* returns all configuration key-value-pairs of a device */
int bnfos_key_dump(bnfos_dev_t *dev, char **keyvals[BNFOS_MAX_KEYS]);

/* register handler function for log messages */
void bnfos_log_sethandler(bnfos_logfunc_t logfunc);

/* kick the watchdog of a device */
int bnfos_kick_wdog(const bnfos_dev_t *dev);

/* ping a device */
int bnfos_ping(const bnfos_dev_t *dev);

/* update firmware on a device (flash mode only) */
int bnfos_flash(const bnfos_dev_t *dev, int fd);

/* erase EEPROM (firmware and settings) of a device (flash mode only) */
int bnfos_reset(const bnfos_dev_t *dev);

#endif /* bnfos.h */
