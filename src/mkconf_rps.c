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

#include <beronet/bnrps.h>
#include <beronet/confmap_rps.h>

int main() {
  int i;

  printf("%s\n", BNRPS_CONFMAP_MAGIC);
  for(i=0; i<BNRPS_MAX_KEYS; i++) {
    if (bnrps_confmap[i].macro) {
      printf("%d_%s=$%s$\n", bnrps_confmap[i].cmd, bnrps_confmap[i].key, bnrps_confmap[i].macro);
    }
  }

  return 0;
}
