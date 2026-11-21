#include "app_main.h"

settings_t device_settings;
static aps_binding_entry_t binding;
static aps_group_tbl_ent_t group = {42, 1};
static repeat_cmd_t retry;
static uint8_t sent[32], sent_count, last_direction;
static bool expect_no_onoff;

void aps_group_list_get(uint8_t *count, uint16_t *ids) { *count = 1; ids[0] = 42; }
aps_group_tbl_ent_t *aps_group_search(uint16_t id, uint8_t ep) { return &group; }
aps_binding_entry_t *bindTblEntryGet(void) { return &binding; }

static void check_options(uint8_t present, uint8_t mask, uint8_t override) {
    assert(present == expect_no_onoff);
    if (expect_no_onoff) {
        assert(mask == ZCL_LEVEL_OPTIONS_EXECUTE_IF_OFF);
        assert(override == 0);
    }
}
#define RECORD(payload) do { \
    check_options(payload->optPresent, payload->optionsMask, payload->optionsOverride); \
    assert(sent_count < sizeof(sent)); sent[sent_count++] = cmd; \
} while (0)
status_t zcl_level_move2level(uint8_t ep, epInfo_t *dst, uint8_t rsp, uint8_t seq, uint8_t cmd, moveToLvl_t *p) {
    RECORD(p); last_direction = p->level == 200 ? LEVEL_MOVE_UP : LEVEL_MOVE_DOWN; return 0;
}
status_t zcl_level_move(uint8_t ep, epInfo_t *dst, uint8_t rsp, uint8_t seq, uint8_t cmd, move_t *p) {
    RECORD(p); last_direction = p->moveMode; return 0;
}
status_t zcl_level_step(uint8_t ep, epInfo_t *dst, uint8_t rsp, uint8_t seq, uint8_t cmd, step_t *p) {
    RECORD(p); last_direction = p->stepMode; return 0;
}
status_t zcl_level_stop(uint8_t ep, epInfo_t *dst, uint8_t rsp, uint8_t seq, uint8_t cmd, stop_t *p) {
    RECORD(p); return 0;
}
bool app_add_repeat_cmd(uint16_t cl, uint8_t ep, uint8_t dest_ep, uint8_t mode, tl_zb_addr_t addr, uint8_t cmd, void *p) {
    retry.srcEp = ep; retry.dstEp = dest_ep; retry.dstAddrMode = mode; retry.dstAddr = addr; retry.cmdId = cmd;
    switch (cmd) {
        case ZCL_CMD_LEVEL_MOVE_TO_LEVEL: case ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF:
            retry.move2Level = *(moveToLvl_t *)p; break;
        case ZCL_CMD_LEVEL_MOVE: case ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF:
            retry.level_move = *(move_t *)p; break;
        case ZCL_CMD_LEVEL_STEP: case ZCL_CMD_LEVEL_STEP_WITH_ON_OFF:
            retry.level_step = *(step_t *)p; break;
        case ZCL_CMD_LEVEL_STOP: retry.level_stop = *(stop_t *)p; break;
        default: assert(false);
    }
    return true;
}

/* GPIO/time/network services used by the production button reader. */
#define DEVICE_BUTTON_MAX 6
#define DEBOUNCE_BUTTON 16
#define FR_COUNTER_MAX 5
#define BATTERY_COUNTER 4
#define RESET_DEVICE_COUNTER 10
#define TIMEOUT_TICK_500MS 500
#define TIMEOUT_20SEC 20000
#define TIMEOUT_3SEC 3000
#define ZCL_CMD_ONOFF_OFF 0
#define ZCL_CMD_ONOFF_ON 1
#define ZCL_CMD_ONOFF_TOGGLE 2
/* PRODUCTION_BUTTON_TYPE */
static app_button_t app_button[6];
typedef struct { bool used, isBusy; } ev_timer_event_t;
static ev_timer_event_t *timerButtonFindBindEvt, *timerFactoryResetEvt;
static bool buttonFindBindFlag, factory_reset;
static struct { bool not_sleep, ota; ev_timer_event_t *timerSetPollRateEvt; } g_appCtx;
static uint8_t repeat_cmd_num;
static uint32_t now;
static bool gpio_released;
static unsigned onoff_count;
static uint8_t onoff_cmd;
typedef struct { uint8_t unused; } zcl_levelAttr_t;
static zcl_levelAttr_t levels[6];
static zcl_levelAttr_t *zcl_levelAttrsGet(void) { return levels; }
static bool drv_gpio_read(uint32_t pin) { return gpio_released; }
static bool clock_time_exceed(uint32_t tick, uint32_t timeout) { return now - tick > timeout; }
static uint32_t clock_time(void) { return now; }
static void light_blink_start(int a, int b, int c) {}
static void light_blink_stop(void) {}
static void light_on(void) {}
static void app_findBindStart(uint8_t i) {}
static void button_factory_reset_start(void) {}
static bool zb_isDeviceJoinedNwk(void) { return true; }
static bool zb_isDeviceFactoryNew(void) { return false; }
static int zb_apsChannelMaskGet(void) { return 0; }
static void zb_rejoinReq(int mask, int duration) {}
static struct { int scanDuration; } g_bdbAttrs;
static void app_setPollRate(int timeout, int rate) {}
static void zb_resetDevice(void) {}
static void batteryCb(void *args) {}
static void clearSleepTimer(void) {}
static int factoryResetCb(void *p) { return -1; }
static int clearButtonFindBindFlagCb(void *p) { return -1; }
#define TL_ZB_TIMER_CANCEL(p) (*(p) = NULL)
static ev_timer_event_t *schedule(int (*cb)(void *), void *args, int timeout) { return NULL; }
#define TL_ZB_TIMER_SCHEDULE(cb, args, timeout) schedule(cb, args, timeout)
static void app_cmdOnOff(uint8_t ep, uint8_t cmd) { onoff_count++; onoff_cmd = cmd; }
/* PRODUCTION_BUTTON_READER */

static void setup(uint8_t ep, uint8_t mode, uint8_t address_mode) {
    memset(&binding, 0, sizeof(binding));
    binding.used = 1; binding.srcEp = ep; binding.clusterId = ZCL_CLUSTER_GEN_LEVEL_CONTROL;
    binding.dstAddrMode = address_mode; binding.groupAddr = 43; binding.dstExtAddrInfo.dstEp = 11;
    device_settings.switchType[ep-1] = mode;
    device_settings.levelMin[ep-1] = 10; device_settings.levelMax[ep-1] = 200;
    device_settings.defaultMoveRate[ep-1] = 100;
    expect_no_onoff = mode >= ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE_NO_ONOFF;
    sent_count = 0;
}
static void check_send_and_retry(uint8_t expected) {
    /* One group-membership send and one binding send. */
    assert(sent_count == 2 && sent[0] == expected && sent[1] == expected);
    assert(retry.cmdId == expected);
    assert(app_repeatCmdLevel(&retry) == -1);
    assert(sent_count == 3 && sent[2] == expected);
    sent_count = 0;
}
static void poll(uint8_t i, bool released) {
    gpio_released = released;
    for (int n = 0; n < DEBOUNCE_BUTTON; n++) read_button_level(i);
}
static void click(uint8_t i) { poll(i, false); now += 30; poll(i, true); now += 30; }
static void finish_clicks(uint8_t i) { now += 600; read_button_level(i); }
static void hold(uint8_t i) {
    poll(i, false); now += 600; read_button_level(i);
    poll(i, true); now += 600; read_button_level(i);
}
static void reset_buttons(void) {
    memset(app_button, 0, sizeof(app_button));
    for (int i = 0; i < 6; i++) app_button[i].debounce = 1;
    now += 2000; sent_count = 0; onoff_count = 0; gpio_released = true;
}
int main(void) {
    /* All three new and all three original modes, every endpoint, both routes/directions. */
    const uint8_t modes[] = {3, 4, 5, 10, 11, 12};
    const uint8_t addresses[] = {APS_SHORT_GROUPADDR_NOEP, APS_LONG_DSTADDR_WITHEP};
    for (int ep = 1; ep <= 6; ep++) for (int m = 0; m < 6; m++) for (int a = 0; a < 2; a++) {
        setup(ep, modes[m], addresses[a]);
        assert(app_level_no_onoff(ep) == (m >= 3));
        for (int dir = 0; dir <= 1; dir++) {
            app_move_to_level(ep, dir);
            check_send_and_retry(expect_no_onoff || dir ? ZCL_CMD_LEVEL_MOVE_TO_LEVEL : ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF);
            app_move_level(ep, dir);
            check_send_and_retry(expect_no_onoff ? ZCL_CMD_LEVEL_MOVE : ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF);
            app_step_level(ep, dir);
            check_send_and_retry(expect_no_onoff ? ZCL_CMD_LEVEL_STEP : ZCL_CMD_LEVEL_STEP_WITH_ON_OFF);
        }
        app_stop_level(ep); check_send_and_retry(ZCL_CMD_LEVEL_STOP);
    }
    for (int ep = 1; ep <= 6; ep++) for (int m = 0; m < 6; m++) {
        setup(ep, modes[m], APS_SHORT_GROUPADDR_NOEP); reset_buttons();
        click(ep-1); finish_clicks(ep-1);
        assert(sent_count == 0 && onoff_count == (m < 3 ? 1u : 0u));
        if (m < 3) assert(onoff_cmd == (m == 0 ? ZCL_CMD_ONOFF_TOGGLE : m == 1 ? ZCL_CMD_ONOFF_ON : ZCL_CMD_ONOFF_OFF));
        reset_buttons(); click(ep-1); click(ep-1); finish_clicks(ep-1);
        assert(onoff_count == 0 && sent_count == (m % 3 == 0 ? 0 : 2));
        if (sent_count) assert(sent[0] == (m >= 3 ? ZCL_CMD_LEVEL_STEP : ZCL_CMD_LEVEL_STEP_WITH_ON_OFF));
        reset_buttons(); hold(ep-1);
        assert(onoff_count == 0 && sent_count == 4 && sent[2] == ZCL_CMD_LEVEL_STOP);
        assert(last_direction == (m % 3 == 2 ? LEVEL_MOVE_DOWN : LEVEL_MOVE_UP));
        sent_count = 0; now += 1000; hold(ep-1);
        assert(onoff_count == 0 && sent_count == 4);
        assert(last_direction == (m % 3 == 0 || m % 3 == 2 ? LEVEL_MOVE_DOWN : LEVEL_MOVE_UP));
    }
    puts("PASS: group/binding/retry commands and single/double/hold/release gestures on all six endpoints");
    return 0;
}
