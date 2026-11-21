#include "app_level.h"
typedef struct {
    uint8_t cmdId, srcEp, dstEp, dstAddrMode;
    tl_zb_addr_t dstAddr;
    moveToLvl_t move2Level;
    move_t level_move;
    step_t level_step;
    stop_t level_stop;
} repeat_cmd_t;
bool app_add_repeat_cmd(uint16_t, uint8_t, uint8_t, uint8_t, tl_zb_addr_t, uint8_t, void *);
status_t zcl_level_move2level(uint8_t, epInfo_t *, uint8_t, uint8_t, uint8_t, moveToLvl_t *);
status_t zcl_level_move(uint8_t, epInfo_t *, uint8_t, uint8_t, uint8_t, move_t *);
status_t zcl_level_step(uint8_t, epInfo_t *, uint8_t, uint8_t, uint8_t, step_t *);
status_t zcl_level_stop(uint8_t, epInfo_t *, uint8_t, uint8_t, uint8_t, stop_t *);
