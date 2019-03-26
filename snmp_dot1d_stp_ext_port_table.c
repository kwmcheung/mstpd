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

#include <libnsh/src/nsh.h>

#include "ctl_functions.h"
#include "snmp.h"

/* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1).dot1dBridge(17).dot1dStp(2) */
#define oid_dot1dStpExtPortTable oid_dot1dStp, 19

#define NUM_TABLE_ENTRIES 6
#define NUM_INDEXES 1

typedef struct table_data_t {
    long port;
    long protocol_migration;
    long admin_edge_port;
    long oper_edge_port;
    long admin_point_to_point;
    long oper_point_to_point;
    long admin_path_cost;

    struct table_data_t *next;
} table_data_t;

static struct table_data_t *table_head = NULL;

static nsh_table_index_t idx[NUM_INDEXES] = {
    NSH_TABLE_INDEX (ASN_INTEGER, table_data_t, port, 0),
};

nsh_table(dot1dStpExtPortTable,
	  table_get_first,
	  table_get_next,
	  table_free,
	  table_data_t,
	  table_head,
	  NUM_TABLE_ENTRIES,
	  idx, NUM_INDEXES);

static void table_create_entry(long port,
                               long protocol_migration,
                               long admin_edge_port,
                               long oper_edge_port,
                               long admin_point_to_point,
                               long oper_point_to_point,
                               long admin_path_cost)
{
    table_data_t *entry;

    entry = SNMP_MALLOC_TYPEDEF (table_data_t);
    if (!entry)
        return;

    entry->port                 = port;
    entry->protocol_migration   = protocol_migration;
    entry->admin_edge_port      = admin_edge_port;
    entry->oper_edge_port       = oper_edge_port;
    entry->admin_point_to_point = admin_point_to_point;
    entry->oper_point_to_point  = oper_point_to_point;
    entry->admin_path_cost      = admin_path_cost;

    entry->next = table_head;
    table_head  = entry;
}

static int snmp_map_p2p_state(int state)
{
    switch (state)
    {
        case p2pForceTrue:  return 0;   /* forceTrue(0) */
        case p2pForceFalse: return 1;   /* forceFalse(1) */
        case p2pAuto:       return 2;   /* auto(2) */
        default:            return 0;
    }
}

static int table_load (netsnmp_cache *cache, void* vmagic)
{
    CIST_BridgeStatus s;
    char root_port_name[IFNAMSIZ];
    int br_index = if_nametoindex(BR_NAME);
    struct dirent **portlist;
    int i, num;

    if (!br_index)
        return SNMP_ERR_GENERR;

    if (CTL_get_cist_bridge_status(br_index, &s, root_port_name))
        return SNMP_ERR_GENERR;

    num = get_port_list(BR_NAME, &portlist);
    if (num <= 0)
	    return SNMP_ERR_GENERR;

    for (i = 0; i < num; i++)
    {
        CIST_PortStatus ps;
        int port_index = if_nametoindex(portlist[i]->d_name);

	if (CTL_get_cist_port_status(br_index, port_index, &ps))
            continue; /* failed to get port state */

        table_create_entry(GET_NUM_FROM_PRIO(ps.port_id),
                           2,   /* always returns false(2) when read */
                           ps.admin_edge_port ? 1 : 2,
                           ps.oper_edge_port  ? 1 : 2,
                           snmp_map_p2p_state(ps.admin_p2p),
                           ps.oper_p2p ? 1 : 2,
                           ps.admin_external_port_path_cost);
    }

    return 0;
}

static int table_handler(netsnmp_mib_handler *handler,
                         netsnmp_handler_registration *reginfo,
                         netsnmp_agent_request_info *reqinfo,
                         netsnmp_request_info *requests)
{
    nsh_table_entry_t table[] = {
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, protocol_migration,   0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, admin_edge_port,      0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, oper_edge_port,       0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, admin_point_to_point, 0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, oper_point_to_point,  0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER, table_data_t, admin_path_cost,      0),
    };

    return nsh_handle_table(reqinfo, requests, table, NUM_TABLE_ENTRIES);
}

void snmp_init_mib_dot1d_stp_ext_port_table(void)
{
    oid table_oid[] = { oid_dot1dStpExtPortTable };

    nsh_register_table_ro("dot1dStpExtPortTable",
                          table_oid,
                          OID_LENGTH (table_oid),
                          table_handler,
                          &dot1dStpExtPortTable,
                          table_load);
}

#endif /* _WITH_SNMP */
