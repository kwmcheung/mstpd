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

#if _WITH_SNMP

#include <dirent.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "snmp_dot1d_stp.h"
#include "snmp_dot1d_stp_port_table.h"
#include "snmp_dot1d_stp_ext_port_table.h"

#define SYSFS_CLASS_NET "/sys/class/net"
#define SYSFS_PATH_MAX  256

static int not_dot_dotdot(const struct dirent *entry)
{
    const char *n = entry->d_name;

    return !('.' == n[0] && (0 == n[1] || ('.' == n[1] && 0 == n[2])));
}

int get_port_list(const char *br_ifname, struct dirent ***portlist)
{
    char buf[SYSFS_PATH_MAX];

    snprintf(buf, sizeof(buf), SYSFS_CLASS_NET "/%s/brif", br_ifname);
    return scandir(buf, portlist, not_dot_dotdot, versionsort);
}

void snmp_init(void)
{
    netsnmp_enable_subagent();
    snmp_disable_log();
    snmp_enable_stderrlog();
    init_agent("mstpdAgent");

    snmp_init_mib_dot1d_stp();
    snmp_init_mib_dot1d_stp_port_table();
    snmp_init_mib_dot1d_stp_ext_port_table();

    init_snmp("mstpdAgent");
}

void snmp_fini(void)
{
    snmp_shutdown("mstpdAgent");
}

#endif /* _WITH_SNMP */
