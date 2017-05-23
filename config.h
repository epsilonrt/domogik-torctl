/**
 * Copyright © 2016-2017 epsilonRT, All rights reserved.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * <http://www.cecill.info>. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 *
 * @file
 * @brief
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif

/* constants ================================================================ */
#define DMG_TORCTL_VENDOR_ID       "domogik"
#define DMG_TORCTL_DEVICE_ID       "torctl"
#define DMG_TORCTL_CONFIG_FILENAME "domogik-torctl.xpl"
#define DMG_TORCTL_INSTANCE_ID     NULL // NULL for auto instance
#define DMG_TORCTL_DEVICE_VERSION  VERSION_SHORT // VERSION_SHORT is automatically defined in version-git.h from git describe
#define DMG_TORCTL_LOG_LEVEL       LOG_INFO
#define DAEMON_MAX_RESTARTS      100
#define GXPL_POLL_RATE_MS        1000

#if defined(BOARD_RASPBERRYPI)
// -----------------------------------------------------------------------------
#include <sysio/rpi.h>
#define DMG_TORCTL_PORTSIZE      2
/* ECS  : broche GPIO23 (broche 16 GPIO), active à l'état haut... */
/* NWARN: broche GPIO8 (broche 24 GPIO), active à l'état bas... */
#define DMG_TORCTL_PORTPINS {{ .num = GPIO23, .act = 1 }, { .num = GPIO8,  .act = 0 }} 
#define DMG_TORCTL_CONFIG_NAME "output"
#define DMG_TORCTL_CONFIG { "ecs:ptec:hc:hp:", "warn:adsp:on:off:" }

#elif defined(BOARD_NANOPI_NEO)
// -----------------------------------------------------------------------------
#include <sysio/nanopi.h>
#define DMG_TORCTL_PORTSIZE      3
/* RL1-3  : broche GPIOA3,A2,A0, active à l'état haut... */
#define DMG_TORCTL_PORTPINS {{ .num = GPIO_A3, .act = 1 },{ .num = GPIO_A2, .act = 1 },{ .num = GPIO_A0, .act = 1 }} 
#define DMG_TORCTL_CONFIG_NAME "output"
#define DMG_TORCTL_CONFIG { "rl1", "rl2", "rl3" }
#else
#error BOARD not supported
#endif

/* default values =========================================================== */
#define DEFAULT_IO_LAYER                  "udp"
#define DEFAULT_CONNECT_TYPE              gxPLConnectViaHub
#define DEFAULT_HEARTBEAT_INTERVAL        300
#define DEFAULT_CONFIG_HEARTBEAT_INTERVAL 60
#define DEFAULT_HUB_DISCOVERY_INTERVAL    3
#define DEFAULT_ALLOC_STR_GROW            256
#define DEFAULT_LINE_BUFSIZE              256
#define DEFAULT_MAX_DEVICE_GROUP          4
#define DEFAULT_MAX_DEVICE_FILTER         4
#define DEFAULT_XBEE_PORT                 "/dev/ttySC0"

// Unix only
#define DEFAULT_CONFIG_HOME_DIRECTORY     ".gxpl"
#define DEFAULT_CONFIG_SYS_DIRECTORY      "/etc/gxpl"

/* build options ============================================================ */
#define CONFIG_DEVICE_CONFIGURABLE    1
#define CONFIG_DEVICE_GROUP           1
#define CONFIG_DEVICE_FILTER          1
// add the "remote-addr" field in hbeat.basic
#define CONFIG_HBEAT_BASIC_EXTENSION  1

/* conditionals options ====================================================== */

/* ========================================================================== */
#ifdef __cplusplus
}
#endif
#endif /* _CONFIG_H_ defined */
