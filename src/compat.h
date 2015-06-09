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

#ifndef _BNFOS_COMPAT_H
#define _BNFOS_COMPAT_H 1

#ifdef __linux__

#define cpu_to_be16(x) __cpu_to_be16(x)
#define be16_to_cpu(x) __be16_to_cpu(x)

#else

#define cpu_to_be16(x) htons(x)
#define be16_to_cpu(x) ntohs(x)

#endif /* __linux__ */

#endif /* compat.h */
