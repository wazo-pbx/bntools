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

#ifndef _GBL4_H
#define _GBL4_H 1

#define GBL4_PREFIX_LEN 4
#define GBL4_PREFIX "GBL\004"
#define GBL4_PREFIX_CHKSUM ('G' ^ 'B' ^ 'L' ^ 4)

#include <netinet/in.h>

typedef enum {
  GBL4_RET_OK = 0,
  GBL4_RET_CONT = 0,
  GBL4_RET_ERR = -1,
  GBL4_RET_ABRT = -1,
} gbl4_ret_e;

typedef struct {
  uint8_t mac[6];
  struct in_addr address;
} gbl4_dev_t;

typedef struct {
  int len;
  struct {
    uint8_t prefix[GBL4_PREFIX_LEN];
    uint8_t cmd;
    uint8_t payload[1500-GBL4_PREFIX_LEN-2];
  } data;
} gbl4_dgram_t;

typedef struct {
  uint8_t mac[6];
  uint16_t hwmagic;
  uint8_t bl_vers_ma;
  uint8_t bl_vers_mi;
  uint8_t fw_vers_ma;
  uint8_t fw_vers_mi;
  uint8_t devmode;
  uint8_t address[4];
  uint8_t netmask[4];
  uint8_t gateway[4];
  uint8_t  phy_conf;
  uint8_t phy_state;
  uint8_t options;
  uint16_t http_port;
} gbl4_payload_get_netconf;

typedef struct {
  uint8_t mac[6];
  uint8_t address[4];
  uint8_t netmask[4];
  uint8_t gateway[4];
  uint8_t phy_conf;
  uint8_t options;
  uint16_t http_port;
} gbl4_payload_set_netconf_req;

typedef struct {
  uint8_t mac[6];
  uint8_t errcode;
} gbl4_payload_set_netconf_resp;

typedef struct {
  uint8_t mac[6];
  uint16_t n;
} gbl4_payload_ping;

typedef struct {
  uint8_t mac[6];
  uint8_t errcode;
} gbl4_payload_reset_resp;

typedef struct {
  uint8_t mac[6];
  uint16_t numpages;
  uint8_t crc;
} gbl4_payload_flash_prep;

#define GBL4_FLASH_PAGE_SIZE 512

typedef struct {
  uint8_t mac[6];
  uint16_t curpage;
  uint8_t data[GBL4_FLASH_PAGE_SIZE];
} gbl4_payload_flash_page;

enum gbl4_request_code_e {
  GBL_REQ_GET_NETCONF = 1,
  GBL_REQ_SET_NETCONF = 2,
  GBL_REQ_FLASH_PREP = 3,
  GBL_REQ_FLASH_PAGE = 4,
  GBL_REQ_RESET = 5,
  GBL_REQ_PING = 9,
  GBL_REQ_SET_MAC = 10,
  GBL_REQ_KICK_WDOG = 11,
};

#define GBL4_PHY_PROBE_10MBIT  0x01
#define GBL4_PHY_PROBE_100MBIT 0x02
#define GBL4_PHY_PROBE_AUTO    0xff

#define GBL4_PHY_LINK_BEAT     0x01
#define GBL4_PHY_LINK_100MBIT  0x02
#define GBL4_PHY_LINK_FDX      0x04

#define GBL4_OPT_DHCP          0x01
#define GBL4_OPT_HTTP_AUTH     0x02
#define GBL4_OPT_IP_ACL        0x04

#define GBL4_TO                3

typedef int (* gbl4_scanfunc_t)(void *usr_data, const uint8_t mac[6],
				const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
				const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
				const struct in_addr address, const struct in_addr netmask,
				const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
				const uint8_t options, const uint16_t http_port);
typedef void (* gbl4_logfunc_t)(const int prio, const char *format, ...);

int gbl4_init();
void gbl4_destroy();
void gbl4_set_loghandler(gbl4_logfunc_t logfunc);
void gbl4_set_timeout(const unsigned short to);

int gbl4_get_netconf(const char *addr, gbl4_scanfunc_t scanfunc, void *usr_data, int count, unsigned int *hwmagic);
int gbl4_set_netconf(const gbl4_dev_t *dev,
		     const struct in_addr address, const struct in_addr netmask,
		     const struct in_addr gateway, const uint8_t phy_conf,
		     const uint8_t options, const uint16_t http_port);
int gbl4_reset(const gbl4_dev_t *dev);
int gbl4_ping(const gbl4_dev_t *dev);
int gbl4_reset(const gbl4_dev_t *dev);
int gbl4_flash(const gbl4_dev_t *dev, const char *b, const int len);
int gbl4_kick_wdog(const gbl4_dev_t *dev);

#endif /* gbl4.h */
