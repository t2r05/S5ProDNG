// s5conv_main.cpp - headless CLI for the superccd S3/S5 Pro RAF -> DNG core
// Replaces the Qt GUI entry point. Same conversion pipeline, no Qt dependency.
//
// Usage: s5conv <input.raf> <output.dng-base> [--12mp-linear] [--start=X]
//                [--delay=X] [--smoothness=X] [--r-luma-nr=X] [--planes]
#include "SuperCCDProcessor.h"
#include "DngWriter.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static void printUsage() {
    fprintf(stderr,
        "s5conv - Fujifilm S3/S5 Pro SuperCCD SR RAF to DNG converter (headless)\n"
        "Usage: s5conv <input.raf> <output.dng-base> [options]\n"
        "Options:\n"
        "  --6mp-cfa        merged 6MP CFA DNG (default, highlight-safe)\n"
        "  --12mp-linear    experimental 12MP linear RGB DNG\n"
        "  --planes         also export separate S/R plane DNGs\n"
        "  --start=X        S->R merge start (0..1, default 0.75)\n"
        "  --delay=X        transition width (0.01..1, default 0.20)\n"
        "  --smoothness=X   transition smoothness (0..1, default 0.65)\n"
        "  --r-luma-nr=X    R-plane luma noise reduction (0..1, default 0)\n");
}

int main(int argc, char **argv) {
#ifdef _WIN32
    // UTF-8 arguments on Windows console
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc < 3) { printUsage(); return 64; }

    const char *inPath = argv[1];
    std::string outBase = argv[2];
    // merged output is written to the exact path given; append .dng when missing
    bool hasDngExt = outBase.size() > 4 &&
        (outBase.compare(outBase.size() - 4, 4, ".dng") == 0 ||
         outBase.compare(outBase.size() - 4, 4, ".DNG") == 0);
    if (!hasDngExt) outBase += ".dng";
    // process() uses this as the merged-file name directly (plane exports add suffixes)

    ConversionSettings settings;
    bool exportPlanes = false;
    std::string fujiJpegPath;
    int fujiMax = 1600;
    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        auto val = [&](const char *prefix) -> const char * {
            size_t n = strlen(prefix);
            return a.compare(0, n, prefix) == 0 ? a.c_str() + n : nullptr;
        };
        if (a == "--6mp" || a == "--6mp-cfa") settings.exportMode = ExportMode::RawCfa6MP;
        else if (a == "--12mp-linear") settings.exportMode = ExportMode::Linear12MPExperimental;
        else if (a == "--planes") exportPlanes = true;
        else if (const char *v = val("--start=")) settings.rTransitionStart = std::clamp(std::atof(v), 0.0, 1.0);
        else if (const char *v = val("--delay=")) settings.rTransitionDelay = std::clamp(std::atof(v), 0.01, 1.0);
        else if (const char *v = val("--smoothness=")) settings.rTransitionSmoothness = std::clamp(std::atof(v), 0.0, 1.0);
        else if (const char *v = val("--r-luma-nr=")) settings.rLumaNoiseReduction = std::clamp(std::atof(v), 0.0, 1.0);
        else if (const char *v = val("--fuji-jpeg=")) fujiJpegPath = v;
        else if (const char *v = val("--fuji-max=")) fujiMax = std::atoi(v);
        else { fprintf(stderr, "Unknown option: %s\n", argv[i]); printUsage(); return 64; }
    }
    settings.exportPlaneImages = exportPlanes;

    SuperCCDProcessor processor;
    QString error;
    QString in = QString::fromLocal8Bit(inPath);
    QString out = QString::fromLocal8Bit(outBase.c_str());

    fprintf(stderr, "s5conv: %s -> %s (merged; --planes adds *_s_pixels/_r_pixels)\n", inPath, outBase.c_str());
    if (!processor.process(in, out, settings, error)) {
        fprintf(stderr, "Conversion FAILED: %s\n", error.toStdString().c_str());
        return 2;
    }
    fprintf(stderr, "Conversion OK\n");
    if (!fujiJpegPath.empty()) {
        QString fujiErr;
        if (processor.renderLibRawJpeg(in, QString::fromLocal8Bit(fujiJpegPath.c_str()), fujiMax, fujiErr)) {
            fprintf(stderr, "Fuji color JPEG: %s\n", fujiJpegPath.c_str());
        } else {
            fprintf(stderr, "Fuji color JPEG FAILED: %s\n", fujiErr.toStdString().c_str());
            return 3;
        }
    }
    return 0;
}
