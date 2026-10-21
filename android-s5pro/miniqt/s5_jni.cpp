// s5_jni.cpp - JNI bridge: Android app -> headless superccd core
#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <string>

#include "SuperCCDProcessor.h"

#define LOG_TAG "s5dng"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_s5pro_converter_MainActivity_nativeVersion(JNIEnv *env, jobject /*thiz*/) {
    return env->NewStringUTF("s5dng 1.0 (superccd core, S3/S5 Pro RAF -> DNG)");
}

// Returns null on success, or an error message string.
JNIEXPORT jstring JNICALL
Java_com_s5pro_converter_MainActivity_nativeConvert(JNIEnv *env, jobject /*thiz*/,
                                                    jstring jIn, jstring jOut,
                                                    jdouble start, jdouble delay,
                                                    jdouble smoothness) {
    const char *in = env->GetStringUTFChars(jIn, nullptr);
    const char *out = env->GetStringUTFChars(jOut, nullptr);
    if (!in || !out) {
        if (in) env->ReleaseStringUTFChars(jIn, in);
        if (out) env->ReleaseStringUTFChars(jOut, out);
        return env->NewStringUTF("bad arguments");
    }
    LOGI("convert %s -> %s", in, out);

    ConversionSettings settings;
    settings.exportMode = ExportMode::RawCfa6MP;
    if (start > 0) settings.rTransitionStart = start;
    if (delay > 0) settings.rTransitionDelay = delay;
    if (smoothness >= 0) settings.rTransitionSmoothness = smoothness;

    SuperCCDProcessor processor;
    QString error;
    bool ok = processor.process(QString::fromLocal8Bit(in),
                                QString::fromLocal8Bit(out),
                                settings,
                                error);

    env->ReleaseStringUTFChars(jIn, in);
    env->ReleaseStringUTFChars(jOut, out);

    if (!ok) {
        std::string msg = error.toStdString();
        LOGE("convert failed: %s", msg.c_str());
        return env->NewStringUTF(msg.c_str());
    }
    LOGI("convert OK");
    return nullptr;
}

} // extern "C"
