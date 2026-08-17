/*
 *       Filename:  key_scan.h
 *    Description:  
 *        Version:  1.0
 *        Created:  05/13/2026 09:03:45 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _KEY_SCAN_H
#define _KEY_SCAN_H
#ifdef __cplusplus 
extern "C" {
#endif

#include <string.h>

/* ═══════════════════════════════════════════════════════════════
 *  事件枚举 key_event_t
 *
 *  按语义分为四层，每层有明确的触发时机和典型用途：
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    KEY_EVENT_NONE = 0,             // 无事件（空闲/初始状态）

    // ── 电气层：硬件边沿事件 ──
    KEY_EVENT_PRESS_DOWN,           // 按下瞬间，消抖确认后触发，仅一次。适用于需要即时响应的场景（如开始计时）。
    KEY_EVENT_PRESS_UP,             // 释放瞬间，消抖确认后触发，仅一次。适用于松手动作（如抬起确认）。

    // ── 持续层：按住期间周期性事件 ──
    KEY_EVENT_PRESSING,             // 按住中， 每次扫描均触发（无消抖、无状态机）。注意：调用频率等于扫描频率（约 100Hz），适用于实时状态指示，不适用于动作触发。
    KEY_EVENT_PRESS_REPEAT,         // 按住连发，按住超过 repeat_start_ms 后首次触发，之后每 repeat_interval_ms 再次触发。适用于音量连续调节等。

    // ── 时间层：基于按住持续时间 ──
    KEY_EVENT_SHORT_PRESS,          // 短按，释放时按住持续时间 / long_press_ms。long_press_ms == 0 时无上限。适用于普通按键动作。
    KEY_EVENT_LONG_PRESS,           // 长按，按住持续时间首次达到 long_press_ms。仅触发一次（按住不放也只触发一次）。适用于模式切换、菜单进入、恢复出厂、紧急操作等。
    KEY_EVENT_LONG_PRESSING,        // 长按连发，超过 long_press_ms 后首次触发，之后每 repeat_interval_ms 再次触发。适用于长按快速调节。
    KEY_EVENT_LONG_PRESS_UP,        // 长按后释放，按住曾达到 long_press_ms 阈值后再松手。适用于"长按选择，松手确认"交互。

    // ── 动作层：基于连续按放次数 ──
    KEY_EVENT_CLICK,                // 点击 click_window_ms 内完成 click_target 次按放。

    KEY_EVENT_MAX
} key_event_t;

/* ═══════════════════════════════════════════════════════════════
 *  用户配置结构体 key_cfg_t
 *
 *  所有事件共用同一配置结构体。
 *  用户根据 event 类型填写相关字段，不相关的字段保持 0 即可
 * （0 表示禁用/无限制，各 handler 内部会正确处理）。
 *
 *  典型用法:
 *    key_config_t cfg = {
 *        .event = KEY_EVENT_LONG_PRESS,
 *        .param = {
 *            .gpio           = 10,
 *            .active_level   = 0,
 *            .debounce_ms    = 20,
 *            .long_press_ms  = 1000,
 *            // 其余字段为 0，自动禁用
 *        },
 *    };
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    /* ── 基础字段（所有事件必填） ── */
    int      gpio;                  // GPIO 编号
    int      active_level;          // 有效电平：0 = 低电平有效（按下接地）, 1 = 高电平有效（按下接 VCC）
    uint16_t debounce_ms;           // 消抖时间（毫秒）, 建议 15~30, 过短可能误触发, 过长感觉迟钝

    /* ── 长按相关 ── */
    uint16_t long_press_ms;         // 长按判定阈值（毫秒）, 使用事件: SHORT_PRESS（作为上限，超过此值不算短按）,
                                    // LONG_PRESS, LONG_PRESSING, LONG_PRESS_UP, 0 = 无限制（SHORT_PRESS 不设上限）

    /* ── 连发相关 ── */
    uint16_t repeat_start_ms;       // 连发首次触发前的等待时间（毫秒）, 使用事件: PRESS_REPEAT, 0 = 禁用按住连发
    uint16_t repeat_interval_ms;    // 连发/长按连发的周期间隔（毫秒）, 使用事件: PRESS_REPEAT, LONG_PRESSING, 0 = 禁用

    /* ── 连击相关 ── */
    uint16_t click_window_ms;       // 连击判定窗口（毫秒）, 建议 250~400, 使用事件: CLICK, 0 = 禁用连击
    uint8_t  click_target;          // CLICK 的目标连击次数, 使用事件: CLICK
} key_cfg_t;

/** @brief 总配置结构体：事件类型 + 参数 */
typedef struct {
    key_event_t event;              // 要监测的事件类型
    key_cfg_t   param;              // 事件参数配置
} key_config_t;

/* ═══════════════════════════════════════════════════════════════
 *  内部状态机状态定义
 *
 *  所有 handler 共用同一套状态值，存放在 key_sta_t.st 中。
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    KST_IDLE        = 0,    /**< 空闲：等待有效电平 */
    KST_DEBOUNCE    = 1,    /**< 按下消抖中：等待电平稳定 */
    KST_PRESSED     = 2,    /**< 确认按下：监测持续时长或等待松开 */
    KST_DEB_REL     = 3,    /**< 释放消抖中：等待释放电平稳定 */
    KST_WAIT_CLICK  = 4,    /**< 连击等待窗口：等待下一次按下或窗口超时 */
    KST_FIRED       = 5,    /**< 已触发：等待松开后回到空闲 */
} key_stamach_t;

/* ═══════════════════════════════════════════════════════════════
 *  运行时状态结构体 key_sta_t
 *
 *  所有事件共用同一状态结构体，内部由 key_scan 管理。
 *  用户只需：
 *    - 分配内存并零初始化
 *    - 将指针传入 key_scan
 *    - 如需暂存（如休眠前保存/恢复），可整体拷贝
 *
 *  字段说明:
 *    t_debounce  - 消抖计时参考点
 *                  所有事件均使用
 *    t_press     - 记录按下确认时刻，用于计算按住持续时间
 *                  使用事件: PRESS_REPEAT, SHORT_PRESS, LONG_PRESS,
 *                            LONG_PRESSING, LONG_PRESS_UP, LONG_LONG_PRESS,
 *                            CLICK
 *    t_repeat    - 连发/长按连发的周期计时参考点
 *                  使用事件: PRESS_REPEAT, LONG_PRESSING
 *    t_release   - 记录释放确认时刻，用于连击窗口计时
 *                  使用事件: CLICK
 *    st          - 内部状态机当前状态（见 KST_* 枚举）
 *                  所有事件均使用
 *    was_long    - 标记按下期间是否已达到长按阈值
 *                  使用事件: LONG_PRESS_UP
 *    click_count - 跨多次按放累加的连击计数
 *                  使用事件: CLICK
 *    reserve     - 保留字段，仅用于字节填充
 *    dbg_on      - 调试模式打印开关
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    struct timespec t_debounce;     // 消抖计时参考点
    struct timespec t_press;        // 按下确认时刻
    struct timespec t_repeat;       // 连发周期计时参考点
    struct timespec t_release;      // 释放确认时刻
    uint8_t         st;             // 内部状态机状态
    uint8_t         was_long;       // 长按阈值已达标志
    uint8_t         click_count;    // 连击累加计数
    uint8_t         reserve;        // 保留字段
    uint32_t        dbg_on;         // 调试模式是否开启
} key_sta_t;

/**
 * @brief  重置运行时状态为初始值
 *
 * 首次使用前必须调用，或用 key_sta_t sta = {0} 零初始化。
 * 仅重置运行时状态，不影响配置。
 *
 * @param  sta  状态指针（不可为 NULL）
 */
static inline void key_state_reset(key_sta_t *sta)
{
    memset(sta, 0, sizeof(*sta));
}

/**
 * @brief  按键扫描主接口 — 每个轮询周期调用一次
 *
 * 内部流程:
 *   1. 参数校验
 *   2. 读取 GPIO 电平（仅一次系统调用）
 *   3. 根据 event 查全局映射表，分派到对应处理函数
 *   4. 处理函数内运行状态机，返回是否触发
 *
 * @param  cfg  用户配置指针（不可为 NULL，运行期间只读）
 * @param  sta  运行时状态指针（不可为 NULL，内部管理）
 * @return true  = 本次检测到配置的事件已触发
 * @return false = 无事件触发
 *
 * @note  建议以 10~100ms 固定周期调用
 * @note  多按键场景: 每个按键独立的 (cfg, sta)，循环调用
 * @note  MULTI_CLICK 触发时，实际连击次数可通过 sta->click_count 获取
 * @note  本函数非线程安全，多线程使用需外部加锁
 */
int key_triggered(const key_config_t *cfg, key_sta_t *sta);

#ifdef __cplusplus
}
#endif
#endif
