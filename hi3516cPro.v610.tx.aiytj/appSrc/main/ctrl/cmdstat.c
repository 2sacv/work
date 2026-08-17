/* 
 *       Filename:  cmdstat.c
 *        Created:  2023-04-17 20:49:21
 *         Author:  zhangjian wuhy,
 *   Organization:  JCO
 */
#include "cmdstat.h"

/* 所有事件(config+alarm)一次性获取，在 loop() 中串行执行 */
int cmd_get_command(struct cmdstat *p_cmd)
{
    int cmd;
    pthread_mutex_lock(&(p_cmd->mutex));

    // diff_cfg2cmd 只做 diff copy set_command()，耗时操作在 loop 中执行
    if (p_cmd->diff_cfg2cmd != NULL) {
        p_cmd->diff_cfg2cmd((void*)p_cmd);
    }
    cmd = (p_cmd->cmd_stage | p_cmd->cmd_self);
    p_cmd->cmd_stage = p_cmd->cmd_self = 0;

    pthread_mutex_unlock(&(p_cmd->mutex));

    return cmd;
}

void cmd_set_command(struct cmdstat *p_cmd, int bit)
{
    p_cmd->cmd_self |= bit;
}

void cmd_clr_command(struct cmdstat *p_cmd, int bit)
{
    p_cmd->cmd_self &= ~bit;
}
