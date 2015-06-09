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

#ifndef _BNFOS_CONFMAP_H
#define _BNFOS_CONFMAP_H 1

#include <beronet/bnfos.h>

#define BNFOS_CONFMAP_MKEY  "bnfos_confmap_magic"
#define BNFOS_CONFMAP_MVAL  "0.1"
#define BNFOS_CONFMAP_MAGIC BNFOS_CONFMAP_MKEY"="BNFOS_CONFMAP_MVAL

static const struct {
  char *key;
  char type;
  int cmd;
  char *parm;
  char *macro;
} bnfos_confmap[BNFOS_MAX_KEYS] = {
  { "sz"     , 'b', 1, "sz=%s"    , "szenario(0)"},
  { "mode"   , 'b', 4, "mode=%s"  , "mode(0)"},
  { "rm"     , 'b', 1, "rm=%s"    , "config(1,1)"},

  { "p0"     , 'b', 5, "p=0&s=%s" , "pwrport(0,0)"},
  { "p0"     , 'b', 1, "p0=%s"    , "config(2,1)"},
  { "p1"     , 'b', 5, "p=1&s=%s" , "pwrport(0,1)"},
  { "p1"     , 'b', 1, "p1=%s"    , "config(3,1)"},

  { "dn"     , 'h', 3, "dn=%s"    , "hostname(1)"},
  { "ip"     , 'a', 3, "ip=%s"    , "netconf(0)"},
  { "nm"     , 'a', 3, "nm=%s"    , "netconf(1)"},
  { "gw"     , 'a', 3, "gw=%s"    , "netconf(2)"},
  { "dns"    , 'a', 3, "dns=%s"   , "netconf(3)"},
  { "dhcp"   , 'b', 3, "dhcp=%s"  , "config(4,1)"},
  { "port"   , 'p', 3, "port=%s"  , "netconf(6)"},
  { "pwd"    , 'b', 3, "pwd=%s"   , "config(5,1)"},
  { "apwd"   , 'd', 3, "apwd=%s"  , NULL},
  
  { "mhost"  , 's', 2, "mhost=%s" , "netconf(5)"},
  { "mfrom"  , 's', 2, "mfrom=%s" , "netconf(7)"},
  { "mto"    , 's', 2, "mto=%s"   , "netconf(8)"},
  { "XXXXX"  , 'n', 7, ""         , NULL},

  { "log"    , 'b', 3, "syslog=%s", "config(10,1)"},
  { "loghost", 'a', 3, "slgip=%s" , "netconf(9)"},
  { "logport", 'p', 3, "slgpt=%s" , "netconf(10)"},

  { "wen"    , 'b', 6, "wen=%s"   , "wdog(0)"},
  { "wen"    , 'b', 2, "wen=%s"   , "config(6,1)"},
  { "wstate" ,   0, 6, "wstate=%s", "wdog(0)"},
  { "wintv"  , 'p', 2, "wintv=%s" , "config(8,?)"},
  { "as"     , 'b', 2, "as=%s"    , "config(9,1)"},
  { "men"    , 'b', 2, "men=%s"   , "config(7,1)"},
  { "wretv"  ,   0, 0, NULL       , "wdog(2)"},
};

#endif /* confmap.h */
