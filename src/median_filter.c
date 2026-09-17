/**
 * @file median_filter.c
 * @brief 中值滤波器的实现：环形缓冲 + 插入排序取中值。
 *
 * 设计约束（面向嵌入式）：
 *   - 不使用 malloc/free，全部内存由调用者提供或在栈上分配；
 *   - 不使用递归，最坏执行时间可预测；
 *   - 窗口上限 16，插入排序 O(n^2) 最坏 256 次比较，开销可忽略。
 */
#include "median_filter.h"

/**
 * @brief  初始化中值滤波器。
 * @param[out] f      待初始化的滤波器实例，不可为 NULL。
 * @param[in]  window 窗口大小（采样点数），有效范围 1..MEDIAN_FILTER_MAX_WINDOW。
 * @return int 0 表示成功；-1 表示参数非法（f 为 NULL，或 window 越界）。
 */
int median_filter_init(median_filter_t *f, size_t window)
{
    /* 先校验参数，避免把非法窗口写进实例 */
    if (f == NULL || window == 0 || window > MEDIAN_FILTER_MAX_WINDOW) {
        return -1;
    }

    f->window = window;
    median_filter_reset(f);
    return 0;
}

/**
 * @brief  清空内部状态，窗口配置保留。
 * @param[out] f 滤波器实例；为 NULL 时安全返回。
 * @return void 无返回值。
 */
void median_filter_reset(median_filter_t *f)
{
    /* 防御性检查：嵌入式里空指针崩溃往往比功能错误更致命 */
    if (f == NULL) {
        return;
    }

    for (size_t i = 0; i < MEDIAN_FILTER_MAX_WINDOW; i++) {
        f->buf[i] = 0;
    }
    f->count = 0;
    f->index = 0;
}

/**
 * @brief  送入一个采样值，返回当前窗口的中值。
 * @param[in,out] f      滤波器实例；为 NULL 时按直通处理，绝不崩溃。
 * @param[in]     sample 本次采样值。
 * @return uint16_t 滤波结果。窗口未满时按"已累积样本"求中值：
 *         奇数个取正中间，偶数个取中间两个的算术平均。
 */
uint16_t median_filter_update(median_filter_t *f, uint16_t sample)
{
    if (f == NULL || f->window == 0) {
        return sample; /* 没配置就当直通，绝不崩 */
    }

    /* --- 第 1 步：把新样本写入环形缓冲区 --- */
    f->buf[f->index] = sample;
    f->index = (f->index + 1) % f->window;
    if (f->count < f->window) {
        f->count++;
    }

    /* --- 第 2 步：复制有效样本再排序 --- */
    /* 为什么要复制？因为排序会打乱顺序，而环形缓冲区必须保持写入次序，
     * 否则下一次覆盖的位置就错了。窗口 <= 16，复制 32 字节代价极低。 */
    uint16_t sorted[MEDIAN_FILTER_MAX_WINDOW];
    for (size_t i = 0; i < f->count; i++) {
        sorted[i] = f->buf[i];
    }

    /* 插入排序：窗口小、近乎有序时表现好，且无递归、无动态内存 */
    for (size_t i = 1; i < f->count; i++) {
        const uint16_t key = sorted[i];
        size_t         j   = i;

        while (j > 0 && sorted[j - 1] > key) {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = key;
    }

    /* --- 第 3 步：取中值 --- */
    if ((f->count % 2u) == 1u) {
        /* 奇数个样本：正中间那个 */
        return sorted[f->count / 2u];
    }

    /* 偶数个样本：中间两个的平均（先提升到 32 位，避免加法溢出） */
    return (uint16_t)(((uint32_t)sorted[(f->count / 2u) - 1u] +
                       (uint32_t)sorted[f->count / 2u]) / 2u);
}
