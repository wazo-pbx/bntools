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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/byteorder.h>
#include <syslog.h>
#include <pthread.h>

#include <beronet/gbl4.h>

#include "compat.h"

#define GBL4_QUERY_SRC     0
#define GBL4_QUERY_DST 50123

static void default_logfunc(const int prio, const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

static int sock = -1;
static gbl4_logfunc_t gbl4_log = default_logfunc;
static unsigned short timeout = 3;
static unsigned short reset_timeout = 8;
static int init = 0;
static pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

#define BNFOS_LOCK pthread_mutex_lock(&mux);
#define BNFOS_UNLOCK pthread_mutex_unlock(&mux);

int gbl4_init() {
  BNFOS_LOCK;

  if (init) {
    BNFOS_UNLOCK;
    return GBL4_RET_OK;
  }

  struct sockaddr_in s;
  s.sin_family = AF_INET;
  s.sin_port = htons(GBL4_QUERY_SRC);
  s.sin_addr.s_addr = htonl(INADDR_ANY);

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1) {
    if (gbl4_log)
      gbl4_log(LOG_ERR, "Failed to create socket: %s\n", strerror(errno));

    BNFOS_UNLOCK;
    return GBL4_RET_ERR;
  }

  if (bind(sock, (struct sockaddr *)&s, sizeof(s)) == -1) {
    if (gbl4_log)
      gbl4_log(LOG_ERR, "Failed to bind: %s\n", strerror(errno));
    close(sock);
    sock = -1;

    BNFOS_UNLOCK;
    return GBL4_RET_ERR;
  }

  init = 1;

  BNFOS_UNLOCK;
  return GBL4_RET_OK;
}

void gbl4_destroy() {
  BNFOS_LOCK;

  if (!init)
    return;

  init = 0;

  if (sock == -1) {
    BNFOS_UNLOCK;
    return;
  }

  close(sock);
  
  sock = -1;

  BNFOS_UNLOCK;
}

void gbl4_set_loghandler(gbl4_logfunc_t logfunc) {
  if (!logfunc)
    gbl4_log = default_logfunc;
  else
    gbl4_log = logfunc;
}

void gbl4_set_timeout(const unsigned short to) {
  timeout = to;
}

static int gbl4_calc_chksum(gbl4_dgram_t *req, const int len) {
  int i;
  uint8_t c = GBL4_PREFIX_CHKSUM;
  uint8_t old = req->data.payload[len];

  c ^= req->data.cmd;

  for(i=0; i<len; i++)
    c ^= req->data.payload[i];

  req->data.payload[len] = c;
  req->len = GBL4_PREFIX_LEN + sizeof(req->data.cmd) + len + sizeof(c);

  if (old == c)
    return GBL4_RET_CONT;

  return GBL4_RET_ABRT;
}

static void gbl4_request_init(gbl4_dgram_t *req, enum gbl4_request_code_e cmd) {
  memcpy(req->data.prefix, GBL4_PREFIX, GBL4_PREFIX_LEN);
  req->data.cmd = cmd;

  req->len = GBL4_PREFIX_LEN + sizeof(req->data.cmd);
}

static int gbl4_send_request(gbl4_dgram_t *req, const char *addr) {
  struct sockaddr_in d;

  d.sin_family = AF_INET;
  d.sin_port = htons(GBL4_QUERY_DST);
  if (addr) {
    if (inet_aton(addr, &d.sin_addr) == 0) {
      gbl4_log(LOG_ERR, "Invalid address '%s'!\n", addr);
      return GBL4_RET_ERR;
    }
  }
  else
    d.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  int i = 0;
  if (d.sin_addr.s_addr == htonl(INADDR_BROADCAST))
    i = 1;

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const void*) &i, sizeof(i)) == -1) {
    gbl4_log(LOG_ERR, "Failed to set SO_BROADCAST: %s\n", strerror(errno));
    return GBL4_RET_ERR;
  }

  if (sendto(sock, req->data.prefix, req->len, 0, (struct sockaddr *)&d, sizeof(d)) == -1) {
    gbl4_log(LOG_ERR, "Failed to send: %s\n", strerror(errno));
    return GBL4_RET_ERR;
  }

  return GBL4_RET_OK;
}

static int gbl4_recv_response(gbl4_dgram_t *req, const int cmd, const int to) {
  struct sockaddr_in s;
  int l;
  socklen_t sl = sizeof(s);
  fd_set rfds;
  struct timeval tv;

  tv.tv_sec = to;
  tv.tv_usec = 0;

  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  
  if (select(sock+1, &rfds, NULL, NULL, &tv) != 1)
    return GBL4_RET_ABRT;

  memset(req, 0, sizeof(*req));
  l = recvfrom(sock, req->data.prefix, sizeof(req->data), 0, (struct sockaddr *)&s, &sl);

  if (l < GBL4_PREFIX_LEN + 2)
    return GBL4_RET_ABRT;

  if (memcmp(req->data.prefix, GBL4_PREFIX, GBL4_PREFIX_LEN))
    return GBL4_RET_ABRT;

  if (req->data.cmd != cmd)
    return GBL4_RET_ABRT;

  req->len = l;

  return gbl4_calc_chksum(req, l - GBL4_PREFIX_LEN - sizeof(req->data.cmd) - 1);
}

static int gbl4_parse_netconf_response(gbl4_dgram_t *req, gbl4_scanfunc_t scanfunc, void *usr_data, int count, unsigned int *hwmagic) {
	int i;

  if (!scanfunc)
    return GBL4_RET_ABRT;

  gbl4_payload_get_netconf *payload = (gbl4_payload_get_netconf *)req->data.payload;

  for (i = 0; i < count; ++i) {
	  if (payload->hwmagic == cpu_to_be16(hwmagic[i]))
		  break;
  }

  if (i == count)
	  return GBL4_RET_CONT;

  const struct in_addr address = { .s_addr = *(uint32_t *)payload->address };
  const struct in_addr netmask = { .s_addr = *(uint32_t *)payload->netmask };
  const struct in_addr gateway = { .s_addr = *(uint32_t *)payload->gateway };

  return scanfunc(usr_data, payload->mac,
		  payload->bl_vers_ma, payload->bl_vers_mi,
		  payload->fw_vers_ma, payload->fw_vers_mi, payload->devmode,
		  address, netmask,
		  gateway, payload->phy_conf, payload->phy_state,
		  payload->options, be16_to_cpu(payload->http_port));
}

int gbl4_get_netconf(const char *addr, gbl4_scanfunc_t scanfunc, void *usr_data, int count, unsigned int *hwmagic) {
  int start = time(NULL);
  gbl4_dgram_t req;

  gbl4_request_init(&req, GBL_REQ_GET_NETCONF);
  gbl4_calc_chksum(&req, 0);

  BNFOS_LOCK;
  if (gbl4_send_request(&req, addr) != GBL4_RET_OK) {
    BNFOS_UNLOCK;
    return GBL4_RET_ERR;
  }

  int now = time(NULL);
  do {
    gbl4_dgram_t req;

    if (gbl4_recv_response(&req, GBL_REQ_GET_NETCONF, (now-start) <= timeout ? timeout-now+start : 0) == GBL4_RET_OK) {
      if (gbl4_parse_netconf_response(&req, scanfunc, usr_data, count, hwmagic) == GBL4_RET_ABRT) {
	BNFOS_UNLOCK;
	return GBL4_RET_OK;
      }

      while (gbl4_recv_response(&req, GBL_REQ_GET_NETCONF, 0) == GBL4_RET_OK)
	if (gbl4_parse_netconf_response(&req, scanfunc, usr_data, count, hwmagic) == GBL4_RET_ABRT) {
	  BNFOS_UNLOCK;
	  return GBL4_RET_OK;
	}
    }
  } while((now=time(NULL)) <= start + timeout);

  BNFOS_UNLOCK;
  return GBL4_RET_OK;
}

int gbl4_set_netconf(const gbl4_dev_t *dev,
		     const struct in_addr address, const struct in_addr netmask,
		     const struct in_addr gateway, const uint8_t phy_conf,
		     const uint8_t options, const uint16_t http_port) {

  gbl4_dgram_t req;

  gbl4_request_init(&req, GBL_REQ_SET_NETCONF);

  gbl4_payload_set_netconf_req *payload = (gbl4_payload_set_netconf_req *)req.data.payload;

  memcpy(payload->mac, dev->mac, sizeof(dev->mac));

  *(uint32_t *)&payload->address =  address.s_addr;
  *(uint32_t *)&payload->netmask =  netmask.s_addr;
  *(uint32_t *)&payload->gateway =  gateway.s_addr;
  payload->phy_conf = phy_conf;
  payload->options = options;
  payload->http_port = cpu_to_be16(http_port);

  gbl4_calc_chksum(&req, sizeof(gbl4_payload_set_netconf_req));

  BNFOS_LOCK;
  gbl4_send_request(&req, inet_ntoa(dev->address));

  if (gbl4_recv_response(&req, GBL_REQ_SET_NETCONF, timeout) == GBL4_RET_OK) {
    BNFOS_UNLOCK;

    gbl4_payload_set_netconf_resp *payload = (gbl4_payload_set_netconf_resp *)req.data.payload; 
    
    if (payload->errcode == 0)
      return GBL4_RET_OK;

    gbl4_log(LOG_NOTICE, "Device reported error code %d!\n", payload->errcode);
  }
  else
    BNFOS_UNLOCK;

  return GBL4_RET_ERR;
}

int gbl4_reset(const gbl4_dev_t *dev) {
  gbl4_dgram_t req;
  int len;

  gbl4_request_init(&req, GBL_REQ_RESET);

  memcpy(req.data.payload, dev->mac, sizeof(dev->mac));
  len = sizeof(dev->mac);

  gbl4_calc_chksum(&req, len);

  BNFOS_LOCK;
  gbl4_send_request(&req, inet_ntoa(dev->address));

  if (gbl4_recv_response(&req, GBL_REQ_RESET, reset_timeout) == GBL4_RET_OK) {
    BNFOS_UNLOCK;

    gbl4_payload_reset_resp *payload = (gbl4_payload_reset_resp *)req.data.payload; 
    
    if (payload->errcode == 0)
      return GBL4_RET_OK;

    gbl4_log(LOG_NOTICE, "Device reported error code %d!\n", payload->errcode);
  }
  else
    BNFOS_UNLOCK;

  return GBL4_RET_ERR;
}

int gbl4_ping(const gbl4_dev_t *dev) {
  gbl4_dgram_t req;
  gbl4_payload_ping *payload = (gbl4_payload_ping *)req.data.payload;
  int len;

  gbl4_request_init(&req, GBL_REQ_PING);

  memcpy(req.data.payload, dev->mac, sizeof(dev->mac));
  len = sizeof(dev->mac);

  payload->n = 0;
  len += sizeof(payload->n);

  gbl4_calc_chksum(&req, len);
  BNFOS_LOCK
  gbl4_send_request(&req, inet_ntoa(dev->address));

  int ret = gbl4_recv_response(&req, GBL_REQ_PING, timeout);
  BNFOS_UNLOCK;

  return ret;
}

int gbl4_flash(const gbl4_dev_t *dev, const char *buff, const int blen) {
  int i;
  char c = 0;
  gbl4_dgram_t req;
  gbl4_payload_flash_prep *plf_prep = (gbl4_payload_flash_prep *)req.data.payload;
  gbl4_payload_flash_page *plf_page = (gbl4_payload_flash_page *)req.data.payload;
  
  for(i=0; i<blen; i++)
    c ^= buff[i];

  int len;

  gbl4_request_init(&req, GBL_REQ_FLASH_PREP);

  memcpy(plf_prep->mac, dev->mac, sizeof(dev->mac));
  len = sizeof(dev->mac);

  plf_prep->numpages = cpu_to_be16( blen/GBL4_FLASH_PAGE_SIZE + (blen % GBL4_FLASH_PAGE_SIZE ? 1 : 0) );
  len += sizeof(plf_prep->numpages);

  plf_prep->crc = c;
  len += sizeof(plf_prep->crc);

  gbl4_calc_chksum(&req, len);

  BNFOS_LOCK;
  gbl4_send_request(&req, inet_ntoa(dev->address));

  if (gbl4_recv_response(&req, GBL_REQ_FLASH_PREP, timeout) != GBL4_RET_OK) {
    BNFOS_UNLOCK;
    return GBL4_RET_ERR;
  }

  /* flashing prepared... starting now */
  int pi, pos;
  
  for(pos=0, pi=0; pos<blen; pos += GBL4_FLASH_PAGE_SIZE, pi++) {
    gbl4_request_init(&req, GBL_REQ_FLASH_PAGE);

    memcpy(plf_page->mac, dev->mac, sizeof(dev->mac));
    len = sizeof(dev->mac);

    plf_page->curpage = cpu_to_be16(pi);
    len += sizeof(plf_page->curpage);
    
    memset(plf_page->data, 0, sizeof(plf_page->data)); 
    memcpy(plf_page->data, &buff[pos], (blen-pos < GBL4_FLASH_PAGE_SIZE ? blen-pos : GBL4_FLASH_PAGE_SIZE));
    len += sizeof(plf_page->data);

    gbl4_calc_chksum(&req, len);
    gbl4_send_request(&req, inet_ntoa(dev->address));

    if (gbl4_recv_response(&req, GBL_REQ_FLASH_PAGE, timeout) != GBL4_RET_OK) {
      BNFOS_UNLOCK;
      return GBL4_RET_ERR;
    }
   
	gbl4_log(LOG_INFO, "\rPage %d of %d flashed...", pi+1, blen/GBL4_FLASH_PAGE_SIZE + (blen % GBL4_FLASH_PAGE_SIZE ? 1 : 0));
  }
  gbl4_log(LOG_INFO, "\n");

  BNFOS_UNLOCK;
  return GBL4_RET_OK;
}

int gbl4_kick_wdog(const gbl4_dev_t *dev) {
  int len;
  gbl4_dgram_t req;

  gbl4_request_init(&req, GBL_REQ_KICK_WDOG);

  memcpy(req.data.payload, dev->mac, sizeof(dev->mac));
  len = sizeof(dev->mac);

  req.data.payload[len] = 0;
  len++;

  gbl4_calc_chksum(&req, len);

  BNFOS_LOCK;
  if (gbl4_send_request(&req, inet_ntoa(dev->address)) != GBL4_RET_OK) {
    BNFOS_UNLOCK;
    return GBL4_RET_ERR;
  }

  int ret = gbl4_recv_response(&req, GBL_REQ_KICK_WDOG, timeout);
  BNFOS_UNLOCK;

  return ret;
}
