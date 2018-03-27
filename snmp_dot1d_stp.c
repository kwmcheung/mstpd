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

#include "ctl_socket_client.h"
#include "snmp.h"

#define SNMP_STP_PRIORITY               2
#define SNMP_STP_TIME_SINCE_TOPO_CHANGE 3
#define SNMP_STP_TOP_CHANGES            4
#define SNMP_STP_DESIGNATED_ROOT        5
#define SNMP_STP_ROOT_COST              6
#define SNMP_STP_ROOT_PORT              7
#define SNMP_STP_MAX_AGE                8
#define SNMP_STP_HELLO_TIME             9
#define SNMP_STP_HOLD_TIME              10
#define SNMP_STP_FORWARD_DELAY          11
#define SNMP_STP_BRIDGE_MAX_AGE         12
#define SNMP_STP_BRIDGE_HELLO_TIME      13
#define SNMP_STP_BRIDGE_FORWARD_DELAY   14
#define SNMP_STP_VERSION                16
#define SNMP_STP_TX_HOLD_COUNT          17

/* iso(1).org(3).dod(6).internet(1).mgmt(2).mib-2(1).dot1dBridge(17).dot1dStp(2) */
#define oid_dot1dStpProtocolSpecification   oid_dot1dStp, 1
#define oid_dot1dStpPriority                oid_dot1dStp, SNMP_STP_PRIORITY
#define oid_dot1dStpTimeSinceTopologyChange oid_dot1dStp, SNMP_STP_TIME_SINCE_TOPO_CHANGE
#define oid_dot1dStpTopChanges              oid_dot1dStp, SNMP_STP_TOP_CHANGES
#define oid_dot1dStpDesignatedRoot          oid_dot1dStp, SNMP_STP_DESIGNATED_ROOT
#define oid_dot1dStpRootCost                oid_dot1dStp, SNMP_STP_ROOT_COST
#define oid_dot1dStpRootPort                oid_dot1dStp, SNMP_STP_ROOT_PORT
#define oid_dot1dStpMaxAge                  oid_dot1dStp, SNMP_STP_MAX_AGE
#define oid_dot1dStpHelloTime               oid_dot1dStp, SNMP_STP_HELLO_TIME
#define oid_dot1dStpHoldTime                oid_dot1dStp, SNMP_STP_HOLD_TIME
#define oid_dot1dStpForwardDelay            oid_dot1dStp, SNMP_STP_FORWARD_DELAY
#define oid_dot1dStpBridgeMaxAge            oid_dot1dStp, SNMP_STP_BRIDGE_MAX_AGE
#define oid_dot1dStpBridgeHelloTime         oid_dot1dStp, SNMP_STP_BRIDGE_HELLO_TIME
#define oid_dot1dStpBridgeForwardDelay      oid_dot1dStp, SNMP_STP_BRIDGE_FORWARD_DELAY
#define oid_dot1dStpVersion                 oid_dot1dStp, SNMP_STP_VERSION
#define oid_dot1dStpTxHoldCount             oid_dot1dStp, SNMP_STP_TX_HOLD_COUNT

static int snmp_get_dot1d_stp(void *value, int len, int id)
{
    CIST_BridgeStatus s;
    char root_port_name[IFNAMSIZ];
    int br_index = if_nametoindex(BR_NAME);

    if (!br_index)
        return SNMP_ERR_GENERR;

    if (CTL_get_cist_bridge_status(br_index, &s, root_port_name))
        return SNMP_ERR_GENERR;

    switch (id)
    {
        case SNMP_STP_PRIORITY:
            *(int*)value = __be16_to_cpu(s.bridge_id.s.priority);
	    break;
        case SNMP_STP_TIME_SINCE_TOPO_CHANGE:
            *(int*)value = s.time_since_topology_change;
            break;
        case SNMP_STP_TOP_CHANGES:
            *(int*)value = s.topology_change_count;
            break;
        case SNMP_STP_DESIGNATED_ROOT:
            snprintf((char*)value, len, "%c%c",
            s.designated_root.s.priority / 256,
            s.designated_root.s.priority % 256);
            memcpy(value + 2, s.designated_root.s.mac_address, ETH_ALEN);
            break;
        case SNMP_STP_ROOT_COST:
            *(int*)value = s.root_path_cost;
            break;
        case SNMP_STP_ROOT_PORT:
            *(int*)value = GET_NUM_FROM_PRIO(s.root_port_id);
            break;
        case SNMP_STP_MAX_AGE:
            *(int*)value = s.root_max_age * 100;
            break;
        case SNMP_STP_HELLO_TIME:
            *(int*)value = s.bridge_hello_time * 100;
            break;
        case SNMP_STP_HOLD_TIME:
            *(int*)value = s.tx_hold_count * 100;
            break;
        case SNMP_STP_FORWARD_DELAY:
            *(int*)value = s.root_forward_delay * 100;
            break;
        case SNMP_STP_BRIDGE_MAX_AGE:
            *(int*)value = s.bridge_max_age * 100;
            break;
        case SNMP_STP_BRIDGE_HELLO_TIME:
            *(int*)value = s.bridge_hello_time * 100;
            break;
        case SNMP_STP_BRIDGE_FORWARD_DELAY:
            *(int*)value = s.bridge_forward_delay * 100;
            break;
        case SNMP_STP_VERSION:
            if (!(protoSTP  != s.protocol_version ||
                  protoRSTP != s.protocol_version))
                return SNMP_ERR_GENERR;

            if (protoSTP == s.protocol_version)
                *(int*)value = 0; /* stpCompatible(0) */
            else
                *(int*)value = 2; /* rstp(2) */

            break;
        case SNMP_STP_TX_HOLD_COUNT:
            *(int*)value = s.tx_hold_count;
            break;
        default:
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

nsh_scalar_handler_const    (dot1dStpProtocolSpecification,   ASN_INTEGER,   3); /* ieee8021d(3) */
nsh_scalar_group_handler_ro (dot1dStpPriority,                ASN_INTEGER,   SNMP_STP_PRIORITY,               snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpTimeSinceTopologyChange, ASN_TIMETICKS, SNMP_STP_TIME_SINCE_TOPO_CHANGE, snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpTopChanges,              ASN_COUNTER,   SNMP_STP_TOP_CHANGES,            snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpDesignatedRoot,          ASN_OCTET_STR, SNMP_STP_DESIGNATED_ROOT,        snmp_get_dot1d_stp, 8,                0);
nsh_scalar_group_handler_ro (dot1dStpRootCost,                ASN_INTEGER,   SNMP_STP_ROOT_COST,              snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpRootPort,                ASN_INTEGER,   SNMP_STP_ROOT_PORT,              snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpMaxAge,                  ASN_INTEGER,   SNMP_STP_MAX_AGE,                snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpHelloTime,               ASN_INTEGER,   SNMP_STP_HELLO_TIME,             snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpHoldTime,                ASN_INTEGER,   SNMP_STP_HOLD_TIME,              snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpForwardDelay,            ASN_INTEGER,   SNMP_STP_FORWARD_DELAY,          snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpBridgeMaxAge,            ASN_INTEGER,   SNMP_STP_BRIDGE_MAX_AGE,         snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpBridgeHelloTime,         ASN_INTEGER,   SNMP_STP_BRIDGE_HELLO_TIME,      snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpBridgeForwardDelay,      ASN_INTEGER,   SNMP_STP_BRIDGE_FORWARD_DELAY,   snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpVersion,                 ASN_INTEGER,   SNMP_STP_VERSION,                snmp_get_dot1d_stp, sizeof(uint32_t), 0);
nsh_scalar_group_handler_ro (dot1dStpTxHoldCount,             ASN_INTEGER,   SNMP_STP_TX_HOLD_COUNT,          snmp_get_dot1d_stp, sizeof(uint32_t), 0);

void snmp_init_mib_dot1d_stp(void)
{
    nsh_register_scalar_ro(dot1dStpProtocolSpecification);
    nsh_register_scalar_ro(dot1dStpPriority);
    nsh_register_scalar_ro(dot1dStpTimeSinceTopologyChange);
    nsh_register_scalar_ro(dot1dStpTopChanges);
    nsh_register_scalar_ro(dot1dStpDesignatedRoot);
    nsh_register_scalar_ro(dot1dStpRootCost);
    nsh_register_scalar_ro(dot1dStpRootPort);
    nsh_register_scalar_ro(dot1dStpMaxAge);
    nsh_register_scalar_ro(dot1dStpHelloTime);
    nsh_register_scalar_ro(dot1dStpHoldTime);
    nsh_register_scalar_ro(dot1dStpForwardDelay);
    nsh_register_scalar_ro(dot1dStpBridgeMaxAge);
    nsh_register_scalar_ro(dot1dStpBridgeHelloTime);
    nsh_register_scalar_ro(dot1dStpBridgeForwardDelay);
    nsh_register_scalar_ro(dot1dStpVersion);
    nsh_register_scalar_ro(dot1dStpTxHoldCount);
}

#endif /* _WITH_SNMP */
