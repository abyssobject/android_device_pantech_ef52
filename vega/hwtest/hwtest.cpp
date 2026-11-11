/*
 * ef52_hwtest: silent hardware checks for the VEGA Iron (run as root).
 *   ef52_hwtest sensors [seconds]  every sensor enabled, events counted, last values
 *   ef52_hwtest mic [ms]           records the built-in mic (AAudio), RMS/peak level;
 *                                  nothing is played
 */
#include <aaudio/AAudio.h>
#include <android/looper.h>
#include <android/sensor.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vector>

static int64_t nowMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

static int sensors(int seconds) {
    ASensorManager* sm = ASensorManager_getInstanceForPackage("ef52_hwtest");
    ASensorList list;
    int n = ASensorManager_getSensorList(sm, &list);
    printf("sensors: %d\n", n);
    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    int fails = 0;
    for (int i = 0; i < n; i++) {
        const ASensor* s = list[i];
        int mode = ASensor_getReportingMode(s);
        printf("[%2d] %-34.34s type=%-3d vendor=%-12.12s mode=%d ", i, ASensor_getName(s),
               ASensor_getType(s), ASensor_getVendor(s), mode);
        if (mode == AREPORTING_MODE_ONE_SHOT || mode == AREPORTING_MODE_SPECIAL_TRIGGER) {
            printf("(one-shot/trigger: not enabled)\n");
            continue;
        }
        ASensorEventQueue* q = ASensorManager_createEventQueue(sm, looper, 1, nullptr, nullptr);
        int minDelay = ASensor_getMinDelay(s);
        int period = minDelay > 20000 ? minDelay : 20000;
        if (ASensorEventQueue_registerSensor(q, s, period, 0) < 0) {
            printf("ENABLE FAILED\n");
            fails++;
            ASensorManager_destroyEventQueue(sm, q);
            continue;
        }
        int count = 0;
        ASensorEvent last;
        memset(&last, 0, sizeof(last));
        int64_t end = nowMs() + seconds * 1000LL;
        while (nowMs() < end) {
            ALooper_pollOnce(100, nullptr, nullptr, nullptr);
            ASensorEvent ev[16];
            ssize_t got;
            while ((got = ASensorEventQueue_getEvents(q, ev, 16)) > 0) {
                count += got;
                last = ev[got - 1];
            }
        }
        ASensorEventQueue_disableSensor(q, s);
        ASensorManager_destroyEventQueue(sm, q);
        if (count == 0) {
            printf("events=0 %s\n", mode == AREPORTING_MODE_ON_CHANGE ? "(on-change: no change seen)" : "NO DATA");
            if (mode == AREPORTING_MODE_CONTINUOUS) fails++;
        } else {
            printf("events=%d (%.0f Hz) last=[%.3f %.3f %.3f]\n", count,
                   count / (double)seconds, last.data[0], last.data[1], last.data[2]);
        }
    }
    printf("RESULT: %s (%d continuous sensors without data)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}

static int mic(int ms) {
    AAudioStreamBuilder* b;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) return 2;
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 1);
    AAudioStreamBuilder_setSampleRate(b, 48000);
    AAudioStreamBuilder_setInputPreset(b, AAUDIO_INPUT_PRESET_UNPROCESSED);
    AAudioStream* st;
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &st);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK) {
        printf("mic: open failed %s\nRESULT: FAIL\n", AAudio_convertResultToText(r));
        return 1;
    }
    int rate = AAudioStream_getSampleRate(st);
    printf("mic: rate=%d ch=%d fmt=%d sharing=%d\n", rate, AAudioStream_getChannelCount(st),
           AAudioStream_getFormat(st), AAudioStream_getSharingMode(st));
    AAudioStream_requestStart(st);
    std::vector<int16_t> buf(rate / 10 * AAudioStream_getChannelCount(st));
    long long total = 0;
    double sum = 0;
    int peak = 0, zeroRuns = 0;
    int64_t end = nowMs() + ms;
    while (nowMs() < end) {
        r = AAudioStream_read(st, buf.data(), rate / 10, 500000000LL);
        if (r < 0) {
            printf("mic: read error %s\n", AAudio_convertResultToText(r));
            break;
        }
        bool allZero = true;
        for (int i = 0; i < r; i++) {
            int v = buf[i];
            sum += (double)v * v;
            if (abs(v) > peak) peak = abs(v);
            if (v) allZero = false;
        }
        if (r > 0 && allZero) zeroRuns++;
        total += r;
    }
    AAudioStream_requestStop(st);
    AAudioStream_close(st);
    double rms = total ? sqrt(sum / total) : 0;
    printf("mic: frames=%lld rms=%.1f (%.1f dBFS) peak=%d all-zero blocks=%d\n", total, rms,
           rms > 0 ? 20 * log10(rms / 32768.0) : -999.0, peak, zeroRuns);
    bool ok = total > rate * ms / 2000 && peak > 0;
    printf("RESULT: %s\n", ok ? "PASS" : "FAIL (no samples or digital silence)");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "sensors")) return sensors(argc > 2 ? atoi(argv[2]) : 2);
    if (argc >= 2 && !strcmp(argv[1], "mic")) return mic(argc > 2 ? atoi(argv[2]) : 1500);
    fprintf(stderr, "usage: %s sensors [seconds] | mic [ms]\n", argv[0]);
    return 2;
}
