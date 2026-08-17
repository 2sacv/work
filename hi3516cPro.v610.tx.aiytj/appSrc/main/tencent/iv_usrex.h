#ifdef PLATFORM_TENCENT
#ifndef __IVM_USREX_H_
#define __IVM_USREX_H_

#include "qcloud_iot_export.h"
#include "iv_def.h"
#include "iv_dm.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
以下函数由开发者应用层去实现，实现具体的业务功能:
1. 云平台在接收到对设备的设置操作(更改ProWritable)或控制操作(Action)时,会主动通知设备;
2. 设备SDK通过iv_usrcb_ProWritable_xxx/iv_usrcb_Action_xxx系列函数通知应用层;
3. 函数的参数，即为对应物模型对象的指针;
*/
#define MAX_ARRAY_JSON_STR_LEN (512)
typedef struct {
    struct {
        TYPE_DEF_TEMPLATE_STRING m_jcpParams[2048+1];
    }action_in;
    struct {
        TYPE_DEF_TEMPLATE_STRING m_result[2048+1];
    }action_out;
}ivm_DataServicesByJCP_t;

typedef struct {
    TYPE_DEF_TEMPLATE_ENUM m_eventType;
    TYPE_DEF_TEMPLATE_STRING m_payload[2048+1];
    TYPE_DEF_TEMPLATE_INT m_channel;
} ivm_uploadDeviceEvent;

typedef struct {
    struct {
        TYPE_DEF_TEMPLATE_INT m_cid;
        TYPE_DEF_TEMPLATE_INT m_lac;
        TYPE_DEF_TEMPLATE_INT m_mnc;
        TYPE_DEF_TEMPLATE_INT m_mcc;
        TYPE_DEF_TEMPLATE_ENUM m_networkType;
        TYPE_DEF_TEMPLATE_INT m_location_4g;
        TYPE_DEF_TEMPLATE_INT m_CallStatus;
    } ProReadonly;
    struct {
        TYPE_DEF_TEMPLATE_BOOL m_record_enable;
    } ProWritable;
    struct {
        ivm_uploadDeviceEvent m_uploadDeviceEvent;
    } Event;
} ivm_extend_param_t;

extern   ivm_extend_param_t     g_ivm_objs;

/*!    \brief	物模型实例全局变量,应用层可直接访问/修改这个对象的成员变量。
        注意:
        1. 设备开发者可直接访问g_ivm_objs全局变量访问/修改物模型
        如：对一个int32类型的物模型对象yyyy的状态置值，应用层可直接操作：
        ```
        ivm_lock();		//互斥加锁
        g_ivm_objs.ProReadonly.yyyy = 1;		       //置状态值
        iv_dm_property_report(yyy, cb, param); //置状态
        ivm_unlock();	//互斥解锁
        ```
        2. 访问互斥
        * 在iv_usrcb_xxxxx系列回调函数中访问时g_ivm_objs时，无需加锁
        * 在其它任何地方访问g_ivm_objs，都应该调用ivm_lock()/ivm_unlock()作互斥处理
        3. 不得在iv_usrcb_xxxxx系列回调函数进行导致阻塞或耗时的操作，这样会阻塞核心通讯线程.
        4. 读写表示该属性即可从设备端上报到云端也可从云端发起控制，只读表示该属性只从设备向云端上报。
        */
/*! 
 \brief  更新物模型 --整形
 */ 
#define ivm_ProReadonly_setInt(obj, _val) \
   do {                                   \
       ivm_lock();                        \
       g_ivm_objs.ProReadonly.m_##obj = _val; \
       iv_dm_property_report(#obj, NULL, NULL);\
       ivm_unlock();                      \
   } while (0)

#define ivm_ProWritable_setInt(obj, _val) \
   do {                                   \
       ivm_lock();                        \
       g_ivm_objs.ProWritable.m_##obj = _val; \
       iv_dm_property_report(#obj, NULL, NULL);\
       ivm_unlock();                      \
   } while (0)
/*! 
 \brief  更新物模型 --字符串
 */ 
#define ivm_ProReadonly_setString(obj, _str)  \
   do {                                   \
       ivm_lock();                        \
       strncpy(g_ivm_objs.ProReadonly.m_##obj, _str, sizeof(g_ivm_objs.ProReadonly.m_##obj) - 1); \
       iv_dm_property_report(#obj, NULL, NULL);       \
       ivm_unlock();                      \
   } while (0)

#define ivm_ProWritable_setString(obj, _str)  \
   do {                                   \
       ivm_lock();                        \
       strncpy(g_ivm_objs.ProWritable.m_##obj, _str, sizeof(g_ivm_objs.ProWritable.m_##obj) - 1); \
       iv_dm_property_report(#obj, NULL, NULL);       \
       ivm_unlock();                      \
   } while (0)
/*! 
 \brief  更新结构体--字符串成员
 */ 
#define ivm_ProWritable_setStructString(obj, member, _str)  \
   do {                                   \
       strncpy(g_ivm_objs.ProWritable.m_##obj.m_##member, _str, sizeof(g_ivm_objs.ProWritable.m_##obj.m_##member) - 1); \
   } while (0)

int iv_usrcb_ProWritable_record_enable(const TYPE_DEF_TEMPLATE_BOOL *record_enable);

int iv_usrcb_Action_DataServicesByJCP(ivm_DataServicesByJCP_t *DataServicesByJCP);

#ifdef __cplusplus
}
#endif

#endif
#endif //PLATFORM_TENCENT 

