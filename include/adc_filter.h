/**
 * adc_filter.h - 滑动平均滤波器（纯逻辑，不依赖任何硬件）
 *
 * 这个文件里没有 HAL、没有寄存器、没有中断。
 * 正因为如此，它才能在 PC 上编译、在 GitHub 的服务器上跑测试。
 * 这就是"解耦"的全部意义。
 */
#ifndef ADC_FILTER_H
#define ADC_FILTER_H

#include <stddef.h>
#include <stdint.h>

#define ADC_FILTER_MAX_WINDOW 16

typedef struct {
    uint16_t buf[ADC_FILTER_MAX_WINDOW];
    size_t   window;   /* 窗口大小（采样点数） */
    size_t   count;    /* 当前已累积的样本数，最大到 window */
    size_t   index;    /* 下一个要覆盖的位置 */
    uint32_t sum;      /* 运行总和，避免每次重新遍历 */
} adc_filter_t;

/**
 * 初始化滤波器。
 * @return 0 成功；-1 参数非法（f 为 NULL，或 window 不在 1..ADC_FILTER_MAX_WINDOW）
 */
int adc_filter_init(adc_filter_t *f, size_t window);

/** 清空内部状态，窗口配置保留 */
void adc_filter_reset(adc_filter_t *f);

/**
 * 送入一个采样值，返回滤波后的结果。
 * 前 count < window 个样本返回的是"到目前为止的平均值"。
 */
uint16_t adc_filter_update(adc_filter_t *f, uint16_t sample);

#endif /* ADC_FILTER_H */
