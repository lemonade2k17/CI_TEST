/**
 * 单元测试：在 PC 上运行，不需要开发板。
 *
 * 故意不依赖 GoogleTest 等第三方库，只用标准 C，
 * 这样 GitHub 的服务器上什么都不用装就能跑起来。
 * 等你熟悉了，再换成 GoogleTest / Unity 都可以。
 *
 * 运行方式：ctest --test-dir build --output-on-failure
 * 退出码非 0 = 测试失败 = CI 变红。
 */
#include <stdio.h>

#include "adc_filter.h"
#include "median_filter.h"

static int g_total  = 0;
static int g_failed = 0;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        g_total++;                                                    \
        if (cond) {                                                   \
            printf("  [ ok ] %s\n", (msg));                           \
        } else {                                                      \
            g_failed++;                                               \
            printf("  [FAIL] %s  (%s:%d)\n", (msg), __FILE__, __LINE__); \
        }                                                             \
    } while (0)

static void test_init_rejects_bad_window(void)
{
    adc_filter_t f;
    printf("test_init_rejects_bad_window\n");
    CHECK(adc_filter_init(&f, 0) == -1, "window = 0 应被拒绝");
    CHECK(adc_filter_init(&f, ADC_FILTER_MAX_WINDOW + 1) == -1,
          "window 超过上限应被拒绝");
    CHECK(adc_filter_init(&f, 1) == 0, "window = 1 应成功");
}

static void test_first_sample_passes_through(void)
{
    adc_filter_t f;
    printf("test_first_sample_passes_through\n");
    adc_filter_init(&f, 4);
    CHECK(adc_filter_update(&f, 1000) == 1000, "第一个样本原样返回");
}

static void test_constant_input_is_stable(void)
{
    adc_filter_t f;
    printf("test_constant_input_is_stable\n");
    adc_filter_init(&f, 4);
    for (int i = 0; i < 20; i++) {
        adc_filter_update(&f, 512);
    }
    CHECK(adc_filter_update(&f, 512) == 512, "恒定输入应输出恒定值");
}

static void test_moving_average_value(void)
{
    adc_filter_t f;
    printf("test_moving_average_value\n");
    adc_filter_init(&f, 3);

    CHECK(adc_filter_update(&f, 1) == 1, "窗口未满时用当前均值 (1/1)");
    CHECK(adc_filter_update(&f, 2) == 1, "窗口未满时用当前均值 (3/2)");
    CHECK(adc_filter_update(&f, 3) == 2, "窗口满时均值为 6/3");
    CHECK(adc_filter_update(&f, 4) == 3, "滑出旧值后均值为 (2+3+4)/3");
    CHECK(adc_filter_update(&f, 5) == 4, "继续滑动均值为 (3+4+5)/3");
}

static void test_spike_is_smoothed(void)
{
    adc_filter_t f;
    printf("test_spike_is_smoothed\n");
    adc_filter_init(&f, 4);
    for (int i = 0; i < 4; i++) {
        adc_filter_update(&f, 100);
    }
    uint16_t out = adc_filter_update(&f, 4000);  /* 一个尖峰 */
    CHECK(out == 1075, "尖峰被拉低到 1075");
    CHECK(out < 4000, "输出明显小于原始尖峰");
}

static void test_reset_clears_state(void)
{
    adc_filter_t f;
    printf("test_reset_clears_state\n");
    adc_filter_init(&f, 4);
    adc_filter_update(&f, 1000);
    adc_filter_update(&f, 1000);
    adc_filter_reset(&f);
    CHECK(adc_filter_update(&f, 7) == 7, "复位后第一个样本原样返回");
}

static void test_null_is_safe(void)
{
    printf("test_null_is_safe\n");
    CHECK(adc_filter_update(NULL, 42) == 42, "NULL 指针不应崩溃");
    adc_filter_reset(NULL);                  /* 不应崩溃 */
    CHECK(1, "reset(NULL) 安全返回");
}

/* ===========================================================================
 * 中值滤波器测试
 * =========================================================================== */

static void test_median_init_rejects_bad_window(void)
{
    median_filter_t f;
    printf("test_median_init_rejects_bad_window\n");
    CHECK(median_filter_init(&f, 0) == -1, "window = 0 应被拒绝");
    CHECK(median_filter_init(&f, MEDIAN_FILTER_MAX_WINDOW + 1) == -1,
          "window 超过上限应被拒绝");
    CHECK(median_filter_init(&f, 3) == 0, "window = 3 应成功");
}

static void test_median_first_sample_passes_through(void)
{
    median_filter_t f;
    printf("test_median_first_sample_passes_through\n");
    median_filter_init(&f, 5);
    CHECK(median_filter_update(&f, 1234) == 1234, "第一个样本原样返回");
}

static void test_median_odd_window(void)
{
    median_filter_t f;
    const uint16_t   in[5] = {50, 10, 40, 20, 30};
    uint16_t         out   = 0;
    printf("test_median_odd_window\n");
    median_filter_init(&f, 5);
    for (int i = 0; i < 5; i++) {
        out = median_filter_update(&f, in[i]);
    }
    /* 排序后为 10,20,30,40,50，正中间是 30 */
    CHECK(out == 30, "5 点窗口的中值是排序后的第 3 个（30）");
}

static void test_median_even_window(void)
{
    median_filter_t f;
    const uint16_t   in[4] = {10, 20, 30, 40};
    uint16_t         out   = 0;
    printf("test_median_even_window\n");
    median_filter_init(&f, 4);
    for (int i = 0; i < 4; i++) {
        out = median_filter_update(&f, in[i]);
    }
    /* 排序后为 10,20,30,40，中间两个是 20 和 30 */
    CHECK(out == 25, "4 点窗口取中间两个的平均 (20+30)/2");
}

static void test_median_removes_spike(void)
{
    median_filter_t f;
    uint16_t         out;
    printf("test_median_removes_spike\n");
    median_filter_init(&f, 3);
    median_filter_update(&f, 100);
    median_filter_update(&f, 100);
    out = median_filter_update(&f, 4000); /* 一个尖峰 */
    /* 排序后为 100,100,4000，中值是 100 —— 尖峰被整个丢掉 */
    CHECK(out == 100, "3 点中值把尖峰完全剔除");
    CHECK(out < 4000, "输出远小于原始尖峰");
}

static void test_median_slides(void)
{
    median_filter_t f;
    printf("test_median_slides\n");
    median_filter_init(&f, 3);
    CHECK(median_filter_update(&f, 1) == 1, "窗口未满：中值为 1");
    CHECK(median_filter_update(&f, 2) == 1, "窗口未满：中值为 (1+2)/2");
    CHECK(median_filter_update(&f, 3) == 2, "窗口满：中值为 2");
    /* 100 挤掉最旧的 1，窗口变为 {100,2,3}，排序后 2,3,100 */
    CHECK(median_filter_update(&f, 100) == 3, "尖峰滑入后中值仍是 3");
}

static void test_median_constant_is_stable(void)
{
    median_filter_t f;
    printf("test_median_constant_is_stable\n");
    median_filter_init(&f, 4);
    for (int i = 0; i < 20; i++) {
        median_filter_update(&f, 512);
    }
    CHECK(median_filter_update(&f, 512) == 512, "恒定输入应输出恒定值");
}

static void test_median_reset_clears_state(void)
{
    median_filter_t f;
    printf("test_median_reset_clears_state\n");
    median_filter_init(&f, 4);
    median_filter_update(&f, 500);
    median_filter_update(&f, 600);
    median_filter_reset(&f);
    CHECK(median_filter_update(&f, 7) == 7, "复位后第一个样本原样返回");
}

static void test_median_null_is_safe(void)
{
    printf("test_median_null_is_safe\n");
    CHECK(median_filter_update(NULL, 42) == 42, "NULL 指针不应崩溃");
    median_filter_reset(NULL); /* 不应崩溃 */
    CHECK(1, "median_filter_reset(NULL) 安全返回");
}

int main(void)
{
    test_init_rejects_bad_window();
    test_first_sample_passes_through();
    test_constant_input_is_stable();
    test_moving_average_value();
    test_spike_is_smoothed();
    test_reset_clears_state();
    test_null_is_safe();

    /* --- 中值滤波器 --- */
    test_median_init_rejects_bad_window();
    test_median_first_sample_passes_through();
    test_median_odd_window();
    test_median_even_window();
    test_median_removes_spike();
    test_median_slides();
    test_median_constant_is_stable();
    test_median_reset_clears_state();
    test_median_null_is_safe();

    printf("\n----------------------------------------\n");
    printf("共 %d 项检查，失败 %d 项\n", g_total, g_failed);
    if (g_failed == 0) {
        printf("全部通过 ✓\n");
        return 0;   /* 退出码 0 = 成功 = CI 变绿 */
    }
    printf("有失败项 ✗\n");
    return 1;       /* 退出码非 0 = 失败 = CI 变红 */
}
