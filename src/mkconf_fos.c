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

#include <beronet/bnfos.h>
#include <beronet/confmap_fos.h>

int main() {
  int i;

  printf("%s\n", BNFOS_CONFMAP_MAGIC);
  for(i=0; i<BNFOS_MAX_KEYS; i++) {
    if (bnfos_confmap[i].macro) {
      printf("%d_%s=$%s$\n", bnfos_confmap[i].cmd, bnfos_confmap[i].key, bnfos_confmap[i].macro);
    }
  }

  return 0;
}
