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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <syslog.h>
#include <string.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <beronet/bnfos.h>
#include <beronet/confmap_fos.h>
#include <string.h>

static void default_logfunc(const int prio, const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}
static bnfos_logfunc_t bnfos_log = default_logfunc;
static int bnfos_initialized = 0;


static void bnfos_key_add(bnfos_dev_t *dev, char *key) {
  int i;
  char *val = strchr(key, '=');

  if (!val)
    return;

  *(val++) = 0;

  if (!strcmp(BNFOS_CONFMAP_MKEY, key)) {
    if (strcmp(val, BNFOS_CONFMAP_MVAL))
      bnfos_log(LOG_WARNING, "Firmware version does not match, expect incorrect values!\n");
    dev->_kvmagic = 1;

    return;
  }

  if (!dev->_kvmagic)
    return;

  for(i=0; i<BNFOS_MAX_KEYS; i++) {
    if (bnfos_confmap[i].macro) {
      char kstr[0x100];

      sprintf(kstr, "%d_%s", bnfos_confmap[i].cmd, bnfos_confmap[i].key);

      if (!strcmp(key, kstr))
	break;
    }
  }

  if (i == BNFOS_MAX_KEYS)
    return;

  switch(bnfos_confmap[i].type) {
  case 'b':
    if ((strcmp("1", val)==0) ||
	(strcmp("checked", val)==0))
      dev->_kvl[i] = strdup("1");
    else
      dev->_kvl[i] = strdup("0");
    break;

  case 'a':
    {
      struct in_addr addr;

      if (inet_aton(val, &addr) == -1)
	dev->_kvl[i] = strdup("0.0.0.0");
      else
	dev->_kvl[i] = strdup(val);
    }
    break;

  case 0:
  case 's':
  case 'h':
  case 'p':
    dev->_kvl[i] = strdup(val);
    break;

  case 'd':
    dev->_kvl[i] = NULL;
    break;

  default:
    bnfos_log(LOG_WARNING, "Unhandled type '%c'!\n", bnfos_confmap[i].type);
    dev->_kvl[i] = strdup(val);
    break;
  }
}

static int bnfos_write_cb(void *ptr, size_t size, size_t nmemb, void *data) {
  int n = size*nmemb;

  bnfos_dev_t *dev = (bnfos_dev_t *)data;

  if (!dev->_kv)
    return n;

  if (dev->_db) {
      dev->_db = realloc(dev->_db, dev->_dbnum + n);
  }
  else
    dev->_db = malloc(n);

  memcpy(&dev->_db[dev->_dbnum], ptr, n);
  dev->_dbnum += n;
  
  char *r;
  while ((r = strchr(dev->_db, '\n'))) {

    *(r++) = 0;

    bnfos_key_add(dev, dev->_db);
      
    dev->_dbnum -= r-dev->_db;
    if (dev->_dbnum<=0)
      dev->_dbnum = 0;
    else
      memmove(dev->_db, r, dev->_dbnum);
  }

  return n;
}

int bnfos_init() {
  if (!bnfos_initialized)
    bnfos_log_sethandler(NULL);

  if (bnfos_initialized)
    return BNFOS_RET_ERR;

  bnfos_initialized = 1;

  gbl4_init();
  return  BNFOS_RET_OK;
}

void bnfos_destroy() {
  gbl4_destroy();

  if (!bnfos_initialized)
    return;
  
  bnfos_initialized = 0;
}

void bnfos_set_timeout(bnfos_dev_t *dev, const unsigned short to) {
  if  (dev)
    curl_easy_setopt(dev->_curl, CURLOPT_TIMEOUT, to);
  else
    gbl4_set_timeout(to);
}

int bnfos_dev_init(bnfos_dev_t *dev) {
  memset(dev, 0, sizeof(bnfos_dev_t));
  dev->address.s_addr = htonl(INADDR_BROADCAST);
  dev->port = 80;
  dev->usrpwd = NULL;

  dev->_url = NULL;
  dev->_curl = curl_easy_init();
  curl_easy_setopt(dev->_curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(dev->_curl, CURLOPT_USERAGENT, "libbnfos"); 
  curl_easy_setopt(dev->_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
  curl_easy_setopt(dev->_curl, CURLOPT_TIMEOUT, BNFOS_DEFAULT_TO);

  return BNFOS_RET_OK;
}

void bnfos_dev_destroy(bnfos_dev_t *dev) {

  if (dev->_url)
    free(dev->_url);

  curl_easy_cleanup(dev->_curl);
}

bnfos_dev_t *bnfos_dev_dup(const bnfos_dev_t *dev) {
  return NULL;
}

const char *bnfos_dev_get_addr(const bnfos_dev_t *dev) {
  if (!dev)
    return NULL;

  return inet_ntoa(dev->address);
}


int bnfos_dev_get_port(const bnfos_dev_t *dev) {
  if (!dev)
    return BNFOS_RET_ERR;

  return dev->port;
}


const uint8_t *bnfos_dev_get_mac(const bnfos_dev_t *dev) {
  if (!dev)
    return NULL;

  return dev->mac;
}

static void mapdev(gbl4_dev_t *gdev, const bnfos_dev_t *dev) {
  memcpy(gdev->mac, dev->mac, 6);
  gdev->address = dev->address;
}

static int bnfos_scanfunc(void *usr_data, const uint8_t mac[6],
			  const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
			  const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
			  const struct in_addr address, const struct in_addr netmask,
			  const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
			  const uint8_t options, const uint16_t http_port) {

  bnfos_scanfunc_t scanfunc = ((void **)usr_data)[0];
  
  int ret = scanfunc(((void **)usr_data)[1], mac,
		     bl_vers_ma, bl_vers_mi,
		     fw_vers_ma, fw_vers_mi, devmode,
		     address, netmask,
		     gateway, phy_conf, phy_state,
		     options, http_port);

  switch (ret) {
  case BNFOS_RET_OK:
    return GBL4_RET_OK;
  case BNFOS_RET_ABRT:
  default:
    return GBL4_RET_ABRT;
  }
}

int bnfos_scan(const char *addr, bnfos_scanfunc_t scanfunc, void *usr_data) {
  void *pdata[2];
  unsigned int hwmagic[] = { 0x50, 0xbb };

  pdata[0] = scanfunc;
  pdata[1] = usr_data;

  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  int ret = gbl4_get_netconf(addr, bnfos_scanfunc, pdata, sizeof(hwmagic) / sizeof(unsigned int), hwmagic);

  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

int bnfos_netconf(const bnfos_dev_t *dev,
		  const struct in_addr address, const struct in_addr netmask,
		  const struct in_addr gateway, const uint8_t phy_conf,
		  const uint8_t options, const uint16_t http_port) {

  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  gbl4_dev_t gdev;

  mapdev(&gdev, dev);

  int ret = gbl4_set_netconf(&gdev, address, netmask, gateway,
			     phy_conf, options, http_port);

  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

#define BNFOS_URL "http://%s:%d/%s"
static void bnfos_curl_init(bnfos_dev_t *dev, const char *tail) {
  dev->_url = realloc(dev->_url, strlen(BNFOS_URL) + 0x20 + strlen(tail));

  sprintf(dev->_url, BNFOS_URL, inet_ntoa(dev->address), dev->port, tail);

  curl_easy_setopt(dev->_curl, CURLOPT_USERPWD, dev->usrpwd); 
  curl_easy_setopt(dev->_curl, CURLOPT_URL, dev->_url);
  curl_easy_setopt(dev->_curl, CURLOPT_WRITEFUNCTION, bnfos_write_cb);
  curl_easy_setopt(dev->_curl, CURLOPT_WRITEDATA, dev);
}

int bnfos_key_get(bnfos_dev_t *dev, const bnfos_key_e key, char **val) {
  bnfos_init();

  return BNFOS_RET_OK;
}

int bnfos_key_set_prep(bnfos_set_t *set, const bnfos_key_e key, const char *val, char **err) {
  memset(set, 0, sizeof(bnfos_set_t));

  if ((key<0) || (key >= BNFOS_MAX_KEYS)) {
    *err = "Invalid key specified!";
    return BNFOS_RET_ERR;
  }

  set->key = key;

  *err = NULL;
  switch(bnfos_confmap[key].type) {
  case 'b':
    if ((strcasecmp("yes", val)==0) ||
	(strcasecmp("y", val)==0) ||
	(strcasecmp("true", val)==0) ||
	(strcasecmp("1", val)==0)) {
      set->val = strdup("1");
      return BNFOS_RET_OK;
    }
    
    
    if (strcasecmp("no", val) &&
	strcasecmp("n", val) &&
	strcasecmp("false", val) &&
	strcasecmp("0", val)) {
      *err = "Value has to be 0 or 1!";
      return BNFOS_RET_ERR;
    }
    
    set->val = strdup("0");
    return BNFOS_RET_OK;
    
  case 'a':
    {
      struct in_addr addr;

      if ((!val) || (inet_aton(val, &addr) == -1)) {
	*err = "No valid ip address!";
	return BNFOS_RET_ERR;
      }

      set->val = strdup(inet_ntoa(addr));
      return BNFOS_RET_OK;
    }

  case 'h':
    if (!val) {
      *err = "Host not specified!";
      return BNFOS_RET_ERR;
    }

    /* XXX check hostname for validy */

    set->val = strdup(val);
    return BNFOS_RET_OK;

  case 'p':
    {
      int v;

      v = atoi(val);

      if ((v<1) || (v>0xffff)) {
	*err = "Value not in range 1 to 65535!";
	return BNFOS_RET_ERR;
      }

      char p[6];
      sprintf(p, "%d", v);

      set->val = strdup(p);
      return BNFOS_RET_OK;
    }

  case 'd':
    if ((val) && (strlen(val) > 15)) {
      *err = "Password exceeded length 15!";
      return BNFOS_RET_ERR;
    }
    else if (!val) {
      *err = "Password not specified!";
      return BNFOS_RET_ERR;
    }

    set->val = strdup(val);
    return BNFOS_RET_OK;
  }

  *err = "Could not parse!";
  return BNFOS_RET_ERR;
}

int bnfos_key_set_do(bnfos_dev_t *dev, bnfos_set_t *set) {
  bnfos_init();
  CURLcode res;
  char tail[0x100];

  if (bnfos_confmap[set->key].parm) {
    char t[0x100];

    sprintf(t, "?cmd=%d&%s", bnfos_confmap[set->key].cmd, bnfos_confmap[set->key].parm);

#if LIBCURL_VERSION_NUM >= 0x070f04
    char *h = curl_easy_escape(dev->_curl , set->val, strlen(set->val));
#else
    char *h = curl_escape(set->val, strlen(set->val));
#endif
    sprintf(tail, t, h);
    curl_free(h);
  }
  else
    sprintf(tail, "?cmd=%d", bnfos_confmap[set->key].cmd);

  bnfos_curl_init(dev, tail);
  if ((res = curl_easy_perform(dev->_curl))) {
    bnfos_log(LOG_ERR, "%s\n", curl_easy_strerror(res));

    return BNFOS_RET_ERR;
  }

  long resp;
  curl_easy_getinfo(dev->_curl, CURLINFO_RESPONSE_CODE, &resp);

  if (resp == 401) {
    bnfos_log(LOG_ERR, "Invalid username/password to change value!\n", resp);
    return BNFOS_RET_ERR;
  }
  else if ((resp<200) || (resp >= 300)) {
    bnfos_log(LOG_ERR, "Failed to change value!\n", resp);
    return BNFOS_RET_ERR;
  }

  return BNFOS_RET_OK;
}

int bnfos_key_dump(bnfos_dev_t *dev, char **keyvals[BNFOS_MAX_KEYS]) {
  bnfos_init();
  CURLcode res;

  bnfos_curl_init(dev, "config.txt");

  dev->_kv = 1;
  if ((res = curl_easy_perform(dev->_curl))) {
    bnfos_log(LOG_ERR, "%s\n", curl_easy_strerror(res));

    return BNFOS_RET_ERR;
  }

  long resp;
  curl_easy_getinfo(dev->_curl, CURLINFO_RESPONSE_CODE, &resp);

  if (resp == 401) {
    bnfos_log(LOG_ERR, "Invalid username/password to get configuration!\n", resp);
    return BNFOS_RET_ERR;
  }
  else if ((resp<200) || (resp >= 300)) {
    bnfos_log(LOG_ERR, "Failed to get configuration!\n", resp);
    return BNFOS_RET_ERR;
  }
  
  if (!dev->_kvmagic) {
    bnfos_log(LOG_ERR, "No valid configuration information found!\n", resp);
    return BNFOS_RET_ERR;
  }

  *keyvals = dev->_kvl;

  return BNFOS_RET_OK;
}

void bnfos_log_sethandler(bnfos_logfunc_t logfunc) {
  if (!logfunc)
    bnfos_log = default_logfunc;
  else
    bnfos_log = logfunc;

  gbl4_set_loghandler(bnfos_log);
}

int bnfos_ping(const bnfos_dev_t *dev) {
  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  gbl4_dev_t gdev;

  mapdev(&gdev, dev);

  int ret = gbl4_ping(&gdev);

  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

int bnfos_kick_wdog(const bnfos_dev_t *dev) {
  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  gbl4_dev_t gdev;

  mapdev(&gdev, dev);

  int ret = gbl4_kick_wdog(&gdev);

  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

int bnfos_flash(const bnfos_dev_t *dev, int fd) {
  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  char *buff;
  struct stat sb;

  if (fstat(fd, &sb)) {
    bnfos_log(LOG_ERR, "Stat failed: %s\n", strerror(errno));
    return BNFOS_RET_ERR;
  }

  buff = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (buff == MAP_FAILED) {
    bnfos_log(LOG_ERR, "Mmap failed: %s\n", strerror(errno));
    return BNFOS_RET_ERR;
  }

  gbl4_dev_t gdev;
  mapdev(&gdev, dev);

  int ret = gbl4_flash(&gdev, buff, sb.st_size);

  munmap(buff, sb.st_size);
 
  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

int bnfos_reset(const bnfos_dev_t *dev) {
  if (gbl4_init() != GBL4_RET_OK)
    return BNFOS_RET_ERR;

  gbl4_dev_t gdev;

  mapdev(&gdev, dev);

  int ret = gbl4_reset(&gdev);

  switch (ret) {
  case GBL4_RET_OK:
    return BNFOS_RET_OK;
  case GBL4_RET_ERR:
  default:
    return BNFOS_RET_ERR;
  }
}

