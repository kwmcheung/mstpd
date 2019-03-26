/*****************************************************************************
  Copyright (c) 2018 Jonas Johansson <jonasj76@gmail.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Author(s): Jonas Johansson <jonasj76@gmail.com>

******************************************************************************/

#ifndef _MSTPD_SNMP_H
#define _MSTPD_SNMP_H

#include <dirent.h>

/* bridge interface name used in SNMP requests */
#define BR_NAME "br0"

/* standard nodes */
#define oid_iso         1                  /* iso(1) */
#define oid_org         oid_iso, 3         /* iso(1).org(3) */
#define oid_dod         oid_org, 6         /* iso(1).org(3).dod(6) */
#define oid_internet    oid_dod, 1         /* iso(1).org(3).dod(6).internet(1) */
#define oid_mgmt        oid_internet, 2    /* iso(1).org(3).dod(6).internet(1).mgmt(2) */
#define oid_mib2        oid_mgmt, 1        /* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1) */
#define oid_dot1dBridge oid_mib2, 17       /* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1).dot1dBridge(17) */
#define oid_dot1dStp    oid_dot1dBridge, 2 /* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1).dot1dBridge(17).dot1dStp(2) */

void snmp_init(void);
void snmp_fini(void);

int get_port_list(const char *br_ifname, struct dirent ***portlist);

#endif /* _MSTPD_SNMP_H */
