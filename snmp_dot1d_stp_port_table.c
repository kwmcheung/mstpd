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
#include <libnsh/src/nsh.h>

#include "ctl_socket_client.h"
#include "snmp.h"

#define ELEMENT_SIZE(s,e) sizeof(((s*)0)->e)

#define SYSFS_CLASS_NET "/sys/class/net"
#define SYSFS_PATH_MAX  256

/* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1).dot1dBridge(17).dot1dStp(2) */
#define oid_dot1dStpPortTable oid_dot1dStp, 15

#define NUM_TABLE_ENTRIES 11
#define NUM_INDEXES 1

typedef struct table_data_t {
    long          port;
    long          priority;
    long          state;
    long          enable;
    long          path_cost;
    unsigned char designated_root[8];
    long          designated_cost;
    unsigned char designated_bridge[8];
    unsigned char designated_port[2];
    u_long        forward_transitions;
    long          path_cost32;

    struct table_data_t *next;
} table_data_t;

static struct table_data_t *table_head = NULL;

static nsh_table_index_t idx[NUM_INDEXES] = {
    NSH_TABLE_INDEX (ASN_INTEGER, table_data_t, port, 0),
};

nsh_table(table_reg, table_get_first, table_get_next, table_free, table_data_t, table_head, NUM_TABLE_ENTRIES, idx, NUM_INDEXES);

static void table_create_entry(long port,
                               long priority,
                               long state,
                               long enable,
                               long path_cost,
                               unsigned char *designated_root,
                               long designated_cost,
                               unsigned char *designated_bridge,
                               unsigned char *designated_port,
                               u_long forward_transitions,
                               long path_cost32)
{
    table_data_t *entry;

    entry = SNMP_MALLOC_TYPEDEF (table_data_t);
    if (!entry)
        return;

    entry->port                    = port;
    entry->priority                = priority;
    entry->state                   = state;
    entry->enable                  = enable;
    entry->path_cost               = path_cost;
    memcpy(entry->designated_root,   designated_root,   ELEMENT_SIZE(table_data_t, designated_root));
    entry->designated_cost         = designated_cost;
    memcpy(entry->designated_bridge, designated_bridge, ELEMENT_SIZE(table_data_t, designated_bridge));
    memcpy(entry->designated_port,   designated_port,   ELEMENT_SIZE(table_data_t, designated_port));
    entry->forward_transitions     = forward_transitions;
    entry->path_cost32             = path_cost32;

    entry->next = table_head;
    table_head  = entry;
}

static int not_dot_dotdot(const struct dirent *entry)
{
    const char *n = entry->d_name;

    return !('.' == n[0] && (0 == n[1] || ('.' == n[1] && 0 == n[2])));
}

static int get_port_list(const char *br_ifname, struct dirent ***portlist)
{
    char buf[SYSFS_PATH_MAX];

    snprintf(buf, sizeof(buf), SYSFS_CLASS_NET "/%s/brif", br_ifname);
    return scandir(buf, portlist, not_dot_dotdot, versionsort);
}

static int snmp_map_port_state(int state)
{
    /* Map to SNMP port state value */
    switch (state)
    {
        case BR_STATE_DISABLED:   return 1; /* disabled(1)   */
        case BR_STATE_LISTENING:  return 3; /* listening(3)  */
        case BR_STATE_LEARNING:   return 4; /* learning(4)   */
        case BR_STATE_FORWARDING: return 5; /* forwarding(5) */
        case BR_STATE_BLOCKING:   return 2; /* blocking(2)   */
        default:                  return 5; /* forwarding(5) */
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
        unsigned char designated_root[8], designated_bridge[8], designated_port[2];

        if (CTL_get_cist_port_status(br_index, port_index, &ps))
            continue; /* failed to get port state */

        /* designated root */
        snprintf((char*)designated_root, sizeof(designated_root), "%c%c",
                 ps.designated_root.s.priority / 256, ps.designated_root.s.priority % 256);
        memcpy(designated_root + 2, ps.designated_root.s.mac_address, 6);

        /* designated bridge */
        snprintf((char*)designated_bridge, sizeof(designated_bridge), "%c%c",
                 ps.designated_bridge.s.priority / 256, ps.designated_bridge.s.priority % 256);
        memcpy(designated_bridge + 2, ps.designated_bridge.s.mac_address, 6);

        /* designated port */
        designated_port[0] = (__be16_to_cpu(ps.designated_port) >> 8) & 0xff;
        designated_port[1] = (__be16_to_cpu(ps.designated_port))      & 0xff;

        table_create_entry(GET_NUM_FROM_PRIO(ps.port_id),
                           GET_PRIORITY_FROM_IDENTIFIER(ps.port_id),
                           snmp_map_port_state(ps.state),
                           (ps.role != roleDisabled) ? 1 : 2,
                           (ps.external_port_path_cost < 0xffff) ? ps.external_port_path_cost : 0xffff,
                           designated_root,
                           ps.designated_external_cost,
                           designated_bridge,
                           designated_port,
                           ps.num_trans_fwd,
                           ps.external_port_path_cost);
    }

    for(i = 0; i < num; i++)
        free(portlist[i]);
    free(portlist);

    return 0;
}

static int table_handler(netsnmp_mib_handler *handler,
                         netsnmp_handler_registration *reginfo,
                         netsnmp_agent_request_info *reqinfo,
                         netsnmp_request_info *requests)
{
    nsh_table_entry_t table[] = {
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, port,                0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, priority,            0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, state,               0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, enable,              0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, path_cost,           0),
        NSH_TABLE_ENTRY_RO (ASN_OCTET_STR, table_data_t, designated_root,     0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, designated_cost,     0),
        NSH_TABLE_ENTRY_RO (ASN_OCTET_STR, table_data_t, designated_bridge,   0),
        NSH_TABLE_ENTRY_RO (ASN_OCTET_STR, table_data_t, designated_port,     0),
        NSH_TABLE_ENTRY_RO (ASN_COUNTER,   table_data_t, forward_transitions, 0),
        NSH_TABLE_ENTRY_RO (ASN_INTEGER,   table_data_t, path_cost32,         0),
    };

    return nsh_handle_table(reqinfo, requests, table, NUM_TABLE_ENTRIES);
}

void snmp_init_mib_dot1d_stp_port_table(void)
{
    oid table_oid[] = { oid_dot1dStpPortTable };

    nsh_register_table_ro("dot1dStpPortTable",
                          table_oid,
                          OID_LENGTH (table_oid),
                          table_handler,
                          &table_reg,
                          table_load);
}

#endif /* _WITH_SNMP */
