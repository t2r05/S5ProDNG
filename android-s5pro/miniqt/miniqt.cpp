// miniqt.cpp - implementation of the mini-Qt shim
#include "miniqt.h"

#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ------------------------------------------------------------------ utf8
static void Utf8ToU16(const char *s, int len, std::u16string &out) {
    out.clear();
    if (!s) return;
    int i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out.push_back(char16_t(c)); i += 1; }
        else if ((c >> 5) == 0x6 && i + 1 < len) {
            out.push_back(char16_t(((c & 0x1F) << 6) | (s[i + 1] & 0x3F))); i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < len) {
            out.push_back(char16_t(((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F))); i += 3;
        } else if ((c >> 3) == 0x1E && i + 3 < len) {
            uint32_t cp = ((c & 0x07) << 18) | ((uint32_t(s[i + 1]) & 0x3F) << 12) | ((uint32_t(s[i + 2]) & 0x3F) << 6) | (s[i + 3] & 0x3F);
            cp -= 0x10000;
            out.push_back(char16_t(0xD800 + (cp >> 10)));
            out.push_back(char16_t(0xDC00 + (cp & 0x3FF)));
            i += 4;
        } else { out.push_back(char16_t(c)); i += 1; }
    }
}

static void U16ToUtf8(const std::u16string &in, std::string &out) {
    out.clear();
    for (size_t i = 0; i < in.size(); ++i) {
        uint32_t cp = in[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < in.size()) {
            uint32_t lo = in[i + 1];
            if (lo >= 0xDC00 && lo <= 0xDFFF) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); ++i; }
        }
        if (cp < 0x80) out.push_back(char(cp));
        else if (cp < 0x800) { out.push_back(char(0xC0 | (cp >> 6))); out.push_back(char(0x80 | (cp & 0x3F))); }
        else if (cp < 0x10000) { out.push_back(char(0xE0 | (cp >> 12))); out.push_back(char(0x80 | ((cp >> 6) & 0x3F))); out.push_back(char(0x80 | (cp & 0x3F))); }
        else { out.push_back(char(0xF0 | (cp >> 18))); out.push_back(char(0x80 | ((cp >> 12) & 0x3F))); out.push_back(char(0x80 | ((cp >> 6) & 0x3F))); out.push_back(char(0x80 | (cp & 0x3F))); }
    }
}

void QString::fromUtf8Impl(const char *s, int len) { Utf8ToU16(s, len, m_s); }
void QString::utf8FromU16(const std::u16string &in, QByteArray &out) {
    std::string tmp; U16ToUtf8(in, tmp); out = QByteArray(tmp.data(), (int)tmp.size());
}

QString QString::join(const QStringList &list, const QString &sep) { return QStringJoin(list, sep); }

// ------------------------------------------------------------------ QImage
void QImage::init(int w, int h, Format f) {
    m_w = w; m_h = h; m_fmt = f;
    int bpp = 0;
    switch (f) {
        case Format_RGB32: bpp = 4; break;
        case Format_RGB888: bpp = 3; break;
        case Format_RGBX64: bpp = 8; break;
        case Format_Grayscale8: bpp = 1; break;
        default: bpp = 0; break;
    }
    m_stride = (int)((size_t(w) * bpp + 3) & ~size_t(3));
    m_data.assign(size_t(m_stride) * size_t(h > 0 ? h : 0), 0);
}

void QImage::fill(quint32 pixelValue) {
    if (m_fmt == Format_RGB32) {
        for (int y = 0; y < m_h; ++y) {
            quint32 *line = reinterpret_cast<quint32 *>(scanLine(y));
            for (int x = 0; x < m_w; ++x) line[x] = pixelValue;
        }
    } else {
        std::fill(m_data.begin(), m_data.end(), 0);
    }
}

QImage QImage::convertToFormat(Format f) const {
    if (m_fmt == f || isNull()) return *this;
    QImage out(m_w, m_h, f);
    if (m_fmt == Format_RGB32 && f == Format_RGB888) {
        for (int y = 0; y < m_h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(scanLine(y));
            uchar *dst = out.scanLine(y);
            for (int x = 0; x < m_w; ++x) {
                quint32 p = src[x];
                dst[x * 3 + 0] = uchar((p >> 16) & 0xFF);
                dst[x * 3 + 1] = uchar((p >> 8) & 0xFF);
                dst[x * 3 + 2] = uchar(p & 0xFF);
            }
        }
    } else if (m_fmt == Format_RGB888 && f == Format_RGB32) {
        for (int y = 0; y < m_h; ++y) {
            const uchar *src = scanLine(y);
            quint32 *dst = reinterpret_cast<quint32 *>(out.scanLine(y));
            for (int x = 0; x < m_w; ++x) {
                dst[x] = qRgb(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            }
        }
    } else if (m_fmt == Format_RGB32 && f == Format_RGBX64) {
        for (int y = 0; y < m_h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(scanLine(y));
            quint16 *dst = reinterpret_cast<quint16 *>(out.scanLine(y));
            for (int x = 0; x < m_w; ++x) {
                quint32 p = src[x];
                quint16 r = quint16(((p >> 16) & 0xFF) * 257);
                quint16 g = quint16(((p >> 8) & 0xFF) * 257);
                quint16 b = quint16((p & 0xFF) * 257);
                dst[x * 4 + 0] = r; dst[x * 4 + 1] = g; dst[x * 4 + 2] = b; dst[x * 4 + 3] = 0xFFFF;
            }
        }
    } else if (m_fmt == Format_RGBX64 && f == Format_RGB32) {
        for (int y = 0; y < m_h; ++y) {
            const quint16 *src = reinterpret_cast<const quint16 *>(scanLine(y));
            quint32 *dst = reinterpret_cast<quint32 *>(out.scanLine(y));
            for (int x = 0; x < m_w; ++x) {
                dst[x] = qRgb(src[x * 4] >> 8, src[x * 4 + 1] >> 8, src[x * 4 + 2] >> 8);
            }
        }
    } else {
        // Fallback: zero-filled target (unsupported pair)
        return out;
    }
    return out;
}

QImage QImage::scaled(int w, int h, Qt::AspectRatioMode aspect, Qt::TransformationMode mode) const {
    if (isNull() || w <= 0 || h <= 0) return QImage();
    int tw = w, th = h;
    if (aspect == Qt::KeepAspectRatio) {
        double sx = double(w) / m_w, sy = double(h) / m_h;
        double s = sx < sy ? sx : sy;
        tw = std::max(1, int(m_w * s));
        th = std::max(1, int(m_h * s));
    }
    QImage out(tw, th, m_fmt);
    const int chans = (m_fmt == Format_RGB888) ? 3 : 4;
    // Area-average downsampling (good enough for preview); nearest for tiny upscales
    for (int y = 0; y < th; ++y) {
        int sy0 = int(qint64(y) * m_h / th);
        int sy1 = int(qint64(y + 1) * m_h / th);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        if (sy1 > m_h) sy1 = m_h;
        uchar *dst = out.scanLine(y);
        for (int x = 0; x < tw; ++x) {
            int sx0 = int(qint64(x) * m_w / tw);
            int sx1 = int(qint64(x + 1) * m_w / tw);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            if (sx1 > m_w) sx1 = m_w;
            quint64 acc[4] = {0, 0, 0, 0};
            quint64 count = 0;
            for (int sy = sy0; sy < sy1; ++sy) {
                const uchar *src = scanLine(sy);
                for (int sx = sx0; sx < sx1; ++sx) {
                    for (int c = 0; c < chans; ++c) acc[c] += src[size_t(sx) * chans + c];
                    ++count;
                }
            }
            for (int c = 0; c < chans; ++c) dst[size_t(x) * chans + c] = uchar(count ? acc[c] / count : 0);
        }
    }
    (void)mode;
    return out;
}

QImage QImage::transformed(const QTransform &t, Qt::TransformationMode mode) const {
    if (t.isIdentity() || isNull()) return *this;
    QImage out;
    int rot = t.rotation();
    if (rot == 90 || rot == 270) out = QImage(m_h, m_w, m_fmt);
    else out = QImage(m_w, m_h, m_fmt);
    const int bpp = (m_fmt == Format_RGB888) ? 3 : 4;
    for (int y = 0; y < m_h; ++y) {
        for (int x = 0; x < m_w; ++x) {
            const uchar *src = scanLine(y) + size_t(x) * bpp;
            int dx = x, dy = y;
            if (rot == 90) { dx = m_h - 1 - y; dy = x; }
            else if (rot == 180) { dx = m_w - 1 - x; dy = m_h - 1 - y; }
            else if (rot == 270) { dx = y; dy = m_w - 1 - x; }
            memcpy(out.scanLine(dy) + size_t(dx) * bpp, src, bpp);
        }
    }
    (void)mode;
    return out;
}

QImage QImage::mirrored(bool horizontal, bool vertical) const {
    if (isNull()) return *this;
    QImage out(m_w, m_h, m_fmt);
    const int bpp = (m_fmt == Format_RGB888) ? 3 : 4;
    for (int y = 0; y < m_h; ++y) {
        int sy = vertical ? m_h - 1 - y : y;
        const uchar *src = scanLine(sy);
        uchar *dst = out.scanLine(y);
        if (!horizontal) {
            memcpy(dst, src, size_t(m_w) * bpp);
        } else {
            for (int x = 0; x < m_w; ++x)
                memcpy(dst + size_t(x) * bpp, src + size_t(m_w - 1 - x) * bpp, bpp);
        }
    }
    return out;
}

QImage QImage::copy(int x, int y, int w, int h) const {
    if (isNull()) return QImage();
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0 || x >= m_w || y >= m_h) return QImage();
    if (x + w > m_w) w = m_w - x;
    if (y + h > m_h) h = m_h - y;
    QImage out(w, h, m_fmt);
    const int bpp = (m_fmt == Format_RGB888) ? 3 : 4;
    for (int row = 0; row < h; ++row)
        memcpy(out.scanLine(row), scanLine(y + row) + size_t(x) * bpp, size_t(w) * bpp);
    return out;
}

QImage QImage::fromData(const uchar *data, int len, const char *format) {
    (void)format; // embedded RAF thumbnails are JPEG
    QImage out;
    if (!data || len <= 0) return out;
    int w = 0, h = 0, comp = 0;
    stbi_uc *pix = stbi_load_from_memory(data, len, &w, &h, &comp, 0);
    if (!pix) return out;
    out.init(w, h, Format_RGB888);
    if (comp == 3) {
        memcpy(out.bits(), pix, size_t(w) * size_t(h) * 3);
    } else if (comp == 4) {
        for (int y = 0; y < h; ++y) {
            const stbi_uc *src = pix + size_t(y) * w * 4;
            uchar *dst = out.scanLine(y);
            for (int x = 0; x < w; ++x) {
                dst[x * 3] = src[x * 4];
                dst[x * 3 + 1] = src[x * 4 + 1];
                dst[x * 3 + 2] = src[x * 4 + 2];
            }
        }
    } else if (comp == 1) {
        for (int y = 0; y < h; ++y) {
            const stbi_uc *src = pix + size_t(y) * w;
            uchar *dst = out.scanLine(y);
            for (int x = 0; x < w; ++x) { dst[x * 3] = dst[x * 3 + 1] = dst[x * 3 + 2] = src[x]; }
        }
    }
    stbi_image_free(pix);
    return out;
}

bool QImage::save(const QString &path, const char *format, int quality) const {
    if (isNull()) return false;
    QByteArray u8 = path.toUtf8();
    QByteArray fmt;
    if (format && *format) fmt = QByteArray(format);
    else {
        QString p = path.toLower();
        if (p.endsWith(QString(".png"))) fmt = QByteArray("png");
        else fmt = QByteArray("jpg");
    }
    // Flatten to RGB888 for encoders
    QImage rgb = (m_fmt == Format_RGB888) ? *this : convertToFormat(Format_RGB888);
    if (fmt == QByteArray("png")) {
        return stbi_write_png(u8.constData(), m_w, m_h, 3, rgb.bits(), rgb.bytesPerLine()) != 0;
    }
    // comp trick: stb writes from tightly packed rows; feed row by row via callback
    std::vector<uchar> packed;
    if (rgb.bytesPerLine() == rgb.width() * 3) {
        packed.assign(rgb.bits(), rgb.bits() + size_t(rgb.height()) * rgb.bytesPerLine());
    } else {
        packed.reserve(size_t(m_h) * m_w * 3);
        for (int y = 0; y < m_h; ++y) {
            const uchar *line = rgb.scanLine(y);
            packed.insert(packed.end(), line, line + size_t(m_w) * 3);
        }
    }
    if (quality < 0) quality = 90;
    return stbi_write_jpg(u8.constData(), m_w, m_h, 3, packed.data(), quality) != 0;
}

// ------------------------------------------------------------------- QFile
bool QFile::open(int flags) {
    if (m_fp) { fclose(m_fp); m_fp = nullptr; }
    bool rd = (flags & QIODevice::ReadOnly) != 0;
    bool wr = (flags & QIODevice::WriteOnly) != 0;
    QByteArray u8 = m_path.toUtf8();
#ifdef _WIN32
    // Windows: use wide open to handle non-ASCII paths
    QString wide = m_path;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
    m_fp = _wfopen(w.c_str(), wr ? (rd ? L"r+b" : L"wb") : L"rb");
#else
    m_fp = fopen(u8.constData(), wr ? (rd ? "r+b" : "wb") : "rb");
#endif
    return m_fp != nullptr;
}

void QFile::close() {
    if (m_fp) { fclose(m_fp); m_fp = nullptr; }
}

QByteArray QFile::readAll() {
    if (!m_fp) return QByteArray();
    fseek(m_fp, 0, SEEK_END);
    long sz = ftell(m_fp);
    fseek(m_fp, 0, SEEK_SET);
    QByteArray out;
    if (sz > 0) {
        out.resize((int)sz);
        size_t rd = fread(out.data(), 1, size_t(sz), m_fp);
        if (size_t(rd) < size_t(sz)) out.resize((int)rd);
    }
    return out;
}

qint64 QFile::write(const void *data, qint64 len) {
    if (!m_fp) return -1;
    return qint64(fwrite(data, 1, size_t(len), m_fp));
}

qint64 QFile::seek(qint64 pos) {
    if (!m_fp) return -1;
    return fseek(m_fp, long(pos), SEEK_SET) == 0 ? pos : -1;
}

bool QFile::resize(qint64 size) {
    if (m_fp) { fflush(m_fp); }
#ifdef _WIN32
    auto toWide = [](const QString &p) {
        QByteArray u8 = p.toUtf8();
        int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
        std::wstring w(size_t(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
        return w;
    };
    HANDLE h = CreateFileW(toWide(m_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER li; li.QuadPart = size;
    BOOL ok = SetFilePointerEx(h, li, nullptr, FILE_BEGIN) && SetEndOfFile(h);
    CloseHandle(h);
    return ok != FALSE;
#else
    return truncate(m_path.toUtf8().constData(), off_t(size)) == 0;
#endif
}

bool QFile::exists(const QString &path) {
#ifdef _WIN32
    QString p = path;
    QByteArray u8 = p.toUtf8();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
    DWORD attrs = GetFileAttributesW(w.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return ::stat(path.toUtf8().constData(), &st) == 0;
#endif
}

bool QFile::remove(const QString &path) {
#ifdef _WIN32
    QByteArray u8 = path.toUtf8();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
    return DeleteFileW(w.c_str()) != 0;
#else
    return ::remove(path.toUtf8().constData()) == 0;
#endif
}

bool QFile::rename(const QString &from, const QString &to) {
#ifdef _WIN32
    auto toWide = [](const QString &p) {
        QByteArray u8 = p.toUtf8();
        int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
        std::wstring w(size_t(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
        return w;
    };
    return MoveFileExW(toWide(from).c_str(), toWide(to).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return ::rename(from.toUtf8().constData(), to.toUtf8().constData()) == 0;
#endif
}

// --------------------------------------------------------------- QFileInfo
bool QFileInfo::exists() const { return QFile::exists(m_path); }

qint64 QFileInfo::size() const {
#ifdef _WIN32
    auto toWide = [](const QString &p) {
        QByteArray u8 = p.toUtf8();
        int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
        std::wstring w(size_t(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
        return w;
    };
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(toWide(m_path).c_str(), GetFileExInfoStandard, &fad)) return 0;
    return (qint64(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
#else
    struct stat st;
    if (::stat(m_path.toUtf8().constData(), &st) != 0) return 0;
    return st.st_size;
#endif
}

QDateTime QFileInfo::lastModified() const {
#ifdef _WIN32
    auto toWide = [](const QString &p) {
        QByteArray u8 = p.toUtf8();
        int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
        std::wstring w(size_t(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
        return w;
    };
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(toWide(m_path).c_str(), GetFileExInfoStandard, &fad)) return QDateTime();
    ULARGE_INTEGER li;
    li.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    li.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    // 100ns since 1601-01-01 -> seconds since 1970-01-01
    return QDateTime::fromSecsSinceEpoch(qint64(li.QuadPart / 10000000ULL) - 11644473600LL);
#else
    struct stat st;
    if (::stat(m_path.toUtf8().constData(), &st) != 0) return QDateTime();
    return QDateTime::fromSecsSinceEpoch(qint64(st.st_mtime));
#endif
}

QString QFileInfo::suffix() const {
    QString f = m_path;
    int slash = f.lastIndexOf(QString("/"));
    if (f.lastIndexOf(QString("\\")) > slash) slash = f.lastIndexOf(QString("\\"));
    QString name = slash >= 0 ? f.mid(slash + 1) : f;
    int dot = name.lastIndexOf(QString("."));
    return dot > 0 ? name.mid(dot + 1) : QString();
}

QString QFileInfo::fileName() const {
    QString f = m_path;
    int slash = f.lastIndexOf(QString("/"));
    if (f.lastIndexOf(QString("\\")) > slash) slash = f.lastIndexOf(QString("\\"));
    return slash >= 0 ? f.mid(slash + 1) : f;
}

// -------------------------------------------------------------------- QDir
bool QDir::mkpath(const QString &sub) const {
    // Qt semantics: mkpath(abs) creates abs; mkpath(rel) creates m_path/sub;
    // mkpath("") creates m_path itself.
    QString path = sub;
    if (path.isEmpty()) {
        path = m_path;
    } else if (!m_path.isEmpty() && path.mid(1, 1) != QString(":") && path.mid(0, 1) != QString("/")) {
        path = m_path + QString("/") + sub;
    }
    if (path.isEmpty()) return false;
    if (QFile::exists(path)) return true;
    // recursive mkdir
    QString p = path;
    QString prefix;
#ifdef _WIN32
    if (p.mid(1, 1) == QString(":")) { prefix = p.mid(0, 3); p = p.mid(3); }
#else
    if (!p.isEmpty() && p.mid(0, 1) == QString("/")) { prefix = QString("/"); p = p.mid(1); }
#endif
    QStringList parts;
    while (!p.isEmpty()) {
        int slash = p.indexOf(QString("/"));
        if (slash < 0) { parts.push_back(p); break; }
        parts.push_back(p.mid(0, slash));
        p = p.mid(slash + 1);
    }
    QString cur = prefix;
    for (const QString &part : parts) {
        if (part.isEmpty()) continue;
        if (!cur.isEmpty() && cur.mid(cur.size() - 1, 1) != QString("/") && cur.mid(cur.size() - 1, 1) != QString(":")) cur += QString("/");
        cur += part;
        if (!QFile::exists(cur)) {
#ifdef _WIN32
            QByteArray u8 = cur.toUtf8();
            int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), nullptr, 0);
            std::wstring w(size_t(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, u8.constData(), u8.size(), &w[0], wlen);
            if (!CreateDirectoryW(w.c_str(), nullptr)) return false;
#else
            if (mkdir(cur.toUtf8().constData(), 0755) != 0) return false;
#endif
        }
    }
    return true;
}

bool QDir::exists() const { return QFile::exists(m_path); }

QString QDir::homePath() {
#ifdef _WIN32
    const char *hp = getenv("USERPROFILE");
    return hp ? QString(hp) : QString("C:/");
#else
    const char *hp = getenv("HOME");
    return hp ? QString(hp) : QString("/");
#endif
}

QString QDir::currentPath() {
    char buf[4096];
#ifdef _WIN32
    DWORD n = GetCurrentDirectoryA(sizeof(buf), buf);
    return n ? QString(buf) : QString("C:/");
#else
    if (getcwd(buf, sizeof(buf))) return QString(buf);
    return QString("/");
#endif
}

// ------------------------------------------------------------ QStandardPaths
QString QStandardPaths::writableLocation(int type) {
#ifdef _WIN32
    if (type == TempLocation) return QDir("C:/ProgramData/s5conv/temp").mkpath(QString()) ? QString("C:/ProgramData/s5conv/temp") : QDir::homePath();
    QDir baseDir("C:/ProgramData/s5conv");
    QString base = baseDir.exists() ? QString("C:/ProgramData/s5conv") : QString();
    if (base.isEmpty()) { baseDir.mkpath(QString()); base = QString("C:/ProgramData/s5conv"); }
    return base;
#else
    const char *home = getenv("HOME");
    QString h = home ? QString(home) : QString("/tmp");
    if (type == TempLocation) return h + QString("/tmp");
    return h + QString("/.s5conv");
#endif
}

QString QStandardPaths::findExecutable(const QString &name) {
    // Search PATH for an executable; used only for the optional exiftool path.
    if (name.isEmpty()) return QString();
#ifdef _WIN32
    std::string pathenv;
    const char *p = getenv("PATH");
    pathenv = p ? p : "";
    std::stringstream ss(pathenv);
    std::string item;
    while (std::getline(ss, item, ';')) {
        if (item.empty()) continue;
        for (const char *ext : { ".exe", ".bat", "" }) {
            std::string cand = item + "\\" + name.toStdString() + ext;
            if (GetFileAttributesA(cand.c_str()) != INVALID_FILE_ATTRIBUTES) return QString(cand.c_str());
        }
    }
    return QString();
#else
    const char *p = getenv("PATH");
    if (!p) return QString();
    std::stringstream ss(p);
    std::string item;
    while (std::getline(ss, item, ':')) {
        if (item.empty()) continue;
        std::string cand = item + "/" + name.toStdString();
        if (access(cand.c_str(), X_OK) == 0) return QString(cand.c_str());
    }
    return QString();
#endif
}

// ---------------------------------------------------------------- QProcess
void QProcess::start(const QString &program, const QStringList &args) {
    m_out.clear(); m_err.clear(); m_exitCode = -1; m_exitStatus = CrashExit;
    std::string cmd = program.toStdString();
    for (const QString &a : args) { cmd += " \""; cmd += a.toStdString(); cmd += "\""; }
#ifdef _WIN32
    FILE *pp = _popen(cmd.c_str(), "rb");
#else
    FILE *pp = popen(cmd.c_str(), "r");
#endif
    if (!pp) return;
    std::vector<char> buf(65536);
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), pp)) > 0) m_out.append(QByteArray(buf.data(), int(n)));
#ifdef _WIN32
    int rc = _pclose(pp);
#else
    int rc = pclose(pp);
#endif
    if (rc >= 0) { m_exitStatus = NormalExit; m_exitCode = rc; }
}

bool QProcess::waitForFinished(int msecs) { (void)msecs; return m_exitStatus == NormalExit; }

// ------------------------------------------------------ string impl helpers
// (fromUtf8Impl / utf8FromU16 defined above)
