/* 
 *       Filename:  aliyun_mmi_cfg.c
 *    Description:  
 *        Version:  1.0
 *        Created:  07/10/2026 10:22:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include <string.h>
#include <stdlib.h>

#include "g_log.h"
#include "utils.h"
#include "factory_db.h"
#include "aliyun_mmi_cfg.h"

int aliyun_mmi_load_triple_key(sMmiTripleKey *p_key)
{
    int membs_loaded = 0;
    int ret = SUCCESS;

    membs_loaded = LoadFile2(F_MMI_TRIPLE, FMT_SCANF_TRIPLE_KEY(p_key));

    if (CNT_TRIPLE_KEY_MEMBS != membs_loaded) {
        ERR("failed to load %s, should %d, real %d membs\n",
            F_MMI_TRIPLE, CNT_TRIPLE_KEY_MEMBS, membs_loaded);
        ret = FAILURE;
    } else {
        pri_mmi(LVL_DBG, "appid: %s\n", p_key->app_id);
        pri_mmi(LVL_DBG, "app_secret: %s\n", p_key->app_secret);
        pri_mmi(LVL_DBG, "device_name: %s\n", p_key->device_name);

#ifdef MMI_HOSTING_FULL
        pri_mmi(LVL_DBG, "api_key: %s\n", p_key->api_key);
#endif

        pri_mmi(LVL_DBG, "workspace_id: %s\n", p_key->workspace_id);

        ret = SUCCESS;
    }

    return ret;
}

int aliyun_mmi_dump_triple_key(sMmiTripleKey *p_key)
{
    char buf_saved[BYTES_MMI_TRIPLE] = {0};
    int membs_assembed = 0, bytes_saved = 0;
    int ret = SUCCESS;

    membs_assembed = snprintf(buf_saved, sizeof(buf_saved) - 1,
                              FMT_SNPRINTF_TRIPLE_KEY(p_key));
    goto_if_fatal_err(CNT_TRIPLE_KEY_MEMBS == membs_assembed, exit, ret = FAILURE,
                      "failed to assemb mmi triple key, should %d, real %d membs\n",
                      CNT_TRIPLE_KEY_MEMBS, membs_assembed);

    pri_mmi(LVL_DBG, "triple key will be saved:\n%s\n", buf_saved);

    bytes_saved = strlen(buf_saved);
    ret = DumpFile(F_MMI_TRIPLE, buf_saved, bytes_saved);
    goto_if_fatal_err(bytes_saved == ret, exit, ret = FAILURE,
                      "failed to dump %s, should %d, real %d bytes\n",
                      F_MMI_TRIPLE, bytes_saved, ret);  

    DBG("succ to dump mmi triple key\n");
    ret = SUCCESS;

exit:

    return ret;
}
