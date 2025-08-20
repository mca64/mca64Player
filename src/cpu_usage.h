/* [1] cpu_usage.h - Header for CPU usage averaging functions. */
#pragma once

/* [2] Add a new CPU usage sample (percentage, 0-100) */
void cpu_usage_add_sample(float usage);

/* [3] Get the average CPU usage over the last N samples */
float cpu_usage_get_avg(void);
