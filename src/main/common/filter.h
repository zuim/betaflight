/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// Don't use it on F1 and F3 to lower RAM usage
// FIR/Denoise filter can be cleaned up in the future as it is rarely used and used to be experimental
#if (defined(STM32F1) || defined(STM32F3))
#define MAX_FIR_DENOISE_WINDOW_SIZE 1
#else
#define MAX_FIR_DENOISE_WINDOW_SIZE 120
#endif

#define MAX_LMA_WINDOW_SIZE 12

struct filter_s;
typedef struct filter_s filter_t;

typedef struct pt1Filter_s {
    float state;
    float k;
} pt1Filter_t;

typedef struct slewFilter_s {
    float state;
    float slewLimit;
    float threshold;
} slewFilter_t;

/* this holds the data required to update samples thru a filter */
typedef struct biquadFilter_s {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} biquadFilter_t;

#define BIQUAD_LPF_ORDER_MAX 6

typedef struct biquadFilterCascade_s {
    int sections;
    biquadFilter_t biquad[(BIQUAD_LPF_ORDER_MAX + 1) / 2];   // each section is of second order
} biquadFilterCascade_t;

typedef struct firFilterDenoise_s {
    int filledCount;
    int targetCount;
    int index;
    float movingSum;
    float state[MAX_FIR_DENOISE_WINDOW_SIZE];
} firFilterDenoise_t;

typedef struct fastKalman_s {
    float k;       // "kalman" gain
    float x;       // state
    float lastX;   // previous state
} fastKalman_t;

typedef struct laggedMovingAverage_s {
    uint16_t movingWindowIndex;
    uint16_t windowSize;
    float weight;
    float movingSum;
    float buf[MAX_LMA_WINDOW_SIZE];
} laggedMovingAverage_t;

typedef enum {
    FILTER_PT1 = 0,
    FILTER_BIQUAD,
    FILTER_FIR,
    FILTER_BUTTERWORTH,
    FILTER_BIQUAD_RC_FIR2,
    FILTER_FAST_KALMAN
} lowpassFilterType_e;

typedef enum {
    FILTER_LPF,    // 2nd order Butterworth section
    FILTER_NOTCH,
    FILTER_BPF,
    FILTER_LPF1,   // 1st order Butterworth section
} biquadFilterType_e;

typedef struct firFilter_s {
    float *buf;
    const float *coeffs;
    float movingSum;
    uint8_t index;
    uint8_t count;
    uint8_t bufLength;
    uint8_t coeffsLength;
} firFilter_t;

typedef float (*filterApplyFnPtr)(filter_t *filter, float input);

float nullFilterApply(filter_t *filter, float input);



void biquadFilterInitLPF(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate);
void biquadFilterInit(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate, float Q, biquadFilterType_e filterType);
void biquadFilterUpdate(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate, float Q, biquadFilterType_e filterType);
int biquadFilterLpfCascadeInit(biquadFilter_t *sections, int order, float filterFreq, uint32_t refreshRate);

float biquadFilterApplyDF1(biquadFilter_t *filter, float input);
float biquadFilterApply(biquadFilter_t *filter, float input);
float biquadCascadeFilterApply(biquadFilterCascade_t *filter, float input);
float filterGetNotchQ(float centerFreq, float cutoffFreq);

void biquadRCFIR2FilterInit(biquadFilter_t *filter, float k);

void fastKalmanInit(fastKalman_t *filter, float k);
float fastKalmanUpdate(fastKalman_t *filter, float input);

void lmaSmoothingInit(laggedMovingAverage_t *filter, uint8_t windowSize, float weight);
float lmaSmoothingUpdate(laggedMovingAverage_t *filter, float input);

float pt1FilterGain(uint16_t f_cut, float dT);
void pt1FilterInit(pt1Filter_t *filter, float k);
float pt1FilterApply(pt1Filter_t *filter, float input);

void slewFilterInit(slewFilter_t *filter, float slewLimit, float threshold);
float slewFilterApply(slewFilter_t *filter, float input);

void firFilterInit(firFilter_t *filter, float *buf, uint8_t bufLength, const float *coeffs);
void firFilterInit2(firFilter_t *filter, float *buf, uint8_t bufLength, const float *coeffs, uint8_t coeffsLength);
void firFilterUpdate(firFilter_t *filter, float input);
void firFilterUpdateAverage(firFilter_t *filter, float input);
float firFilterApply(const firFilter_t *filter);
float firFilterUpdateAndApply(firFilter_t *filter, float input);
float firFilterCalcPartialAverage(const firFilter_t *filter, uint8_t count);
float firFilterCalcMovingAverage(const firFilter_t *filter);
float firFilterLastInput(const firFilter_t *filter);

#if defined(USE_FIR_FILTER_DENOISE)
void firFilterDenoiseInit(firFilterDenoise_t *filter, uint8_t gyroSoftLpfHz, uint16_t targetLooptime);
float firFilterDenoiseUpdate(firFilterDenoise_t *filter, float input);
#endif
