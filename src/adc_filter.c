#include "adc_filter.h"

int adc_filter_init(adc_filter_t *f, size_t window)
{
    if (f == NULL || window == 0 || window > ADC_FILTER_MAX_WINDOW) {
        return -1;
    }
    f->window = window;
    adc_filter_reset(f);
    return 0;
}

void adc_filter_reset(adc_filter_t *f)
{
    if (f == NULL) {
        return;
    }
    for (size_t i = 0; i < ADC_FILTER_MAX_WINDOW; i++) {
        f->buf[i] = 0;
    }
    f->count = 0;
    f->index = 0;
    f->sum   = 0;
}

uint16_t adc_filter_update(adc_filter_t *f, uint16_t sample)
{
    if (f == NULL || f->window == 0) {
        return sample;  /* 没配置就当直通，绝不崩 */
    }

    if (f->count == f->window) {
        /* 窗口满了：先减去被挤出去的那个旧值 */
        f->sum -= f->buf[f->index];
    } else {
        f->count++;
    }

    f->buf[f->index] = sample;
    f->sum += sample;
    f->index = (f->index + 1) % f->window;

    return (uint16_t)(f->sum / f->count);
}
