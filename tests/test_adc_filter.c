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

int main(void)
{
    test_init_rejects_bad_window();
    test_first_sample_passes_through();
    test_constant_input_is_stable();
    test_moving_average_value();
    test_spike_is_smoothed();
    test_reset_clears_state();
    test_null_is_safe();

    printf("\n----------------------------------------\n");
    printf("共 %d 项检查，失败 %d 项\n", g_total, g_failed);
    if (g_failed == 0) {
        printf("全部通过 ✓\n");
        return 0;   /* 退出码 0 = 成功 = CI 变绿 */
    }
    printf("有失败项 ✗\n");
    return 1;       /* 退出码非 0 = 失败 = CI 变红 */
}
