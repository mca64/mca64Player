/* [1] cpu_usage.c - Simple CPU usage averaging for diagnostics. */
#include "cpu_usage.h"

#define CPU_AVG_SAMPLES 60 /* [2] Number of samples to average */
static float cpu_usage_samples[CPU_AVG_SAMPLES]; /* [3] Circular buffer for samples */
static int cpu_sample_index = 0;                 /* [4] Current index in buffer */
static int cpu_sample_count = 0;                 /* [5] Number of valid samples */

/* [6] Add a new CPU usage sample (percentage, 0-100) */
void cpu_usage_add_sample(float usage) {
    cpu_usage_samples[cpu_sample_index] = usage;
    cpu_sample_index = (cpu_sample_index + 1) % CPU_AVG_SAMPLES;
    if (cpu_sample_count < CPU_AVG_SAMPLES) cpu_sample_count++;
}

/* [7] Get the average CPU usage over the last N samples */
float cpu_usage_get_avg(void) {
    if (cpu_sample_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < cpu_sample_count; i++) sum += cpu_usage_samples[i];
    return sum / cpu_sample_count;
}
