/*
 * bnfos -- The bero*fos CLI.
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <beronet/bnfos.h>
#include <beronet/confmap_fos.h>
#include <getopt.h>
#include <netdb.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

struct scanarg {
	int cnt;
	FILE *cfg;
};

/* log handler for STDERR and SYSLOG */
static void stderr_logfunc(const int prio, const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  if (prio >= LOG_ERR)
    vfprintf(stderr, format, ap);
  else
    vfprintf(stdout, format, ap);
  va_end(ap);
}
static void syslog_logfunc(const int prio, const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  vsyslog(prio, format, ap);
  va_end(ap);
}
bnfos_logfunc_t logmsg = stderr_logfunc;

/* callback function for checking device mode (flash / operational) */
static int modecheckfunc(void *usr_data, const uint8_t mac[6],
	     const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
	     const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
	     const struct in_addr address, const struct in_addr netmask,
	     const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
	     const uint8_t options, const uint16_t http_port) {

	*(int *)usr_data = devmode;

  return BNFOS_RET_ABRT;
}

/* scanning callback function */
static int scanfunc(void *usr_data, const uint8_t mac[6],
	     const uint8_t bl_vers_ma, const uint8_t bl_vers_mi,
	     const uint8_t fw_vers_ma, const uint8_t fw_vers_mi, const uint8_t devmode,
	     const struct in_addr address, const struct in_addr netmask,
	     const struct in_addr gateway, const uint8_t phy_conf, const uint8_t phy_state,
	     const uint8_t options, const uint16_t http_port) {
  int i;
  struct scanarg *arg = usr_data;
  int num = ++arg->cnt;
  char buf[3];

  if (arg->cfg) {
	  fputs("\n[fos", arg->cfg);
	  fputc('0' + num, arg->cfg);
	  fputs("]\n", arg->cfg);
  }

  printf("#%d:\n mac: ", num);
  if (arg->cfg)
	  fputs("mac = ", arg->cfg);
  for(i=0; i<6; i++) {
    if (i) {
      printf(":");
	  if (arg->cfg)
		  fputc(':', arg->cfg);
	}
	sprintf(buf, "%02hhX", mac[i]);
	printf(buf);
	if (arg->cfg)
		fputs(buf, arg->cfg);
  }
  if (arg->cfg)
	  fputc('\n', arg->cfg);
  printf("\n");
  printf(" bootloader: v%d.%d\n", bl_vers_ma, bl_vers_mi);
  printf(" firmware: v%d.%d\n", fw_vers_ma, fw_vers_mi);
  printf(" device mode: %s\n", (devmode ? "FLASH MODE" : "OPERATIONAL"));
  printf("\n");
  printf(" address: %s\n", inet_ntoa(address));
  if (arg->cfg) {
	  if (address.s_addr)
		  fputs("host = ", arg->cfg);
	  else
		  fputs(";host = ", arg->cfg);
	  fputs(inet_ntoa(address), arg->cfg);
	  fputc('\n', arg->cfg);
  }
  printf(" netmask: %s\n", inet_ntoa(netmask));
  printf(" gateway: %s\n", inet_ntoa(gateway));
  printf(" phy conf: 0x%02x (", phy_conf);
  if ((phy_conf & BNFOS_PHY_PROBE_10MBIT) && (phy_conf != BNFOS_PHY_PROBE_AUTO)) {
    printf("PROBE_10MBIT");
  }
  if ((phy_conf & BNFOS_PHY_PROBE_100MBIT) && (phy_conf != BNFOS_PHY_PROBE_AUTO)) {
    if (phy_conf & BNFOS_PHY_PROBE_10MBIT)
      printf(" | ");
    printf("PROBE_100MBIT");
  }
  else if (phy_conf == BNFOS_PHY_PROBE_AUTO) {
    printf("PROBE_AUTO");
  }
  printf(")\n");

  printf(" phy state: 0x%02x (", phy_state);
  if (phy_state & BNFOS_PHY_LINK_BEAT)
    printf("LINK_BEAT | ");
  if (phy_state & BNFOS_PHY_LINK_100MBIT)
    printf("LINK_100MBIT");
  else
    printf("LINK_10MBIT");
  if (phy_state & BNFOS_PHY_LINK_FDX)
    printf(" | LINK_FDX");
  printf(")\n");

  printf(" options: 0x%02x ", options);
  int opt=0;

  if (options & BNFOS_OPT_DHCP) {
    printf("(OPT_DHCP");
    opt = 1;
  }

  if (options & BNFOS_OPT_HTTP_AUTH) {
    if (opt)
      printf(" | ");
    else
      printf("(");

    printf("OPT_HTTP_AUTH");
    opt = 1;
  }

  if (options & BNFOS_OPT_IP_ACL) {
    if (opt)
      printf(" | ");
    else
      printf("(");

    printf("OPT_IP_ACL");
    opt = 1;
  }
  if (opt)
    printf(")");
  printf("\n");
  
  printf(" http port: %d\n", http_port);
  printf("\n");

  if (arg->cfg)
	  fputs("#login = <user>:<password>\n", arg->cfg);

  return BNFOS_RET_CONT;
}

/* operation modes */
typedef enum {
  MODE_SCAN = 0,
  MODE_NETCONF,
  MODE_PING,
  MODE_KICK,
  MODE_RESET,
  MODE_FLASH,
  MODE_SET,
  MODE_GET,
  MODE_SHOW,

  MODE_NONE,
} bnfos_mode_e;

/* arguments, with description and mandatory/optional information for each mode */
#define AC_DEFAULT 1
#define AC_NETCONF 2
#define NUM_AC     2
static struct {
  uint mask;
  char *prefix;
} argclasses[NUM_AC] = {
  {AC_DEFAULT, ""},
  {AC_NETCONF, "configure "},
};

/* occ :=
 * 	0 -> not allowed
 * 	1 -> required
 * 	2 -> allowed
 * 	3 -> required, but has an alternative
 * 	4 -> allowed, but has an alternative
 * 	5 -> alternative
 */
static struct {
  char c;
  char *descr;
  char *val;
  char *key;
  uint ac;
  int occ[MODE_NONE];
} shortopts[] = {
  {'m', "mac address of the device (notation: XX:XX:XX:XX:XX:XX)", NULL, "mac", AC_DEFAULT, {0, 3, 3, 3, 3, 3, 0, 0, 0}},
  {'h', "hostname or ip address of the device (default: 255.255.255.255)", NULL, "host", AC_DEFAULT, {2, 4, 4, 4, 4, 3, 3, 3, 3}},
  {'i', "ip address (default: 0.0.0.0)", NULL, "ip", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'n', "netmask address", NULL, "netmask", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'g', "gateway address (default: 0.0.0.0)", NULL, "gateway", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'e', "ethernet phy configuration (values: 0=auto; 1=10mbit; 2=100mbit; default: 0)", NULL, "phy", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'d', "enable dhcp (1=on; 0=off; default: 1)", NULL, "status", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'a', "enable http auth (1=on; 0=off; default: 0)", NULL, "status", AC_NETCONF, {0, 2, 0, 0, 0, 0, 0, 0, 0}},
  {'p', "http port (default: 80)", NULL, "port", AC_DEFAULT|AC_NETCONF, {0, 2, 0, 0, 0, 0, 2, 2, 2}},
  {'u', "authentification data (notation: <username>:<password>)", NULL, "usrpwd", AC_DEFAULT, {0, 0, 0, 0, 0, 0, 2, 2, 2}},
  {'c', "continuous operation", NULL, NULL, AC_DEFAULT, {2, 0, 2, 2, 0, 0, 0, 2, 2}},
  {'b', "fork to background (implies -s)", NULL, NULL, AC_DEFAULT, {0, 0, 0, 2, 0, 0, 0, 0, 0}},
  {'w', "wait 'time' seconds between continuous operations (default: 2)", NULL, "time", AC_DEFAULT, {2, 0, 2, 2, 0, 0, 0, 2, 2}},
  {'t', "timeout in seconds (default: 3)", NULL, "sec", AC_DEFAULT, {2, 2, 2, 2, 0, 2, 2, 2, 2}},
  {'s', "enable syslog output", NULL, NULL, AC_DEFAULT, {2, 2, 2, 2, 2, 2, 2, 2, 2}},
  {'x', "write scan results to " CONFFILE, NULL, NULL, AC_DEFAULT, {2, 0, 0, 0, 0, 0, 0, 0, 0}},
  {'f', "load mac and/or ip address from " CONFFILE, NULL, "id", AC_DEFAULT, {0, 5, 5, 5, 5, 5, 5, 5, 5}},
  {0, NULL, NULL, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

/* corresponding getopts_long string */
#define SHORTOPTS "m:h:i:n:g:e:d:a:p:u:cbw:t:sxf:"

/* mode descriptions */
const static char *longops_descr[MODE_NONE+1][2] = {
  {"scan the network for devices", NULL},
  {"configure network parameters", NULL},
  {"send ping request", NULL},
  {"kick watchdog timer", NULL},
  {"reset to default values (also deletes firmware)", NULL},
  {"upload firmware  (flash mode only)", "file"},
  {"set configuration value", "key>=<value"},
  {"show single configuration or status value", "key"},
  {"shows configuration and status", NULL},
  {NULL, NULL}
};

/* corresponding getopts_long struct */
const static struct option longopts[MODE_NONE+1] = {
  {"scan", 0, NULL, 0},
  {"netconf", 0, NULL, 0},
  {"ping", 0, NULL, 0},
  {"kick", 0, NULL, 0},
  {"reset", 0, NULL, 0},
  {"flash", 1, NULL, 0},
  {"set", 1, NULL, 0},
  {"get", 1, NULL, 0},
  {"show", 0, NULL, 0},

  { NULL, 0, NULL, 0},
};

/* keyword description for --set / --get */
static struct {
  char *keyword;
  char *descr;
} keys[BNFOS_MAX_KEYS] = {
  {"scenario", "scenario (0=fallback; 1=bypass)"},

  {"mode", "relais mode (0=A--D; 1=A--B or A--B,C--D)"},
  {"modedef", "default relais mode (0=A--D; 1=A--B or A--B,C--D)"},

  {"power1", "state of powerport 1 (0=off; 1=on)"},
  {"power1def", "default state of powerport 1 (0=off; 1=on)"},
  {"power2", "state of powerport 2 (0=off; 1=on)"},
  {"power2def", "default state of powerport 2 (0=off; 1=on)"},

  {"hostname", "device hostname"},

  {"address", "ip address"},
  {"netmask", "netmask address"},
  {"gateway", "gateway address"},
  {"dns", "dns server address"},
  {"dhcp", "query dhcp server (0=off; 1=on)"},
  {"port", "http listen port"},
  {"pwd", "http password protection (0=off; 1=on)"},
  {"apwd", "admin password"},
  
  {"smtpserv", "smtp server"},
  {"smtpfrom", "smtp sender address"},
  {"smtpto", "smtp destination address"},
  {"smtptest", "trigger testmail"},

  {"syslog", "syslog logging (0=off; 1=on)"},
  {"slgip", "syslog server ip"},
  {"slgpt", "syslog server port"},
  {"wdog", "watchdog enable (0=off; 1=on)"},
  {"wdogdef", "default watchdog enable (0=off; 1=on)"},
  {"wdogstate", "watchdog state (0=off; 1=on; 2=failure)"},
  {"wdogitime", "watchdog intervall time"},
  {"wdogaudio", "watchdog audio alarm (0=off; 1=on)"},
  {"wdogmail", "watchdog alarm mails (0=off; 1=on)"},
  {"wdogrtime", "watchdog remaining time to failure"},
};

static void show_keys(const int inv) {
  int i;

  switch (inv) {
  case 1:
    fprintf(stderr, "Invalid key-value pair given, valid pairs:\n");
    break;
  case 2:
    fprintf(stderr, "No key-value pair given, valid pairs:\n");
    break;
  case 3:
    fprintf(stderr, "Invalid key given, valid keys:\n");
    break;
  default:
    fprintf(stderr, " valid '--set' <key>=<value> pairs / '--get' keys:\n");
    break;
  }

  for(i=0; i<BNFOS_MAX_KEYS; i++) {
    int len = strlen(keys[i].keyword);

    if (inv!=3) {
      fprintf(stderr, "  %s=", keys[i].keyword);
      switch(bnfos_confmap[i].type) {
      case 'b':
	fprintf(stderr, "{0|1}");
	len += 5;
	break;
      case 'p':
	fprintf(stderr, "{1..65535}");
	len += 10;
	break;
      default:
	fprintf(stderr, "<%s>", bnfos_confmap[i].key);
	len += 2 + strlen(bnfos_confmap[i].key);
	break;
      }
    }
    else
      fprintf(stderr, "  %s", keys[i].keyword);

    while(len++<20)
      fprintf(stderr, " ");

	if (bnfos_confmap[i].macro)
		fprintf(stderr, "%s\n", keys[i].descr);
	else
		fprintf(stderr, "%s (only --set)\n", keys[i].descr);
  }
}

/* dynamicly (!) build usage text */
static void show_usage() {
  int i;
  int j;
  int has_reqalt;
  int has_optalt;

  fprintf(stderr, "\nUsage: bnfos <MODE> ...\n\n");

  fprintf(stderr, " operation modes:\n");
  for(i=0; longopts[i].name; i++) {
    fprintf(stderr, "  --%s", longopts[i].name);

    switch(longopts[i].has_arg) {
    case 1:
      fprintf(stderr, " <%s>", longops_descr[i][1]);
      break;
    case 2:
      fprintf(stderr, " [%s]", longops_descr[i][1]);
      break;
    }

	has_reqalt = has_optalt = 0;

	for(j=0; shortopts[j].c; j++) {
		if (shortopts[j].occ[i] == 1) {
			fprintf(stderr, " -%c", shortopts[j].c);
			if (shortopts[j].key)
				fprintf(stderr, " <%s>", shortopts[j].key);
		}
		else if (shortopts[j].occ[i] == 3)
			has_reqalt = 1;
		else if (shortopts[j].occ[i] == 4)
			has_optalt = 1;

	}

	if (has_reqalt || has_optalt) {
		fprintf(stderr, " {");
		for(j=0; shortopts[j].c; j++) {
			if (shortopts[j].occ[i] == 3) {
				fprintf(stderr, " -%c", shortopts[j].c);
				if (shortopts[j].key)
					fprintf(stderr, " <%s>", shortopts[j].key);
			} else if (shortopts[j].occ[i] == 4) {
				fprintf(stderr, " [-%c", shortopts[j].c);
				if (shortopts[j].key)
					fprintf(stderr, " <%s>", shortopts[j].key);
				fprintf(stderr, "]");
			}
		}
		for(j=0; shortopts[j].c; j++) {
			if (shortopts[j].occ[i] == 5) {
				if (has_reqalt) {
					fprintf(stderr, " | -%c", shortopts[j].c);
					if (shortopts[j].key)
						fprintf(stderr, " <%s>", shortopts[j].key);
				} else {
					fprintf(stderr, " | [-%c", shortopts[j].c);
					if (shortopts[j].key)
						fprintf(stderr, " <%s>", shortopts[j].key);
					fprintf(stderr, "]");
				}
				break;
			}
		}
		fprintf(stderr, " }");
	}

	for(j=0; shortopts[j].c; j++) {
		if (shortopts[j].occ[i] == 2) {
			fprintf(stderr, " [-%c", shortopts[j].c);
			if (shortopts[j].key)
				fprintf(stderr, " <%s>", shortopts[j].key);
			fprintf(stderr, "]");
		}
	}

    fprintf(stderr, "\n");

    fprintf(stderr, "   %s\n\n", longops_descr[i][0]);
  }
    
  fprintf(stderr, " arguments:\n");
  for(j=0; j<NUM_AC; j++) {
    for(i=0; shortopts[i].c; i++) {
      if (shortopts[i].ac & argclasses[j].mask) {
	fprintf(stderr, "  -%c", shortopts[i].c);
	
	if (shortopts[i].key)
	  fprintf(stderr, " <%s>", shortopts[i].key);
	else
	  fprintf(stderr, "\t");
      
	fprintf(stderr, "\t%s%s\n", argclasses[j].prefix, shortopts[i].descr);
      }
    }
    fprintf(stderr, "\n");
  }

  show_keys(0);
  fprintf(stderr, "\n");
}

static int parse_mac (char *mac, bnfos_dev_t *dev)
{
	if (strlen(mac) != 17) {
		fprintf(stderr, "Invalid mac address format, expecting XX:XX:XX:XX:XX:XX notation!\n");
		return 1;
	}

	int inv = 0;
	char *p = mac;
	int j;
	for(j=0;(j<6) && (!inv);j++) {
		char *q;
		dev->mac[j] = strtol(p, &q, 16);

		if (q-p != 2)
			inv = 1;
		else {
			p = q;
			if (*q)
				p++;
		}
	}

	if (inv) {
		fprintf(stderr, "Invalid mac address format at '%s', expecting XX:XX:XX:XX:XX:XX notation!\n", p);
		return 1;
	}
	return 0;
}

static int parse_host (char *hostname, bnfos_dev_t *dev)
{
	if (inet_aton(hostname, &dev->address) == -1) {
		struct hostent *host = gethostbyname(hostname);
		if (host == NULL) {
			fprintf(stderr, "Could not resolve hostname!\n");
			return 1;
		}

		if (inet_aton(host->h_addr, &dev->address) == -1) {
			fprintf(stderr, "Invalid host address!\n");
			return 1;
		}
	}
	return 0;
}

static int altarg_given (int mode)
{
	int i = 0;

	for(; shortopts[i].c; ++i) {
		if (shortopts[i].occ[mode] == 5)
			return shortopts[i].val != NULL;
	}

	return 0;
}

/* here the magic begins */
int main(int argc, char **argv) {
  int i = 0;
  bnfos_mode_e mode = MODE_NONE;
  char *mode_arg;

  bnfos_log_sethandler(logmsg);

  bnfos_init();
  
  /* scan command line args, croak on invalid args */
  int c;
  do {
    int longoptin = 0;
    
    c = getopt_long(argc, argv, SHORTOPTS, longopts, &longoptin);
    
    switch(c) {
    case -1:
      break;
      
    case 0:
      if (mode != MODE_NONE) {
	fprintf(stderr, "Unexpected '--%s' argument!\n", longopts[longoptin].name);
	return 1;
      }
      mode = longoptin;
      mode_arg = optarg;
      break;
      
    case '?':
      show_usage();
      return 1;

    default:
      if (mode == MODE_NONE) {
	fprintf(stderr, "Invalid first argument, expecting an operation mode!\n");
	return 1;
      }
      
      for (i=0; shortopts[i].c; i++) {
	if (shortopts[i].c == c) {
	  if (shortopts[i].val) {					
	    fprintf(stderr, "Argument '-%c' only allowed once!\n", c);	
	    return 1;							
	  }
	  
	  if (shortopts[i].occ[mode] == 0) {				
	    fprintf(stderr, "No argument '-%c' allowed in this mode!\n", c); 
	    return 1;							
	  }
	  if (optarg)
	    shortopts[i].val = optarg;
	  else
	    shortopts[i].val = (void *)-1;
	  
	  break;
	}
      }
      
      if (!shortopts[i].c) {
	fprintf(stderr, "Unhandled argument '-%c'!\n", c);
	return 1;
      }
      break;
    }
  } while (c != -1);
  
  /* there must be a mode given */
  if (mode == MODE_NONE) {
    show_usage();
    return 1;
  }
  
  /* request all mandatory args for the mode to be given */
  int missing = 0;
  for(i=0; shortopts[i].c; i++) {
    if (((shortopts[i].occ[mode] == 1) && (!shortopts[i].val)) ||
		((shortopts[i].occ[mode] == 3) && (!shortopts[i].val && !altarg_given(mode)))) {
      if (!missing) {
	fprintf(stderr, "Mandatory arguments missing:\n\n");
	missing = 1;
      }

      fprintf(stderr, "  -%c", shortopts[i].c);

      if (shortopts[i].key)
	fprintf(stderr, " <%s>", shortopts[i].key);
      else
	fprintf(stderr, "\t");

      fprintf(stderr, "\t%s\n", shortopts[i].descr);
    }
  }
  if (missing) {
    int opt = 0;
    int j;

    for(j=0; j<NUM_AC; j++) {
      for(i=0; shortopts[i].c; i++) {
	if (shortopts[i].ac & argclasses[j].mask) {
	  if (((shortopts[i].occ[mode] == 2) || 
		   (shortopts[i].occ[mode] == 4) ||  
		   (shortopts[i].occ[mode] == 5)) && (!shortopts[i].val)) {
	    if (!opt) {
	      fprintf(stderr, "\nOptional arguments:\n\n");
	    }
	    opt = 1+j;
	    
	    fprintf(stderr, "  -%c", shortopts[i].c);

	    if (shortopts[i].key)
	      fprintf(stderr, " <%s>", shortopts[i].key);
	    else
	      fprintf(stderr, "\t");
	    
	    fprintf(stderr, "\t%s%s\n", argclasses[j].prefix, shortopts[i].descr);
	  }
	}
      }
      if (opt == 1+j)
	fprintf(stderr, "\n");
    }
    return 1;
  }
  
  /* setup some usefull defaults */
  bnfos_dev_t dev;
  bnfos_dev_init(&dev);
  struct in_addr address= { htonl(INADDR_ANY) };
  struct in_addr netmask= { htonl(IN_CLASSC_NET) };
  struct in_addr gateway= { htonl(INADDR_ANY) };
  int phy_conf = BNFOS_PHY_PROBE_AUTO;
  int options = BNFOS_OPT_DHCP;
  int loop = 0;
  int detach = 0;
  int iwt = 2;
  int has_netmask = 0;
  int write_config = 0;
  char *load_config = 0;

  /* parse args and check for validity */
  for(i=0; shortopts[i].c; i++) {
    if ((shortopts[i].occ[mode] > 0) && (shortopts[i].val)) {
      switch(shortopts[i].c) {
      case 'm':
      if (parse_mac(shortopts[i].val, &dev))
		  return 1;
	break;
      case 'h':
	  if (parse_host(shortopts[i].val, &dev))
		  return 1;
	break;
      case 'i':
	if (inet_aton(shortopts[i].val, &address) == -1) {
	  fprintf(stderr, "Invalid ip address!\n");
	  return 1;
	}
	break;
      case 'n':
	if (inet_aton(shortopts[i].val, &netmask) == -1) {
	  fprintf(stderr, "Invalid netmask address!\n");
	  return 1;
	}
	has_netmask = 1;
	break;
      case 'g':
	if (inet_aton(shortopts[i].val, &gateway) == -1) {
	  fprintf(stderr, "Invalid gateway address!\n");
	  return 1;
	}
	break;
      case 'e':
	switch(atoi(shortopts[i].val)) {
	case 0:
	  phy_conf = BNFOS_PHY_PROBE_AUTO;
	  break;
	case 1:
	  phy_conf = BNFOS_PHY_PROBE_10MBIT;
	  break;
	case 2:
	  phy_conf = BNFOS_PHY_PROBE_100MBIT;
	  break;
	case 3:
	  phy_conf = BNFOS_PHY_PROBE_10MBIT | BNFOS_PHY_PROBE_100MBIT;
	  break;
	default:
	  fprintf(stderr, "PHY configuration has invalid value!\n");
	  return 1;
	}
	break;
      case 'd':
	{
		int h = atoi(shortopts[i].val);
		if (h != 0 && h != 1) {
			fprintf(stderr, "DHCP status has invalid value!\n");
			return 1;
		}
		if (h)
			options |= BNFOS_OPT_DHCP;
		else
			options &= ~BNFOS_OPT_DHCP;
	}
	break;
      case 'a':
	{
		int h = atoi(shortopts[i].val);
		if (h != 0 && h != 1) {
			fprintf(stderr, "HTTP_AUTH status has invalid value!\n");
			return 1;
		}
		if (h)
			options |= BNFOS_OPT_HTTP_AUTH;
		else
			options &= ~BNFOS_OPT_HTTP_AUTH;
	}
	break;
      case 'p':
	{
	  int h = atoi(shortopts[i].val);
	  if ((h<1) || (h>0xffff)) {
	    fprintf(stderr, "Port has invalid value!\n");
	    return 1;
	  }
	  dev.port = h;
	}
	break;
      case 'u':
	dev.usrpwd = shortopts[i].val;
	break;
      case 'c':
	loop = 1;
	break;
      case 'b':
	detach = 1;
	logmsg = syslog_logfunc;
	break;
      case 'w':
	{
	  int h = atoi(shortopts[i].val);
	  if (h<0) {
	    fprintf(stderr, "Wait time has invalid value!\n");
	    return 1;
	  }
	  iwt = h;
	}
	break;
      case 't':
	{
	  int h = atoi(shortopts[i].val);
	  if (h<0) {
	    fprintf(stderr, "Timeout has invalid value!\n");
	    return 1;
	  }
	  bnfos_set_timeout(NULL, h);
	  bnfos_set_timeout(&dev, h);
	}
      case 's':
	logmsg = syslog_logfunc;
	break;
      case 'x':
	write_config = 1;
	break;
      case 'f':
	load_config = shortopts[i].val;
	break;
      default:
	fprintf(stderr, "Oops, unhandled argument '-%c'...\n", shortopts[i].c);
	break;
      }
    }
  }

  if (load_config) {
	  logmsg(LOG_NOTICE, "Loading config for: %s\n", load_config);
	  FILE *cfg = fopen(CONFFILE, "r");
	  int in_config = 0;
	  int leave = 0;
	  int read_mac = 0;
	  int read_host = 0;
	  char line[256];
	  char *p;
	  if (!cfg) {
		  fprintf(stderr, "Could not open " CONFFILE ": %s\n", strerror(errno));
		  return 1;
	  }
	  while (fgets(line, sizeof(line), cfg)) {
		  switch (line[0]) {
		  case '\t':
		  case '\n':
		  case 0:
		  case '#':
		  case ';':
			  break;
		  case '[':
			  if (in_config)
				  leave = 1;
			  else {
				  p = strchr(line, ']');
				  if (p) {
					  *p = 0;
					  if (!strcasecmp(load_config, line + 1))
						  in_config = 1;
				  } else {
					  fprintf(stderr, "Failed to parse line: %s\n", line);
					  return 1;
				  }
			  }
			  break;
		  case 'm':
			  if (in_config) {
				  char *p = line;
				  if (!strncasecmp("mac", p, 3)) {
					  p += 3;
					  while (*p && (*p == ' ' || *p == '\t' || *p == '='))
						  ++p;
					  if (*p && strlen(p) >= 17) {
						  p[17] = 0;
						  if (parse_mac(p, &dev))
							  return 1;
					  } else {
						  fprintf(stderr, "Could not parse mac address in line: %s\n", line);
						  return 1;
					  }
				  } else {
					  fprintf(stderr, "Failed to parse line: %s\n", line);
					  return 1;
				  }
				  read_mac = 1;
			  }
			  break;
		  case 'h':
			  if (in_config) {
				  char *p = line;
				  if (!strncasecmp("host", p, 4)) {
					  p += 4;
					  while (*p && (*p == ' ' || *p == '\t' || *p == '='))
						  ++p;
					  if (*p) {
						  if (parse_host(p, &dev))
							  return 1;
					  } else {
						  fprintf(stderr, "Could not parse host in line: %s\n", line);
						  return 1;
					  }
				  } else {
					  fprintf(stderr, "Failed to parse line: %s\n", line);
					  return 1;
				  }
				  read_host = 1;
			  }
			  break;
		  case 'l':
			  if (in_config) {
				  char *p = line;
				  char *end;
				  if (!strncasecmp("login", p, 5)) {
					  p += 5;
					  while (*p && (*p == ' ' || *p == '\t' || *p == '='))
						  ++p;
					  if (*p) {
						  end = &p[strlen(p) - 1];
						  while (*end && (*end == ' ' || *end == '\t' || *end == '\n')) {
							  *end = 0;
							  --end;
						  }
						  if (*p)
							  dev.usrpwd = p;
						  else {
							  fprintf(stderr, "Could not parse login information.\n");
							  return 1;
						  }
					  } else {
						  fprintf(stderr, "Could not parse host in line: %s\n", line);
						  return 1;
					  }
				  } else {
					  fprintf(stderr, "Failed to parse line: %s\n", line);
					  return 1;
				  }
				  read_host = 1;
			  }
			  break;
		  default:
			  fprintf(stderr, "Syntax error in " CONFFILE " at line: %s\n", line);
			  return 1;
		  }
		  if (leave)
			  break;
	  }
	  fclose(cfg);

	  if (!read_mac && !read_host) {
		  fprintf(stderr, "No configuration for device %s found!\n", load_config);
		  return 1;
	  }
  }

  /* if netmask was not given, set it to some good default */
  if (!has_netmask) {
    if (IN_CLASSA(ntohl(address.s_addr)))
      netmask.s_addr = htonl(IN_CLASSA_NET);
    else if (IN_CLASSB(ntohl(address.s_addr)))
      netmask.s_addr = htonl(IN_CLASSB_NET);
  }

  /* we are requested to fork to background */
  if (detach) {
    pid_t pid = fork();
    
    if (pid == -1) {
      /* error */
      logmsg(LOG_ERR, "Could not fork, exiting!\n");
      return 1;
    }
    else if (pid) {
      /* parent */
      return 0;
    }
    else {
      /* child */

      fclose(stdin);
      fclose(stdout);
      fclose(stderr);

      /* detach from terminal */
      setpgrp();
    }
  }

  /* the real work starts here, depending on the mode */
  int ret;
  do {
    switch(mode) {
    case MODE_SCAN:
      {
	struct scanarg arg = { 0, 0 };

	if (write_config) {
		arg.cfg = fopen(CONFFILE, "w");
		if (arg.cfg)
			logmsg(LOG_NOTICE, "Writing results to: " CONFFILE "\n");
		else
			logmsg(LOG_WARNING, "Failed to open " CONFFILE ": %s\n", strerror(errno));
	}

	if (bnfos_scan(inet_ntoa(dev.address), scanfunc, &arg) == BNFOS_RET_OK) {
	  logmsg(LOG_NOTICE, "\n%d device%s found.\n\n", arg.cnt, arg.cnt == 1 ? "" : "s");
	  ret = 0;
	}
	else {
	  logmsg(LOG_WARNING, "Network scanning failed!\n");
	  ret = 1;
	}
	break;
      }

    case MODE_NETCONF:
      if (bnfos_netconf(&dev, address, netmask, gateway, phy_conf, options, dev.port) == BNFOS_RET_OK) {
	logmsg(LOG_NOTICE, "Setting network configuration succeeded!\n");
	return 0;
      }
      else {
	logmsg(LOG_WARNING, "Setting network configuration failed!\n");
	return 1;
      }

    case MODE_PING:
      if (bnfos_ping(&dev) == BNFOS_RET_OK) {
	logmsg(LOG_NOTICE, "Ping succeeded!\n");
	ret = 0;
      }
      else {
	logmsg(LOG_WARNING, "Ping failed!\n");
	ret = 1;
      }
      break;

    case MODE_KICK:
      if (bnfos_kick_wdog(&dev) == BNFOS_RET_OK) {
	logmsg(LOG_NOTICE, "Watchdog successfully kicked!\n");
	ret = 0;
      }
      else {
	logmsg(LOG_WARNING, "Watchdog did not response!\n");
	ret = 1;
      }
      break;

    case MODE_RESET:
      if (bnfos_reset(&dev) != BNFOS_RET_OK) {
	logmsg(LOG_WARNING, "Reset failed!\n");
	return 1;
      }
      logmsg(LOG_NOTICE, "Reset succeeded!\n");
      
      return 0;

      break;

    case MODE_FLASH:
      {
	int fd = open(mode_arg, O_RDONLY);
	
	if (fd == -1) {
	  logmsg(LOG_WARNING, "Could not open firmware file: %s\n", strerror(errno));
	  return 1;
	}

	int flashmode = -1;

	if (bnfos_scan(inet_ntoa(dev.address), modecheckfunc, &flashmode) != BNFOS_RET_OK || flashmode == -1) {
	  logmsg(LOG_WARNING, "Device not found!\n");
	  return 1;
	}
	if (!flashmode) {
	  logmsg(LOG_WARNING, "Device not in flashmode!\n");
	  return 1;
	}
	if (bnfos_flash(&dev, fd) != BNFOS_RET_OK) {
	  close(fd);
	  logmsg(LOG_WARNING, "Flashing failed!\n");
	  return 1;
	}
	close(fd);
	logmsg(LOG_NOTICE, "Flashing succeeded!\n");

	return 0;
      }

    case MODE_SET:
      {
	char *val = strchr(mode_arg, '=');
	if (val) {
	  *val=0;
	  val++;
	}

	for(i=0; i<BNFOS_MAX_KEYS; i++) {
	  if (!strcasecmp(keys[i].keyword,mode_arg))
	    break;
	}
	if (i == BNFOS_MAX_KEYS) {
	  show_keys(1);
	  return 1;
	}
	
	if (bnfos_confmap[i].parm && !val) {
	  show_keys(2);
	  return 1;
	}

	
	bnfos_set_t set;
	char *err;
	if (bnfos_key_set_prep(&set, i, val, &err) == BNFOS_RET_ERR) {
	  logmsg(LOG_WARNING, "Setting %s failed: %s\n", mode_arg, err);
	  return 1;
	}
	if (bnfos_key_set_do(&dev, &set) != BNFOS_RET_OK) {
	  logmsg(LOG_WARNING, "Setting %s failed!\n", mode_arg);

	  return 1;
	}
	logmsg(LOG_NOTICE, "Setting %s succeeded!\n", mode_arg);

	return 0;
      }
    case MODE_GET:
    case MODE_SHOW:
      {
	char **keyvals;

	if (mode == MODE_GET) {
	  for(i=0; i<BNFOS_MAX_KEYS; i++) {
	    if (!strcasecmp(keys[i].keyword,mode_arg))
	      break;
	  }

	  if ((i == BNFOS_MAX_KEYS) || (!bnfos_confmap[i].macro)) {
	    show_keys(3);
	    return 1;
	  }
	}

	if (bnfos_key_dump(&dev, &keyvals) != BNFOS_RET_OK)
	  logmsg(LOG_WARNING, "Retrieving configuration and status failed!\n");
	else {
	  if (mode == MODE_GET) {
	    logmsg(LOG_NOTICE, "%s = %s\n", keys[i].keyword, keyvals[i]);
	  }
	  else {
	    logmsg(LOG_NOTICE, "Configuration of device %s:\n", inet_ntoa(dev.address));
	    for(i=0; i<BNFOS_MAX_KEYS; i++) {
	      if (!bnfos_confmap[i].macro)
		continue;
	      
	      char key[0x100];
	      sprintf(key, keys[i].keyword);
	      while(strlen(key)<10)
		strcat(key, " ");
	      
	      logmsg(LOG_NOTICE, " %s= %s\n", key, keyvals[i]);
	    }
	    logmsg(LOG_NOTICE, "\n");
	  }
	}
      }
      break;

    default:
      logmsg(LOG_ERR, "Unhandled mode %d!\n", mode);
      return 1;
    }

    if (loop)
      sleep(iwt);
  } while(loop);
  
  return ret;
}
