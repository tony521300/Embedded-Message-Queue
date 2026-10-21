/**
 * @file queue.c
 * @brief 固定大小循环队列实现（基于 Queue_t / QueueNode_t），支持每项超时失效。
 *
 * 该模块在一个数组上实现环形队列，使用 head/tail 指针和 count 记录元素数。
 * 每个队列项包含一个有效标志 valid 和一个过期时间 expire_time（以秒为单位的时间戳）。
 *
 * 约定：
 *  - QUEUE_MAX_SIZE 为队列的最大元素数（编译期常量）。
 *  - 使用 current_second（秒）与每项的 expire_time 比较判断是否过期。
 *  - 返回值通常用 unsigned char 表示布尔结果：1 表示成功/存在有效项，0 表示失败/空/满等。
 *
 * 线程安全：本实现未做任何并发保护，如果在多任务/中断环境中使用，调用者须加锁/禁用中断。
 * @author Cnn
 * @date 2025-11-03
 */

#include "Queue.h"

QueueNode_t gMsgRow = {0}; // 节点，用于操作队列元素
Queue_t gMsgQueue   = {0}; // 全局队列实例

/**
 * @brief 判断节点是否过期（防溢出）0xFFFF
 */
#define IS_EXPIRED(now, start, exp) ((unsigned int)((now) - (start)) >= (unsigned int)((exp) - (start)))
/**
 * @brief 队列初始化
 * @param q 队列指针
 * @note 将队列清空，初始化头尾和计数器
 */
void Queue_Init(Queue_t *q)
{
    unsigned char i;
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
    for (i = 0; i < QUEUE_MAX_SIZE; i++) {
        q->items[i].valid = 0;
    }
}

/**
 * @brief 向队列中推入一个节点
 * @param q 队列指针
 * @param type 节点类型
 * @param val 节点值
 * @param timeout_s 节点过期时间（秒）
 * @param current_second 当前时间（秒）
 * @return 1=成功入队，0=队列满无法入队
 */
unsigned char Queue_Push(Queue_t *q, Queue_Type_t type, unsigned char val, const unsigned char *params, unsigned int timeout_s, unsigned int current_second)
{
    unsigned char i;
    QueueNode_t *node; // 定义

    if (q->count >= QUEUE_MAX_SIZE) {
        return 0; // 队列已满
    }

    node       = &q->items[q->tail];
    node->type = type;
    node->val  = val;
    for (i = 0; i < QUEUE_PARAM_NUM; i++) {
        node->params[i] = params[i];
    }
    node->start_time  = current_second;
    node->expire_time = current_second + timeout_s;
    node->valid       = 1;

    q->tail = (q->tail + 1) % QUEUE_MAX_SIZE;
    q->count++;
    return 1;
}

/**
 * @brief 从队列中弹出一个节点
 * @param q 队列指针
 * @param out 用于存储弹出的节点
 * @param current_second 当前时间（秒）
 * @return 1=成功弹出，0=队列为空或无有效节点
 */
unsigned char Queue_Pop(Queue_t *q, QueueNode_t *out, unsigned int current_second)
{
    if (q->count == 0) {
        return 0; // 队列为空
    }

    // 跳过无效或过期项
    while (q->count > 0) {
        QueueNode_t *node = &q->items[q->head];

        if (!node->valid || IS_EXPIRED(current_second, node->start_time, node->expire_time)) {
            node->valid = 0;
            q->head     = (q->head + 1) % QUEUE_MAX_SIZE;
            q->count--;
            continue;
        }

        // 有效项输出
        *out        = *node;
        node->valid = 0;
        q->head     = (q->head + 1) % QUEUE_MAX_SIZE;
        q->count--;
        return 1;
    }

    return 0; // 所有项都无效
}

/**
 * @brief 移除队列中已过期的节点
 * @param q 队列指针
 * @param current_second 当前时间（秒）
 */
void Queue_RemoveExpired(Queue_t *q, unsigned int current_second)
{
    unsigned char i;
    for (i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (q->items[i].valid && IS_EXPIRED(current_second, q->items[i].start_time, q->items[i].expire_time)) {
            q->items[i].valid = 0;
        }
    }
}

/**
 * @brief 应用主循环示例
 * @param current_second 当前时间（秒）
 * @note 建议每>=100ms调用一次，用于处理队列任务
 */
void App_MainLoop(unsigned int current_second)
{
    // 添加队列元素（类型=1，值=100，超时=5秒）
    // Queue_Push(&gMsgQueue, 1, 100, 5, current_second);

    // 定期清理超时项
    Queue_RemoveExpired(&gMsgQueue, current_second);

    // 取出队列内容
    if (Queue_Pop(&gMsgQueue, &gMsgRow, current_second)) {
        // 处理消息

        if (gMsgRow.type == 0x01) {
            // 示例处理
            // if (UART_SendWithRetry(gMsgRow.val)) {
            //     // 发送成功处理

            // }
        }
    }
}