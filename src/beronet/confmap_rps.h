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

#ifndef _BNRPS_CONFMAP_H
#define _BNRPS_CONFMAP_H 1

#include <beronet/bnrps.h>

#define BNRPS_CONFMAP_MKEY  "bnrps_confmap_magic"
#define BNRPS_CONFMAP_MVAL  "0.1"
#define BNRPS_CONFMAP_MAGIC BNRPS_CONFMAP_MKEY"="BNRPS_CONFMAP_MVAL

static const struct {
  char *key;
  char type;
  int cmd;
  char *parm;
  char *macro;
} bnrps_confmap[BNRPS_MAX_KEYS] = {
  { "p1"     , 'b', 1, "p=1&s=%s" , "state(0,0)"},
  { "p2"     , 'b', 1, "p=2&s=%s" , "state(0,1)"},
  { "p3"     , 'b', 1, "p=3&s=%s" , "state(0,2)"},
  { "p4"     , 'b', 1, "p=4&s=%s" , "state(0,3)"},
  { "p5"     , 'b', 1, "p=5&s=%s" , "state(0,4)"},
  { "p6"     , 'b', 1, "p=6&s=%s" , "state(0,5)"},
  { "p7"     , 'b', 1, "p=7&s=%s" , "state(0,6)"},
  { "p8"     , 'b', 1, "p=8&s=%s" , "state(0,7)"},

  { "dn"     , 'h', 4, "a=%s"     , "hostname(1)"},
  { "ip"     , 'a', 4, "ip=%s"    , "netcfg(0)"},
  { "nm"     , 'a', 4, "nm=%s"    , "netcfg(1)"},
  { "gw"     , 'a', 4, "gw=%s"    , "netcfg(2)"},
  { "dhcp"   , 'b', 4, "dhcp=%s"  , "selchk(0,1,0)"},
  { "port"   , 'p', 4, "port=%s"  , "netcfg(3)"},
  { "pwd"    , 'b', 4, "pwd=%s"   , "selchk(1,1,0)"},
  { "apwd"   , 'd', 4, "apwd=%s"  , NULL},
  { "upwd"   , 'd', 4, "upwd=%s"  , NULL},
  
  { "log"    , 'b', 4, "syslog=%s", "selchk(2,1,0)"},
  { "loghost", 'a', 4, "slgip=%s" , "netcfg(4)"},
  { "logport", 'p', 4, "slgpt=%s" , "netcfg(5)"},
};

#endif /* confmap.h */
