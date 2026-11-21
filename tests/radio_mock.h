#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "zcl_custom_attr.h"
#include "app_button.h"
typedef uint8_t u8;
/* Telink defines bool as u8; HOLD_FIX intentionally stores the value 2. */
#undef bool
#define bool u8
typedef uint16_t u16;
typedef uint8_t status_t;
typedef uint8_t extAddr_t[8];
typedef union { uint16_t shortAddr; extAddr_t extAddr; } tl_zb_addr_t;
typedef struct { uint16_t profileId; uint8_t dstAddrMode, txOptions, dstEp; tl_zb_addr_t dstAddr; } epInfo_t;
typedef struct { uint16_t group_addr; uint8_t n_endpoints; } aps_group_tbl_ent_t;
typedef struct {
    uint8_t used, srcEp, dstAddrMode;
    uint16_t clusterId, groupAddr;
    struct { uint8_t dstEp; extAddr_t extAddr; } dstExtAddrInfo;
} aps_binding_entry_t;
typedef struct { uint8_t switchType[6], levelMin[6], levelMax[6], defaultMoveRate[6]; } settings_t;
extern settings_t device_settings;
#define TRUE 1
#define FALSE 0
#define HA_PROFILE_ID 0x104
#define APS_GROUP_TABLE_NUM 1
#define APS_BINDING_TABLE_NUM 1
#define APS_SHORT_GROUPADDR_NOEP 1
#define APS_LONG_DSTADDR_WITHEP 3
#define APS_TX_OPT_ACK_TX 1
#define ZCL_CLUSTER_GEN_LEVEL_CONTROL 8
#define ZCL_SEQ_NUM 0
#define APP_DEBUG(...) ((void)0)
#define TL_SETSTRUCTCONTENT(obj, value) memset(&(obj), (value), sizeof(obj))
void aps_group_list_get(uint8_t *, uint16_t *);
aps_group_tbl_ent_t *aps_group_search(uint16_t, uint8_t);
aps_binding_entry_t *bindTblEntryGet(void);
