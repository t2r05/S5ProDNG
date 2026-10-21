// s5_jni.cpp - JNI bridge: Android app -> headless superccd core
#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdio>
#include <string>

#include "SuperCCDProcessor.h"

#define LOG_TAG "s5dng"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_s5pro_converter_MainActivity_nativeVersion(JNIEnv *env, jobject /*thiz*/) {
    return env->NewStringUTF("s5dng 1.1 (superccd core, S3/S5 Pro RAF -> DNG, batch+thumb+exif)");
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

// Extracts the embedded JPEG preview, downscales to <=512px, saves as JPEG.
// Returns true on success.
JNIEXPORT jboolean JNICALL
Java_com_s5pro_converter_MainActivity_nativeExtractThumbnail(JNIEnv *env, jobject /*thiz*/,
                                                             jstring jIn, jstring jOutJpeg) {
    const char *in = env->GetStringUTFChars(jIn, nullptr);
    const char *out = env->GetStringUTFChars(jOutJpeg, nullptr);
    QString error;
    QImage thumb;
    bool ok = SuperCCDProcessor::extractEmbeddedThumbnail(QString::fromLocal8Bit(in), thumb, &error);
    if (ok && !thumb.isNull()) {
        if (thumb.width() > 512 || thumb.height() > 512) {
            thumb = thumb.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        ok = thumb.save(QString::fromLocal8Bit(out), "jpg", 85);
        if (!ok) error = QStringLiteral("thumbnail jpeg encode failed");
    } else if (ok) {
        ok = false;
        error = QStringLiteral("empty thumbnail");
    }
    env->ReleaseStringUTFChars(jIn, in);
    env->ReleaseStringUTFChars(jOutJpeg, out);
    if (!ok) LOGE("thumb failed: %s", error.toStdString().c_str());
    return ok ? JNI_TRUE : JNI_FALSE;
}

// Reads camera/shot metadata, returns a pretty JSON string (empty on failure).
JNIEXPORT jstring JNICALL
Java_com_s5pro_converter_MainActivity_nativeReadMetadata(JNIEnv *env, jobject /*thiz*/,
                                                         jstring jIn) {
    const char *in = env->GetStringUTFChars(jIn, nullptr);
    QString error;
    SuperCCDMetadata md;
    bool ok = SuperCCDProcessor::readMetadata(QString::fromLocal8Bit(in), md, &error);
    env->ReleaseStringUTFChars(jIn, in);
    if (!ok) {
        LOGE("meta failed: %s", error.toStdString().c_str());
        return env->NewStringUTF("");
    }
    char buf[1024];
    std::string j = "{";
    auto esc = [](const std::string &s) {
        std::string r;
        for (char c : s) { if (c == '"') r += "\\\""; else if (c == '\\') r += "\\\\"; else r += c; }
        return r;
    };
    auto sput = [&](const char *k, const QString &v, bool last = false) {
        j += "\""; j += k; j += "\":\""; j += esc(v.toStdString()); j += "\""; if (!last) j += ",";
    };
    auto nput = [&](const char *k, const std::string &v, bool last = false) {
        j += "\""; j += k; j += "\":"; j += v; if (!last) j += ",";
    };
    sput("make", md.make); sput("model", md.model); sput("lens", md.lensModel);
    sput("software", md.software); sput("dateTime", md.dateTime);
    snprintf(buf, sizeof(buf), "%d", md.iso);            nput("iso", buf);
    snprintf(buf, sizeof(buf), "%.6g", md.shutter);      nput("shutter_s", buf);
    snprintf(buf, sizeof(buf), "%.2g", md.aperture);     nput("aperture", buf);
    snprintf(buf, sizeof(buf), "%.3g", md.focalLength);  nput("focal_mm", buf);
    snprintf(buf, sizeof(buf), "%u", md.whiteLevel);     nput("whiteLevel", buf);
    snprintf(buf, sizeof(buf), "%.1f,%.1f,%.1f,%.1f",
             md.blackLevels[0], md.blackLevels[1], md.blackLevels[2], md.blackLevels[3]);
    nput("blackLevels", buf);
    snprintf(buf, sizeof(buf), "%.4f,1,%.4f", md.asShotNeutral[0], md.asShotNeutral[2]);
    nput("asShotNeutral", buf);
    snprintf(buf, sizeof(buf), "%d", md.fujiWidth);      nput("fujiWidth", buf);
    snprintf(buf, sizeof(buf), "%d", md.flip);           nput("flip", buf);
    snprintf(buf, sizeof(buf), "%zu", md.curve.size());  nput("curveEntries", buf, true);
    j += "}";
    return env->NewStringUTF(j.c_str());
}

} // extern "C"
