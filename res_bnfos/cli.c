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

#include "res_bnfos.h"
#include "cli.h"
#include "kick.h"

static void bnfos_status(const int fd, struct res_bnfos_dev_t *dev) {
  char buff[0x100];
  int i;

  term_color(buff, dev->name, COLOR_BRWHITE, 0, sizeof(buff));
  ast_cli(fd, "[%s]\n", buff);

  ast_cli(fd, " dev_mac = ");
  for (i = 0; i < 6; ++i)
	  ast_cli(fd, "%.2hhx%s", dev->dev.mac[i], i < 5 ? ":" : "");
  ast_cli(fd, "\n");

  ast_cli(fd, " dev_ip = %s\n", ast_inet_ntoa(dev->dev.address));

  ast_cli(fd, " kick_interval = %ds\n", dev->kick_interval);
  ast_cli(fd, " kick_start_delay = %ds\n", dev->kick_start_delay);

  ast_cli(fd, " events_cmd = %s\n", dev->events_cmd);
  ast_cli(fd, " last_event = %s\n", dev->last_event);
}

static void bnfos_start(const int fd, struct res_bnfos_dev_t *dev) {
  bnfos_kick_start(dev);
}

static void bnfos_stop(const int fd, struct res_bnfos_dev_t *dev) {
  bnfos_kick_stop(dev);
}

static int bnfos_cli_wdog(int fd, int argc, char *argv[]) {
  int i;

  for (i=0; i<bnfos_devnum; i++) {					
    if ((argc > 2) && (strcasecmp(bnfos_devices[i]->name, argv[2])))
      continue;

    if (!strcmp(argv[1], "status"))
      bnfos_status(fd, bnfos_devices[i]);
    else if (!strcmp(argv[1], "start"))
      bnfos_start(fd, bnfos_devices[i]);
    else if (!strcmp(argv[1], "stop"))
      bnfos_stop(fd, bnfos_devices[i]);

    if (argc > 2)
      break;

    ast_cli(fd, "\n");
  }

  if ((argc > 2) && (i == bnfos_devnum))
    ast_cli(fd, "bero*fos '%s' is unknown\n\n", argv[2]);

  return 0;
}


static int bnfos_scanfunc(void *usr_data, const char mac[6],
			  const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
			  const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
			  const struct in_addr address, const struct in_addr netmask,
			  const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
			  const uint8_t options, const uint16_t http_port) {
  int i;
  int fd = ((int *)usr_data)[0];
  int num = ++((int *)usr_data)[1];
#ifdef AST_VERSION_1_4
  const char *buff;
#else
  char buff[0x20];
#endif

  ast_cli(fd, "#%d:\n mac: ", num);
  for(i=0; i<6; i++) {
    if (i)
      ast_cli(fd, ":");
    ast_cli(fd, "%02X", mac[i]);
  }
  ast_cli(fd, "\n");
  ast_cli(fd, " bootloader: v%d.%d\n", bl_vers_ma, bl_vers_mi);
  ast_cli(fd, " firmware: v%d.%d\n", fw_vers_ma, fw_vers_mi);
  ast_cli(fd, " device mode: %s\n", (devmode ? "FLASH MODE" : "OPERATIONAL"));
  ast_cli(fd, "\n");
#ifdef AST_VERSION_1_4
  buff = ast_inet_ntoa(address);
#else
  ast_inet_ntoa(buff, sizeof(buff), address);
#endif
  ast_cli(fd, " address: %s\n", buff);
#ifdef AST_VERSION_1_4
  buff = ast_inet_ntoa(netmask);
#else
  ast_inet_ntoa(buff, sizeof(buff), netmask);
#endif
  ast_cli(fd, " netmask: %s\n", buff);
#ifdef AST_VERSION_1_4
  buff = ast_inet_ntoa(gateway);
#else
  ast_inet_ntoa(buff, sizeof(buff), gateway);
#endif
  ast_cli(fd, " gateway: %s\n", buff);
  ast_cli(fd, " phy conf: 0x%02x (", phy_conf);
  if ((phy_conf & BNFOS_PHY_PROBE_10MBIT) && (phy_conf != BNFOS_PHY_PROBE_AUTO)) {
    ast_cli(fd, "PROBE_10MBIT");
  }
  if ((phy_conf & BNFOS_PHY_PROBE_100MBIT) && (phy_conf != BNFOS_PHY_PROBE_AUTO)) {
    if (phy_conf & BNFOS_PHY_PROBE_10MBIT)
      ast_cli(fd, " | ");
    ast_cli(fd, "PROBE_100MBIT");
  }
  else if (phy_conf == BNFOS_PHY_PROBE_AUTO) {
    ast_cli(fd, "PROBE_AUTO");
  }
  ast_cli(fd, ")\n");

  ast_cli(fd, " phy state: 0x%02x (", phy_state);
  if (phy_state & BNFOS_PHY_LINK_BEAT)
    ast_cli(fd, "LINK_BEAT | ");
  if (phy_state & BNFOS_PHY_LINK_100MBIT)
    ast_cli(fd, "LINK_100MBIT");
  else
    ast_cli(fd, "LINK_10MBIT");
  if (phy_state & BNFOS_PHY_LINK_FDX)
    ast_cli(fd, " | LINK_FDX");
  ast_cli(fd, ")\n");

  ast_cli(fd, " options: 0x%02x ", options);
  int opt=0;

  if (options & BNFOS_OPT_DHCP) {
    ast_cli(fd, "(OPT_DHCP");
    opt = 1;
  }

  if (options & BNFOS_OPT_HTTP_AUTH) {
    if (opt)
      ast_cli(fd, " | ");
    else
      ast_cli(fd, "(");

    ast_cli(fd, "OPT_HTTP_AUTH");
    opt = 1;
  }

  if (options & BNFOS_OPT_IP_ACL) {
    if (opt)
      ast_cli(fd, " | ");
    else
      ast_cli(fd, "(");

    ast_cli(fd, "OPT_IP_ACL");
    opt = 1;
  }
  if (opt)
    ast_cli(fd, ")");
  ast_cli(fd, "\n");
  
  ast_cli(fd, " http port: %d\n", http_port);
  ast_cli(fd, "\n");

  return BNFOS_RET_CONT;
}

static int bnfos_cli_scan(int fd, int argc, char *argv[]) {
  int parm[2] = {fd, 0};

  if (bnfos_scan(argc>2 ? argv[2] : NULL, bnfos_scanfunc, parm) == BNFOS_RET_OK)
    ast_cli(fd, "\n%d device%s found.\n\n", parm[1], parm[1] == 1 ? "" : "s");
  else
    ast_cli(fd, "Network scanning failed!\n");

  ast_cli(fd, "\n");
  
  return 0;
}

#ifndef AST_MODULE_INFO
static char *complete_wdog_cmd(char *line, char *word, int pos, int state) {
#else
static char *complete_wdog_cmd(const char *line, const char *word, int pos, int state) {
#endif
  int n=0;		      
  int i;		      

  for (i=0; i<bnfos_devnum; i++) {					
    if ((strlen(word) == 0) || (!strncasecmp(bnfos_devices[i]->name, word, strlen(word)))) { 
      if (n == state) {						
	return strdup(bnfos_devices[i]->name);
      }								
      
      n++;
    }
  }

  return NULL;
}

static struct ast_cli_entry bnfos_clis[] = {
  {
    {"bnfos", "status", NULL},
    bnfos_cli_wdog,
    "Shows the current status of the configured bero*fos devices.", 
    "Usage: bnfos status [BNFOS]\n"
    "\n"
    "       If BNFOS is omitted, the status of all devices is shown.\n"
    "\n",
    complete_wdog_cmd,
  },
  {
    {"bnfos", "stop", NULL},
    bnfos_cli_wdog,
    "Stops a bero*fos device timer.", 
    "Usage: bnfos stop [BNFOS]\n"
    "\n"
    "       If BNFOS is omitted, all timers are stopped.\n"
    "\n",
    complete_wdog_cmd,
  },
  {
    {"bnfos", "start", NULL},
    bnfos_cli_wdog,
    "Starts a bero*fos device timer.", 
    "Usage: bnfos start [BNFOS]\n"
    "\n"
    "       If BNFOS is omitted, all timers are started.\n"
    "\n",
    complete_wdog_cmd,
  },
  {
    {"bnfos", "scan", NULL},
    bnfos_cli_scan,
    "Scans the network for bero*fos devices.", 
    "Usage: bnfos scan [BROADCAST ADDRESS]\n"
    "\n"
    "       If BROADCAST ADDRESS is omitted, 255.255.255.255 is used.\n"
    "\n",
    NULL,
  },
};

void bnfos_register_cli() {
  ast_cli_register_multiple(bnfos_clis, sizeof(bnfos_clis) / sizeof(*bnfos_clis));
}


void bnfos_unregister_cli() {
  ast_cli_unregister_multiple(bnfos_clis, sizeof(bnfos_clis) / sizeof(*bnfos_clis));
}
