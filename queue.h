/*
#include "Queue.h"

// 依赖于全局秒计数 如果有可以替代
extern struct {
    uint32_t second; // 系统全局秒计数
} PubTimesVar;

Queue_t gMsgQueue;
QueueNode_t gMsgRow;

void App_Init(void)
{
    Queue_Init(&gMsgQueue);
}

void App_MainLoop(void)
{
    // 添加队列元素（类型=1，值=100，超时=5秒）
    Queue_Push(&gMsgQueue, QUEUE_UART, 100, 5, PubTimesVar.second);

    // 定期清理超时项
    Queue_RemoveExpired(&gMsgQueue, PubTimesVar.second);

    // 取出队列内容
    if (Queue_Pop(&gMsgQueue, &gMsgRow, PubTimesVar.second)) {
        // 处理消息

        if (gMsgRow.type == QUEUE_UART) {
            // 示例处理
            if (UART_SendWithRetry(gMsgRow.val)) {
                // 发送成功
                gMsgRow.valid = 0; // 标记为已处理
            }
        }
    }
}


*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define QUEUE_MAX_SIZE 100 // 队列最大长度，可按需修改

/**
 * @brief 队列类型枚举
 * @details 用于区分不同队列中存储的数据类型。
 */
typedef enum {
    QUEUE_UART = 1, ///< 通信数据队列
    QUEUE_PID,      ///< 加热控制相关数据队列
    // 可根据需要增加其它队列类型
} Queue_Type_t;

// 队列扩展参数数量
#define QUEUE_PARAM_NUM 8

/**
 * @brief 队列节点结构体
 * @details 每个节点存储一个队列元素，包括类型、值、扩展参数、时间及有效性标志。
 */
typedef struct {
    Queue_Type_t type;                     // 队列元素类型
    unsigned char val;                     // 队列元素值
    unsigned char params[QUEUE_PARAM_NUM]; // 扩展参数
    unsigned int start_time;               // 创建时间（单位：秒）
    unsigned int expire_time;              // 过期时间（单位：秒）
    unsigned char valid;                   // 有效标志：1=有效，0=无效/过期
} QueueNode_t;

/**
 * @brief 队列结构体
 * @details 使用循环队列实现，支持固定大小存储
 */
typedef struct Queue {
    QueueNode_t items[QUEUE_MAX_SIZE]; // 队列数组存储节点
    unsigned char head;                // 队列头索引
    unsigned char tail;                // 队列尾索引
    unsigned char count;               // 当前队列中有效节点数量
} Queue_t;

// 全局变量声明（建议加前缀避免命名冲突）
extern QueueNode_t gMsgRow; // 临时节点，用于操作队列元素
extern Queue_t gMsgQueue;   // 全局队列实例

/**
 * @brief 队列初始化
 * @param q 队列指针
 * @note 将队列清空，初始化头尾和计数器
 */
void Queue_Init(Queue_t *q);

/**
 * @brief 向队列中推入一个节点
 * @param q 队列指针
 * @param type 节点类型
 * @param val 节点值
 * @param timeout_s 节点过期时间（秒）
 * @param current_second 当前时间（秒）
 * @return 1=成功入队，0=队列满无法入队
 */
unsigned char Queue_Push(Queue_t *q, Queue_Type_t type, unsigned char val, const unsigned char *params, unsigned int timeout_s, unsigned int current_second);

/**
 * @brief 从队列中弹出一个节点
 * @param q 队列指针
 * @param out 用于存储弹出的节点
 * @param current_second 当前时间（秒）
 * @return 1=成功弹出，0=队列为空或无有效节点
 */
unsigned char Queue_Pop(Queue_t *q, QueueNode_t *out, unsigned int current_second);

/**
 * @brief 移除队列中已过期的节点
 * @param q 队列指针
 * @param current_second 当前时间（秒）
 */
void Queue_RemoveExpired(Queue_t *q, unsigned int current_second);

/**
 * @brief 应用主循环示例
 * @param current_second 当前时间（秒）
 * @note 建议每100ms调用一次，用于处理队列任务
 */
void App_MainLoop(unsigned int current_second);

#endif // _QUEUE_H_
