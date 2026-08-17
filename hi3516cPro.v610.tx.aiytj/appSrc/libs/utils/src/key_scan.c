/* 
 *       Filename:  key_scan.c
 *    Description:  
 *        Version:  1.0
 *        Created:  05/13/2026 09:03:37 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

/* 架构:
 *   - 统一结构体: 配置 key_cfg_t、状态 key_sta_t 各一个，所有事件共用
 *   - 公共阶段函数: phase_* 封装重复的状态转换逻辑
 *   - 全局映射表: g_handlers[event] → 处理函数，O(1) 分派
 *   - 每个处理函数仅关注该事件的差异逻辑
 */

#include <stdio.h>

#include "utils.h"
#include "key_scan.h"

#define LOG_LVL_DBG(enable, fmt, args...)   if (enable) DBG(fmt, ##args)
#define LOG_LVL_WAR(enable, fmt, args...)   if (enable) WAR(fmt, ##args)
#define LOG_LVL_ERR(enable, fmt, args...)   if (enable) ERR(fmt, ##args)
#define LOG_LVL_PANIC(enable, fmt, args...) if (enable) SYSLOG(fmt, ##args)
#define LOG_LVL_LOOP(enable, fmt, args...)  if (enable) DBG(fmt, ##args)

#define pri_key(lvl, dbg, fmt, args...)     LOG_##lvl(dbg & lvl, fmt, ##args)

const char g_evt_string[KEY_EVENT_MAX][16] = {
    "NONE",
    "PRESS_DOWN",
    "PRESS_UP",
    "PRESSING",
    "PRESS_REPEAT",
    "SHORT_PRESS",
    "LONG_PRESS",
    "LONG_PRESSING",
    "LONG_PRESS_UP",
    "CLICK"
};

/* ═══════════════════════════════════════════════════════════════
 *  处理函数类型 & 前向声明 & 全局映射表
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief handler 函数签名
 *
 * @param  ev     事件类型（CLICK/DBL/TRL/MULTI 共用 handler 时需区分）
 * @param  cfg    用户配置（只读）
 * @param  sta    运行时状态（读写）
 * @param  active 当前是否为有效电平（已由 key_scan 预先读取）
 * @return TRUE = 事件触发
 */
typedef int (*key_handler_fn)(key_event_t ev, const key_cfg_t *cfg,
                              key_sta_t *sta, int active);

/* ═══════════════════════════════════════════════════════════════
 *  内部工具函数
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  读取 GPIO 并判断当前是否处于有效电平
 * @param  c  配置（使用 gpio 和 active_level 字段）
 * @return 1 = 有效电平, 0 = 无效电平或读取失败
 */
static inline int is_key_active(const key_cfg_t *c)
{
    int val;

    /* 读取失败视为无效 */
    if (gpio_open_get_value(c->gpio, &val) != 0) {
        ERR("failed to get gpio %d value\n", c->gpio);
        return FALSE;
    }

    return (val == c->active_level);
}

/* ═══════════════════════════════════════════════════════════════
 *  公共阶段函数
 *
 *  将状态机中多个 handler 共有的阶段逻辑提取为独立函数。
 *  所有函数操作统一的 key_sta_t 结构体，字段名一致，
 *  不同 handler 中的相同阶段只需一行调用即可复用。
 *
 *  阶段编号与状态机流转顺序对应:
 *    ① idle_to_debounce  → ② debounce_check  → ③ enter_pressed
 *    → ④ detect_release  → ⑤ release_debounce
 *    ⑥ hold_exceeds（横切：时间阈值判断）
 * ═══════════════════════════════════════════════════════════════ */

/**
 * @brief  阶段 ① 空闲 → 按下消抖
 *
 * 在 KST_IDLE 状态下调用。检测到有效电平时，
 * 将状态切换为 KST_DEBOUNCE 并启动消抖计时。
 *
 * @param  sta     运行时状态（读写）
 * @param  active  当前是否为有效电平
 * @return TRUE  = 已进入消抖状态（调用者可执行事件专属初始化）
 * @return FALSE = 仍空闲（电平无效）
 */
static inline int phase_idle_to_debounce(key_sta_t *sta, int active)
{
    if (!active) {
        //pri_key(LVL_LOOP, sta->dbg_on, "key not active, keep idle\n");
        return FALSE;
    }

    sta->st = KST_DEBOUNCE;
    ms_clock_reset(&sta->t_debounce);

    pri_key(LVL_DBG, sta->dbg_on, "key pressed from idle to debounce\n");

    return TRUE;
}

/**
 * @brief  阶段 ② 按下消抖判定
 *
 * 在 KST_DEBOUNCE 状态下调用。检查有效电平是否在 debounce_ms
 * 内保持稳定：
 *   - 电平回弹 → 消抖失败，回退至 IDLE
 *   - 持续稳定且时间到 → 消抖通过
 *   - 持续稳定但时间未到 → 继续等待
 *
 * @param  sta          运行时状态（读写）
 * @param  active       当前是否为有效电平
 * @param  debounce_ms  消抖时间阈值
 * @return -1 = 抖动回弹，已回退至 KST_IDLE
 * @return  0 = 仍在消抖等待中
 * @return  1 = 消抖通过，按下已确认
 */
static inline int phase_debounce_check(key_sta_t *sta,
                                       int active, uint16_t debounce_ms)
{
    if (!active) {
        sta->st = KST_IDLE;                 /* 电平回弹，消抖失败 */
        pri_key(LVL_DBG, sta->dbg_on, "key release when debouncing, reset to idle\n");
        return -1;
    }

    if (ms_since_previous(&sta->t_debounce) >= debounce_ms) {
        pri_key(LVL_DBG, sta->dbg_on,
                "key debounce check success, t_debounce >= %d\n", debounce_ms);
        return 1;                           /* 消抖时间到，确认按下 */
    }

    pri_key(LVL_DBG, sta->dbg_on, "key debouncing\n");

    return 0;                               /* 仍在等待 */
}

/**
 * @brief  阶段 ③ 进入确认按下状态
 *
 * 消抖通过后调用。将状态切换为 KST_PRESSED，
 * 并记录按下时刻（后续可据此计算按住持续时间）。
 *
 * @param  sta  运行时状态（读写）
 */
static inline void phase_enter_pressed(key_sta_t *sta)
{
    sta->st = KST_PRESSED;
    ms_clock_reset(&sta->t_press);

    pri_key(LVL_DBG, sta->dbg_on, "key from debounce to pressed\n");
}

/**
 * @brief  阶段 ④ 检测松开 → 启动释放消抖
 *
 * 在 KST_PRESSED 状态下调用。检测到无效电平（松开）时，
 * 将状态切换为 KST_DEB_REL 并启动释放消抖计时。
 *
 * @param  sta     运行时状态（读写）
 * @param  active  当前是否为有效电平
 * @return TRUE  = 检测到松开，已进入 KST_DEB_REL
 * @return FALSE = 仍按住
 */
static inline int phase_detect_release(key_sta_t *sta, int active)
{
    if (active) {
        pri_key(LVL_DBG, sta->dbg_on, "key pressed, keep status\n");
        return FALSE;
    }

    sta->st = KST_DEB_REL;
    ms_clock_reset(&sta->t_debounce);

    pri_key(LVL_DBG, sta->dbg_on, "key change to debounce release\n");

    return TRUE;
}

/**
 * @brief  阶段 ⑤ 释放消抖判定
 *
 * 在 KST_DEB_REL 状态下调用。检查无效电平是否在 debounce_ms
 * 内保持稳定：
 *   - 电平回弹（又按下了）→ 恢复 KST_PRESSED
 *   - 持续稳定且时间到 → 释放确认
 *   - 持续稳定但时间未到 → 继续等待
 *
 * @param  sta          运行时状态（读写）
 * @param  active       当前是否为有效电平
 * @param  debounce_ms  消抖时间阈值
 * @return -1 = 抖动回弹，已恢复至 KST_PRESSED
 * @return  0 = 仍在消抖等待中
 * @return  1 = 释放已确认
 */
static inline int phase_release_debounce(key_sta_t *sta,
                                         int active, uint16_t debounce_ms)
{
    if (active) {
        sta->st = KST_PRESSED;              /* 电平回弹，恢复按住 */
        pri_key(LVL_DBG, sta->dbg_on,
                "key pressed when release debouce, turn to pressed\n");
        return -1;
    }

    if (ms_since_previous(&sta->t_debounce) >= debounce_ms) {
        pri_key(LVL_DBG, sta->dbg_on,
                "key release check succ, t_debounce >= %d\n", debounce_ms);
        return 1;                           /* 释放确认 */
    }

    pri_key(LVL_DBG, sta->dbg_on, "key release checking\n");

    return 0;                               /* 仍在等待 */
}

/**
 * @brief  阶段 ⑥ 检查按住持续时间是否超过阈值
 *
 * 在 KST_PRESSED 状态下调用，用于长按/超长按/连发等时间判定。
 * 计算从按下确认（t_press）到当前时刻的间隔。
 *
 * @param  sta          运行时状态（只读）
 * @param  threshold_ms  时间阈值（毫秒）
 * @return TRUE  = 已超过阈值
 * @return FALSE = 尚未达到
 */
static inline int phase_hold_exceeds(key_sta_t *sta, uint16_t threshold_ms)
{
    if (ms_since_previous(&sta->t_press) >= threshold_ms) {
        pri_key(LVL_DBG, sta->dbg_on, "t_press >= %d\n", threshold_ms);
        return TRUE;
    } else {
        return FALSE;
    }
}


/* ═══════════════════════════════════════════════════════════════
 *
 *  各事件处理函数
 *
 *  每个函数仅关注该事件的差异逻辑，
 *  公共阶段（消抖、释放检测等）通过调用 phase_* 函数复用。
 *  操作的字段名在所有 handler 中保持一致：
 *    st / t_debounce / t_press / t_repeat / t_release / was_long / click_count
 *
 * ═══════════════════════════════════════════════════════════════ */


/* ─────────────────────────────────────────────────────────────
 *  cb_press_down — 按下检测
 *
 *  状态流:
 *    IDLE ──[active]──▶ DEBOUNCE ──[稳定 debounce_ms]──▶ FIRED
 *                           │                               │
 *                        [抖动]                         [!active]
 *                           ▼                               ▼
 *                         IDLE                            IDLE
 *
 *  消抖通过后直接进入 FIRED 并返回 TRUE，
 *  等待松开后才回到 IDLE 允许下一次触发。
 * ──────────────────────────────────────────────────────────── */
static int cb_press_down(key_event_t ev, const key_cfg_t *c,
                         key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            pri_key(LVL_DBG, s->dbg_on, "key from debounce to fired\n");
            s->st = KST_FIRED;
            return TRUE;
        }
        break;
    }

    case KST_FIRED: {
        if (!active) {
            pri_key(LVL_DBG, s->dbg_on, "key release when fired, reset to idle\n");
            s->st = KST_IDLE;   /* 松开后允许下次触发 */
        }
        break;
    }
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_press_up — 释放检测
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED ──[!active]──▶ DEB_REL
 *                                                    │
 *                                              [释放确认]
 *                                                    ▼
 *                                              return TRUE
 * ──────────────────────────────────────────────────────────── */
static int cb_press_up(key_event_t ev, const key_cfg_t *c,
                       key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;
    }

    case KST_PRESSED: {
        phase_detect_release(s, active);
        break;
    }

    case KST_DEB_REL: {
        if (phase_release_debounce(s, active, c->debounce_ms) > 0) {
            pri_key(LVL_DBG, s->dbg_on, "key from debounce release to idle\n");
            s->st = KST_IDLE;
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_pressing — 持续电平检测（无状态机）
 *
 *  每次调用直接返回当前电平状态。
 *  注意：返回频率 = 调用频率（约 100Hz），
 *  适用于实时状态指示（如 LED 联动），不适用于触发动作。
 * ──────────────────────────────────────────────────────────── */
static int cb_pressing(key_event_t ev, const key_cfg_t *c,
                       key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);
    return active;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_press_repeat — 按住连发
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED
 *                             │
 *                held >= repeat_start_ms ?
 *                   ├─ 首次到达 → return TRUE, 记录 t_repeat
 *                   └─ 间隔 >= repeat_interval_ms → return TRUE
 *                             │
 *                          [!active] → IDLE
 *
 *  适用于音量/亮度连续调节等需要按住不放持续触发的场景。
 * ──────────────────────────────────────────────────────────── */
static int cb_press_repeat(key_event_t ev, const key_cfg_t *c,
                           key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
            ms_clock_reset(&s->t_repeat);   /* 初始化连发计时 */
        }
        break;
    }

    case KST_PRESSED: {
        if (!active) {
            s->st = KST_IDLE;
            pri_key(LVL_DBG, s->dbg_on, "key release, reset to idle\n");
            break;
        }

        /* 按住时间达到首次延迟 且 连发间隔已到 */
        if (phase_hold_exceeds(s, c->repeat_start_ms) &&
            ms_since_previous(&s->t_repeat) >= c->repeat_interval_ms) {
            pri_key(LVL_DBG, s->dbg_on,
                    "key repeat triggered, t_repeat >= %d\n", c->repeat_interval_ms);
            ms_clock_reset(&s->t_repeat);
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_short_press — 短按（释放时按住时间 < long_press_ms）
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED ──[!active]──▶ DEB_REL
 *                                                    │
 *                                              [释放确认]
 *                                                    │
 *                                    持续时间 < long_press_ms ?
 *                                      ├─ 是 → return TRUE
 *                                      └─ 否 → IDLE（是长按，不触发）
 *
 *  long_press_ms == 0 时无上限，任何短于无限的按放均触发。
 * ──────────────────────────────────────────────────────────── */
static int cb_short_press(key_event_t ev, const key_cfg_t *c,
                          key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;
    }

    case KST_PRESSED: {
        phase_detect_release(s, active);
        break;
    }

    case KST_DEB_REL:
        if (phase_release_debounce(s, active, c->debounce_ms) > 0) {
            s->st = KST_IDLE;
            pri_key(LVL_DBG, s->dbg_on,
                    "key from deb rel to idle, long_press_ms: %d, exceeds: %d\n",
                    c->long_press_ms, phase_hold_exceeds(s, c->long_press_ms));
            /* 未设上限 或 持续时间未达上限 → 是短按 */
            return (c->long_press_ms == 0 ||
                    !phase_hold_exceeds(s, c->long_press_ms));
        }
        break;
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_long_press — 长按（持续时间 >= long_press_ms 时触发一次）
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED
 *                             │
 *                      held >= long_press_ms ?
 *                         ├─ 是 → FIRED (return TRUE)
 *                         └─ 否 → [!active] → IDLE
 *    FIRED ──[!active]──▶ IDLE
 *
 *  触发一次后进入 FIRED 状态等待松开，按住不放不会重复触发。
 *  适用于恢复出厂设置、SOS 紧急操作等需要防误触的场景。
 * ──────────────────────────────────────────────────────────── */
static int cb_long_press(key_event_t ev, const key_cfg_t *c,
                         key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE:
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;

    case KST_PRESSED:
        /* 优先检查阈值，避免松开瞬间丢失触发 */
        if (phase_hold_exceeds(s, c->long_press_ms)) {
            s->st = KST_FIRED;
            pri_key(LVL_DBG, s->dbg_on, "key from pressed to fired\n");
            return TRUE;
        }

        if (!active) {
            pri_key(LVL_DBG, s->dbg_on, "key release before exceeds, reset to idle\n");
            s->st = KST_IDLE;
        }
        break;

    case KST_FIRED:
        if (!active) {
            pri_key(LVL_DBG, s->dbg_on, "key release after exceeds, reset to idle\n");
            s->st = KST_IDLE;
        }
        break;
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_long_pressing — 长按连发
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED
 *                             │
 *                      held >= long_press_ms ?
 *                         ├─ 是 → FIRED (首次 return TRUE, 记录 t_repeat)
 *                         └─ 否 → [!active] → IDLE
 *    FIRED:
 *         间隔 >= repeat_interval_ms → return TRUE
 *         [!active] → IDLE
 *
 *  与 LONG_PRESS 的区别：触发后不会停止，而是周期性重复触发。
 *  适用于长按快速增减数值。
 * ──────────────────────────────────────────────────────────── */
static int cb_long_pressing(key_event_t ev, const key_cfg_t *c,
                            key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        phase_idle_to_debounce(s, active);
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;
    }

    case KST_PRESSED: {
        if (!active) {
            pri_key(LVL_DBG, s->dbg_on, "key from pressed to idle\n");
            s->st = KST_IDLE;
            break;
        }

        if (phase_hold_exceeds(s, c->long_press_ms)) {
            s->st = KST_FIRED;
            ms_clock_reset(&s->t_repeat);   /* 开始周期计时 */
            pri_key(LVL_DBG, s->dbg_on,
                    "key exceeds, first tiggered, from pressed to fired\n");
            return TRUE;                    /* 首次触发 */
        }
        break;
    }

    case KST_FIRED: {
        if (!active) {
            s->st = KST_IDLE;
            pri_key(LVL_DBG, s->dbg_on, "key release from fired to idle\n");
            break;
        }

        if (ms_since_previous(&s->t_repeat) >= c->repeat_interval_ms) {
            ms_clock_reset(&s->t_repeat);
            pri_key(LVL_DBG, s->dbg_on,
                    "key repeat triggered, t_repeat >= %d\n", c->repeat_interval_ms);
            return TRUE;                    /* 周期触发 */
        }

        break;
    }
    }

    return FALSE;
}


/* ─────────────────────────────────────────────────────────────
 *  cb_long_press_up — 长按后释放
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED ──[!active]──▶ DEB_REL
 *                             │                       │
 *                      记录 was_long           [释放确认]
 *                                                   │
 *                                  was_long ? return TRUE : IDLE
 *
 *  "长按选择，松手确认" 交互模式。
 *  仅当按住期间曾达到 long_press_ms 阈值，松手后才触发。
 * ──────────────────────────────────────────────────────────── */
static int cb_long_press_up(key_event_t ev, const key_cfg_t *c,
                            key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    switch (s->st) {
    case KST_IDLE: {
        if (phase_idle_to_debounce(s, active)) {
            s->was_long = 0;                /* 新序列开始，清除标志 */
        }
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;
    }

    case KST_PRESSED: {
        /* 先检查阈值再检测松开，避免同一周期内丢失 */
        if (!s->was_long && phase_hold_exceeds(s, c->long_press_ms)) {
            pri_key(LVL_DBG, s->dbg_on, "key pressed exceeds\n");
            s->was_long = 1;
        }
        phase_detect_release(s, active);
        break;
    }

    case KST_DEB_REL: {
        if (phase_release_debounce(s, active, c->debounce_ms) > 0) {
            s->st = KST_IDLE;
            pri_key(LVL_DBG, s->dbg_on,
                    "key from del rel to idle, triggered: %d\n", s->was_long);
            return (s->was_long != 0);      /* 仅曾达长按阈值才触发 */
        }
        break;
    }
    }

    return FALSE;
}

/* ─────────────────────────────────────────────────────────────
 *  cb_click — 连击检测（单击/双击/三击/N击 共用）
 *
 *  状态流:
 *    IDLE ──▶ DEBOUNCE ──▶ PRESSED ──[!active]──▶ DEB_REL
 *                                                     │
 *                                               [释放确认]
 *                                                     │
 *                                             click_count++
 *                                                     │
 *                                   count >= target ?
 *                                     ├─ 是 → return TRUE（序列完成）
 *                                     └─ 否 → WAIT_CLICK
 *
 *    WAIT_CLICK:
 *      [active]     → DEBOUNCE（保留 click_count，累加下一轮）
 *      [窗口超时]   → IDLE（未达目标，放弃本次序列）
 *
 *  由 ev 参数决定目标次数：
 *    CLICK=1, DBL=2, TRL=3, MULTI_CLICK=click_target
 * ──────────────────────────────────────────────────────────── */
static int cb_click(key_event_t ev, const key_cfg_t *c,
                    key_sta_t *s, int active)
{
    pri_key(LVL_LOOP, s->dbg_on, "%s\n", __func__);

    if (c->click_target <= 0) {
        pri_key(LVL_ERR, s->dbg_on, "click target not set\n");
        return FALSE;
    }

    switch (s->st) {
    case KST_IDLE: {
        if (phase_idle_to_debounce(s, active)) {
            s->click_count = 0;            /* 新序列开始，重置计数 */
        }
        break;
    }

    case KST_DEBOUNCE: {
        if (phase_debounce_check(s, active, c->debounce_ms) > 0) {
            phase_enter_pressed(s);
        }
        break;
    }

    case KST_PRESSED: {
        phase_detect_release(s, active);
        break;
    }

    case KST_DEB_REL: {
        if (phase_release_debounce(s, active, c->debounce_ms) > 0) {
            s->click_count++;
            pri_key(LVL_DBG, s->dbg_on,
                    "key click release, count: %d\n", s->click_count);

            if (s->click_count >= c->click_target) {
                /* 达到目标次数 → 触发 */
                s->st = KST_IDLE;
                pri_key(LVL_DBG, s->dbg_on,
                        "key click triggered, from deb rel to idle\n");
                return TRUE;
            }

            /* 未达标 → 进入连击等待窗口 */
            s->st = KST_WAIT_CLICK;
            ms_clock_reset(&s->t_release); /* 窗口计时开始 */
        }
        break;
    }

    case KST_WAIT_CLICK: {
        if (active) {
            /* 窗口期内再次按下，保留 click_count 进入下一轮 */
            s->st = KST_DEBOUNCE;
            ms_clock_reset(&s->t_debounce);
            pri_key(LVL_DBG, s->dbg_on,
                    "key click again, from wait click to debounce\n");
        } else if (ms_since_previous(&s->t_release) >= c->click_window_ms) {
            /* 窗口超时，未达目标次数，放弃 */
            s->st = KST_IDLE;
            pri_key(LVL_DBG, s->dbg_on,
                    "key wait click timeout(t_release >= %d)\n", c->click_window_ms);
            pri_key(LVL_DBG, s->dbg_on, "from wait click to idle\n");
        }
        break;
    }
    }

    return FALSE;
}

/**
 * @brief  事件 → 处理函数 全局映射表
 *
 * 通过 key_event_t 枚举值直接索引，O(1) 分派。
 * CLICK / DOUBLE_CLICK / TRIPLE_CLICK / MULTI_CLICK 共用 cb_click，
 * 由 ev 参数区分目标连击次数。
 */
static const key_handler_fn g_handlers[KEY_EVENT_MAX] = {
    [KEY_EVENT_NONE]            = NULL,
    [KEY_EVENT_PRESS_DOWN]      = cb_press_down,
    [KEY_EVENT_PRESS_UP]        = cb_press_up,
    [KEY_EVENT_PRESSING]        = cb_pressing,
    [KEY_EVENT_PRESS_REPEAT]    = cb_press_repeat,
    [KEY_EVENT_SHORT_PRESS]     = cb_short_press,
    [KEY_EVENT_LONG_PRESS]      = cb_long_press,
    [KEY_EVENT_LONG_PRESSING]   = cb_long_pressing,
    [KEY_EVENT_LONG_PRESS_UP]   = cb_long_press_up,
    [KEY_EVENT_CLICK]           = cb_click,
};

/* ═══════════════════════════════════════════════════════════════
 *  API 实现
 * ═══════════════════════════════════════════════════════════════ */
int key_triggered(const key_config_t *cfg, key_sta_t *sta)
{
    if (NULL == cfg || NULL == sta) {
        ERR("key scan parameters is null!\n");
        return FALSE;
    }

    if (cfg->event <= KEY_EVENT_NONE || cfg->event >= KEY_EVENT_MAX) {
        ERR("invalid key event %d!\n", cfg->event);
        return FALSE;
    }

    key_handler_fn fn = g_handlers[cfg->event];
    if (!fn) {
        ERR("key event %d not supported yet!\n", cfg->event);
        return FALSE;
    }

    /* GPIO 仅读取一次，避免处理函数内重复系统调用 */
    int active = is_key_active(&cfg->param);

    return fn(cfg->event, &cfg->param, sta, active);
}
