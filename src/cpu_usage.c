#include "cpu_usage.h"
#define CPU_AVG_SAMPLES 60
static float cpu_usage_samples[CPU_AVG_SAMPLES];
static int cpu_sample_index = 0;
static int cpu_sample_count = 0;

void cpu_usage_add_sample(float usage) {
    cpu_usage_samples[cpu_sample_index] = usage;
    cpu_sample_index = (cpu_sample_index + 1) % CPU_AVG_SAMPLES;
    if (cpu_sample_count < CPU_AVG_SAMPLES) cpu_sample_count++;
}
float cpu_usage_get_avg(void) {
    if (cpu_sample_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < cpu_sample_count; i++) sum += cpu_usage_samples[i];
    return sum / cpu_sample_count;
}
