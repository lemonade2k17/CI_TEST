/**
 * @file median_filter.h
 * @brief 中值滤波器：用排序把尖峰整个丢掉（纯逻辑，不依赖任何硬件）。
 *
 * 与滑动平均滤波器的区别：
 *   - 滑动平均对尖峰只能"稀释"：一个 4000 的尖峰仍会按 1/N 的比例污染输出；
 *   - 中值滤波直接把尖峰"剔除"：排序后它落在最边上，根本不参与取值。
 *   代价是需要排序。窗口 <= 16 时开销可忽略，因此在 ADC 去毛刺场景里非常常用。
 *
 * 本文件不含 HAL、寄存器、中断，因此可在 PC 上编译并在 CI 上运行测试。
 */
#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <stddef.h>
#include <stdint.h>

/** 窗口上限：栈上排序缓冲区的尺寸由此决定，16 个 uint16_t 仅占 32 字节 */
#define MEDIAN_FILTER_MAX_WINDOW 16

/** 中值滤波器实例（调用者负责分配，本模块不做任何动态内存分配） */
typedef struct {
    uint16_t buf[MEDIAN_FILTER_MAX_WINDOW]; /**< 环形缓冲区 */
    size_t   window;                        /**< 窗口大小（采样点数） */
    size_t   count;                         /**< 已累积样本数，最大到 window */
    size_t   index;                         /**< 下一个要覆盖的位置 */
} median_filter_t;

/**
 * @brief  初始化中值滤波器。
 * @param[out] f      待初始化的滤波器实例，不可为 NULL。
 * @param[in]  window 窗口大小（采样点数），有效范围 1..MEDIAN_FILTER_MAX_WINDOW。
 * @return int 0 表示成功；-1 表示参数非法（f 为 NULL，或 window 越界）。
 */
int median_filter_init(median_filter_t *f, size_t window);

/**
 * @brief  清空内部状态，窗口配置保留。
 * @param[out] f 滤波器实例；为 NULL 时安全返回。
 * @return void 无返回值。
 */
void median_filter_reset(median_filter_t *f);

/**
 * @brief  送入一个采样值，返回当前窗口的中值。
 * @param[in,out] f      滤波器实例；为 NULL 时按直通处理，绝不崩溃。
 * @param[in]     sample 本次采样值。
 * @return uint16_t 滤波结果。窗口未满时按"已累积样本"求中值：
 *         奇数个取正中间，偶数个取中间两个的算术平均。
 */
uint16_t median_filter_update(median_filter_t *f, uint16_t sample);

#endif /* MEDIAN_FILTER_H */
