// miniqt.h - Minimal Qt-compatible shim for superccd core headless builds
// Covers ONLY the API surface actually used by SuperCCDProcessor/DngWriter/
// CfaPlaneAlignment/AmazeDemosaic/PreviewColorNormalization/PreviewPixelCorrection.
// Not a general Qt replacement. UTF-16 native strings, like Qt.
#ifndef MINIQT_H
#define MINIQT_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

typedef signed char qint8;
typedef unsigned char quint8;
typedef unsigned char uchar;
typedef short qint16;
typedef unsigned short quint16;
typedef unsigned short ushort;
typedef int qint32;
typedef unsigned int quint32;
typedef unsigned int uint;
typedef long long qint64;
typedef unsigned long long quint64;
typedef unsigned long long qulonglong;
typedef long long qsizetype;
typedef double qreal;

namespace Qt {
enum CaseSensitivity { CaseInsensitive = 0, CaseSensitive = 1 };
enum AspectRatioMode { IgnoreAspectRatio = 0, KeepAspectRatio = 1, KeepAspectRatioByExpanding = 2 };
enum TransformationMode { FastTransformation = 0, SmoothTransformation = 1 };
enum GlobalColor { black = 0xFF000000u, white = 0xFFFFFFFFu, transparent = 0x00000000u };
}

#define Q_UNUSED(x) (void)(x);

typedef quint32 QRgb;
inline QRgb qRgb(int r, int g, int b) { return (0xFF000000u | (quint32(r) << 16) | (quint32(g) << 8) | quint32(b)); }

class QStringList;
class QChar {
public:
    QChar(char16_t c = 0) : c(c) {}
    char16_t c;
};

// ---------------------------------------------------------------- QByteArray
class QByteArray {
public:
    QByteArray() {}
    QByteArray(const char *s) : m_s(s ? s : "") {}
    QByteArray(const char *s, int len) : m_s(s, s ? s + len : 0) {}
    QByteArray(int n, char c) : m_s(size_t(n > 0 ? n : 0), c) {}
    char operator[](int i) const { return i >= 0 && i < size() ? m_s[size_t(i)] : char(0); }
    char &operator[](int i) { return m_s[size_t(i)]; }
    QByteArray mid(int pos, int n = -1) const {
        if (pos < 0) pos = 0; if (pos >= size()) return QByteArray();
        if (n < 0 || pos + n > size()) n = size() - pos;
        return QByteArray(m_s.data() + pos, n);
    }
    QByteArray left(int n) const { return mid(0, n); }
    QByteArray right(int n) const { int s = size(); return s <= n ? *this : mid(s - n, n); }
    const char *constData() const { return m_s.c_str(); }
    char *data() { return &m_s[0]; }
    const char *data() const { return m_s.data(); }
    int size() const { return (int)m_s.size(); }
    bool isEmpty() const { return m_s.empty(); }
    void clear() { m_s.clear(); }
    void resize(int n) { m_s.resize(size_t(n)); }
    void append(const QByteArray &o) { m_s += o.m_s; }
    void append(const char *s) { m_s += s; }
    void append(char c) { m_s += c; }
    QByteArray &operator+=(const QByteArray &o) { m_s += o.m_s; return *this; }
    QByteArray &operator+=(const char *s) { m_s += s; return *this; }
    friend inline QByteArray operator+(const QByteArray &a, const QByteArray &b) { QByteArray r; r.m_s = a.m_s + b.m_s; return r; }
    friend inline bool operator==(const QByteArray &a, const QByteArray &b) { return a.m_s == b.m_s; }
    friend inline bool operator!=(const QByteArray &a, const QByteArray &b) { return a.m_s != b.m_s; }
    const char *begin() const { return m_s.data(); }
    const char *end() const { return m_s.data() + m_s.size(); }
    const std::string &std() const { return m_s; }
private:
    std::string m_s;
};

// ------------------------------------------------------------------ QString
class QString {
public:
    QString() {}
    QString(const char *s) { if (s) fromUtf8Impl(s, (int)strlen(s)); }
    QString(const std::u16string &s) : m_s(s) {}
    QString(const char16_t *s) { if (s) { size_t n = 0; while (s[n]) ++n; m_s.assign(s, s + n); } }

    static QString fromUtf8(const char *s, int len = -1) { QString r; r.fromUtf8Impl(s, len < 0 ? (s ? (int)strlen(s) : 0) : len); return r; }
    static QString fromLatin1(const char *s, int len = -1) { return fromUtf8(s, len); }
    static QString fromLocal8Bit(const char *s, int len = -1) { return fromUtf8(s, len); }
    static QString fromStdString(const std::string &s) { return fromUtf8(s.data(), (int)s.size()); }
    static QString fromUtf16(const ushort *s, int len = -1) { QString r; if (s) { int n = len; if (n < 0) { n = 0; while (s[n]) ++n; } r.m_s.assign(s, s + n); } return r; }
    static QString fromUtf8(const QByteArray &a) { return fromUtf8(a.constData(), a.size()); }
    static QString fromLatin1(const QByteArray &a) { return fromLatin1(a.constData(), a.size()); }
    static QString fromLocal8Bit(const QByteArray &a) { return fromLocal8Bit(a.constData(), a.size()); }
    static QString number(int v) { char b[32]; snprintf(b, sizeof(b), "%d", v); return QString(b); }
    static QString number(uint v) { char b[32]; snprintf(b, sizeof(b), "%u", v); return QString(b); }
    static QString number(long long v) { char b[32]; snprintf(b, sizeof(b), "%lld", v); return QString(b); }
    static QString number(unsigned long long v) { char b[32]; snprintf(b, sizeof(b), "%llu", v); return QString(b); }
    static QString number(double v, char fmt = 'g', int prec = 6) {
        char b[64]; snprintf(b, sizeof(b), "%.*f", prec, v); (void)fmt; return QString(b);
    }

    int size() const { return (int)m_s.size(); }
    bool isEmpty() const { return m_s.empty(); }
    bool isNull() const { return m_s.empty(); }
    void clear() { m_s.clear(); }

    const ushort *utf16() const { return reinterpret_cast<const ushort *>(m_s.data()); }
    std::u16string &raw() { return m_s; }
    const std::u16string &raw() const { return m_s; }

    std::string toStdString() const { return toUtf8().std(); }
    QByteArray toUtf8() const { QByteArray out; utf8FromU16(m_s, out); return out; }
    QByteArray toLatin1() const {
        QByteArray out; out.resize((int)m_s.size());
        for (size_t i = 0; i < m_s.size(); ++i) ((char *)out.data())[i] = (char)(m_s[i] & 0xFF);
        return out;
    }
    QByteArray toLocal8Bit() const { return toUtf8(); }

    QString &append(const QString &o) { m_s += o.m_s; return *this; }
    QString &append(const char *s) { if (s) m_s += QString(s).m_s; return *this; }
    QString &operator+=(const QString &o) { m_s += o.m_s; return *this; }
    QString &operator+=(const char *s) { return append(s); }
    friend inline QString operator+(const QString &a, const QString &b) { QString r; r.m_s = a.m_s + b.m_s; return r; }
    friend inline QString operator+(const QString &a, const char *b) { QString r; r.m_s = a.m_s + QString(b).m_s; return r; }
    friend inline QString operator+(const char *a, const QString &b) { QString r; r.m_s = QString(a).m_s + b.m_s; return r; }
    friend inline bool operator==(const QString &a, const QString &b) { return a.m_s == b.m_s; }
    friend inline bool operator!=(const QString &a, const QString &b) { return a.m_s != b.m_s; }
    friend inline bool operator<(const QString &a, const QString &b) { return a.m_s < b.m_s; }

    QString toLower() const { QString r = *this; for (auto &c : r.m_s) { if (c >= u'A' && c <= u'Z') c = char16_t(c + 32); } return r; }
    QString toUpper() const { QString r = *this; for (auto &c : r.m_s) { if (c >= u'a' && c <= u'z') c = char16_t(c - 32); } return r; }
    QString trimmed() const {
        size_t b = 0, e = m_s.size();
        auto isws = [](char16_t c) { return c == u' ' || c == u'\t' || c == u'\n' || c == u'\r'; };
        while (b < e && isws(m_s[b])) ++b;
        while (e > b && isws(m_s[e - 1])) --e;
        return QString(m_s.substr(b, e - b));
    }
    QString mid(int pos, int n = -1) const {
        if (pos < 0) pos = 0; if (pos >= (int)m_s.size()) return QString();
        if (n < 0 || pos + n > (int)m_s.size()) n = (int)m_s.size() - pos;
        return QString(m_s.substr(size_t(pos), size_t(n)));
    }
    QString left(int n) const { return mid(0, n); }
    QString right(int n) const { int s = (int)m_s.size(); return s <= n ? *this : mid(s - n, n); }
    void chop(int n) { if (n >= (int)m_s.size()) m_s.clear(); else m_s.resize(m_s.size() - size_t(n)); }
    bool endsWith(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        if (s.size() > size()) return false;
        QString t = mid(size() - s.size(), s.size());
        return cs == Qt::CaseInsensitive ? t.toLower() == s.toLower() : t == s;
    }
    bool startsWith(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        if (s.size() > size()) return false;
        QString t = mid(0, s.size());
        return cs == Qt::CaseInsensitive ? t.toLower() == s.toLower() : t == s;
    }
    bool contains(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        return indexOf(s, 0, cs) >= 0;
    }
    int indexOf(const QString &s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        if (s.isEmpty()) return from <= size() ? from : -1;
        QString a = (cs == Qt::CaseInsensitive) ? toLower() : *this;
        QString b = (cs == Qt::CaseInsensitive) ? s.toLower() : s;
        if (b.size() > a.size()) return -1;
        for (int i = from; i + (int)b.size() <= a.size(); ++i) {
            int j = 0;
            while (j < (int)b.size() && a.m_s[size_t(i + j)] == b.m_s[size_t(j)]) ++j;
            if (j == (int)b.size()) return i;
        }
        return -1;
    }
    int lastIndexOf(const QString &s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        if (s.isEmpty()) return -1;
        QString a = (cs == Qt::CaseInsensitive) ? toLower() : *this;
        QString b = (cs == Qt::CaseInsensitive) ? s.toLower() : s;
        if (b.size() > a.size()) return -1;
        if (from < 0 || from > a.size() - (int)b.size()) from = a.size() - (int)b.size();
        for (int i = from; i >= 0; --i) {
            int j = 0;
            while (j < (int)b.size() && a.m_s[size_t(i + j)] == b.m_s[size_t(j)]) ++j;
            if (j == (int)b.size()) return i;
        }
        return -1;
    }
    int compare(const QString &o, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
        QString a = (cs == Qt::CaseInsensitive) ? toLower() : *this;
        QString b = (cs == Qt::CaseInsensitive) ? o.toLower() : o;
        if (a.m_s < b.m_s) return -1;
        if (a.m_s > b.m_s) return 1;
        return 0;
    }

    // ---- arg(): Qt-style %1/%2 substitution (lowest unused index first) ----
    QString arg(const QString &v) const { return argImpl([v](int) { return v; }); }
    QString arg(int v) const { return argQString(number(v)); }
    QString arg(uint v) const { return argQString(number(v)); }
    QString arg(long long v) const { return argQString(number(v)); }
    QString arg(unsigned long long v) const { return argQString(number(v)); }
    QString arg(long v) const { return argQString(number((long long)v)); }
    QString arg(double v, int fieldWidth = 0, char fmt = 'g', int prec = -1) const {
        (void)fieldWidth;
        char b[96];
        if (prec >= 0) snprintf(b, sizeof(b), "%.*f", prec, v);
        else if (fmt == 'f') snprintf(b, sizeof(b), "%f", v);
        else snprintf(b, sizeof(b), "%g", v);
        return argQString(QString(b));
    }
    QString arg(char v) const { char b[2] = { v, 0 }; return argQString(QString(b)); }
    QString arg(QChar v) const { return argQString(QString(std::u16string(1, v.c))); }

    static QString join(const QStringList &list, const QString &sep);

private:
    template <typename F> QString argImpl(F &&f) const { return argQString(f(lowestPlaceholder())); }
    QString argQString(const QString &v) const {
        int idx = lowestPlaceholder();
        if (idx <= 0) return *this;
        QString tag = QString::number(idx);
        QString self = *this;
        int p = self.indexOf(tag);
        if (p < 0) return self;
        QString r = self.mid(0, p) + v + self.mid(p + (int)tag.size());
        return r;
    }
    int lowestPlaceholder() const {
        int best = INT32_MAX; int bestPos = INT32_MAX;
        for (int n = 1; n <= 99; ++n) {
            QString tag = QString::number(n);
            int p = indexOf(tag, 0, Qt::CaseSensitive);
            bool hit = (p >= 0);
            // ensure the hit is a standalone placeholder (%10 vs %1)
            if (hit) {
                int after = p + (int)tag.size();
                if (after < size() && m_s[size_t(after)] >= u'0' && m_s[size_t(after)] <= u'9') hit = false;
                if (p > 0 && m_s[size_t(p - 1)] == u'%') { /* still fine: %11 handled by longer tag first */ }
            }
            if (hit && (n < best || (n == best && p < bestPos))) { /* prefer smallest n */ }
            if (hit && n < best) { best = n; bestPos = p; }
        }
        return best == INT32_MAX ? -1 : best;
    }
    void fromUtf8Impl(const char *s, int len);
    static void utf8FromU16(const std::u16string &in, QByteArray &out);
    std::u16string m_s;
};

// --------------------------------------------------------------- QStringList
class QStringList : public std::vector<QString> {
public:
    QStringList() {}
    QStringList(std::initializer_list<QString> init) { for (const QString &s : init) push_back(s); }
    QStringList &operator<<(const QString &s) { push_back(s); return *this; }
    void append(const QString &s) { push_back(s); }
    QString join(const QString &sep) const { return QString::join(*this, sep); }
};

struct QLatin1String {
    const char *s;
    QLatin1String(const char *str) : s(str) {}
    operator QString() const { return QString(s); }
};

// ------------------------------------------------------------------- QSize
class QSize {
public:
    QSize(int w = 0, int h = 0) : m_w(w), m_h(h) {}
    int width() const { return m_w; }
    int height() const { return m_h; }
    bool isEmpty() const { return m_w <= 0 || m_h <= 0; }
private:
    int m_w, m_h;
};

// --------------------------------------------------------------- QDateTime
class QDateTime {
public:
    QDateTime() : m_secs(0), m_valid(false) {}
    static QDateTime fromSecsSinceEpoch(qint64 s) { QDateTime d; d.m_secs = s; d.m_valid = true; return d; }
    qint64 toSecsSinceEpoch() const { return m_secs; }
    QDateTime toUTC() const { return *this; }
    bool isValid() const { return m_valid; }
    friend inline bool operator==(const QDateTime &a, const QDateTime &b) { return a.m_secs == b.m_secs && a.m_valid == b.m_valid; }
    friend inline bool operator!=(const QDateTime &a, const QDateTime &b) { return !(a == b); }
private:
    qint64 m_secs;
    bool m_valid;
};

// ---------------------------------------------------------------- QRgba64
class QRgba64 {
public:
    QRgba64() : m_r(0), m_g(0), m_b(0), m_a(0) {}
    static QRgba64 fromRgba64(quint16 r, quint16 g, quint16 b, quint16 a) { QRgba64 c; c.m_r = r; c.m_g = g; c.m_b = b; c.m_a = a; return c; }
    static QRgba64 fromRgba64(quint64 v) {
        QRgba64 c;
        c.m_r = quint16(v & 0xFFFF); c.m_g = quint16((v >> 16) & 0xFFFF);
        c.m_b = quint16((v >> 32) & 0xFFFF); c.m_a = quint16((v >> 48) & 0xFFFF);
        return c;
    }
    static QRgba64 fromRgba(quint8 r, quint8 g, quint8 b, quint8 a) {
        return fromRgba64(quint16(r * 0x101), quint16(g * 0x101), quint16(b * 0x101), quint16(a * 0x101));
    }
    quint16 red() const { return m_r; }
    quint16 green() const { return m_g; }
    quint16 blue() const { return m_b; }
    quint16 alpha() const { return m_a; }
    void setRed(quint16 v) { m_r = v; }
    void setGreen(quint16 v) { m_g = v; }
    void setBlue(quint16 v) { m_b = v; }
    void setAlpha(quint16 v) { m_a = v; }
    quint64 rgba64() const { return quint64(m_r) | (quint64(m_g) << 16) | (quint64(m_b) << 32) | (quint64(m_a) << 48); }
private:
    quint16 m_r, m_g, m_b, m_a;
};

inline QRgba64 qPremultiply(QRgba64 c) { return c; }

// -------------------------------------------------------------- QTransform
class QTransform {
public:
    QTransform() : m_type(None) {}
    void rotate(int degrees) { m_rot = degrees % 360; m_type = Rot; }
    bool isIdentity() const { return m_type == None; }
    int rotation() const { return m_rot; }
private:
    enum Kind { None, Rot } m_type;
    int m_rot = 0;
};

// ------------------------------------------------------------------ QImage
class QImage {
public:
    enum Format {
        Format_Invalid = 0,
        Format_RGB32 = 4,     // 0xffRRGGBB, 4 bytes/px
        Format_RGB888 = 13,   // 3 bytes/px, 32-bit aligned scanlines
        Format_RGBX64 = 25,   // 16:16:16:16, 8 bytes/px
        Format_Grayscale8 = 26
    };

    QImage() : m_w(0), m_h(0), m_fmt(Format_Invalid) {}
    QImage(int w, int h, Format f) { init(w, h, f); }
    QSize size() const { return QSize(m_w, m_h); }
    int width() const { return m_w; }
    int height() const { return m_h; }
    Format format() const { return m_fmt; }
    bool isNull() const { return m_data.empty() || m_w <= 0 || m_h <= 0; }
    int bytesPerLine() const { return m_stride; }
    int byteCount() const { return (int)m_data.size(); }
    size_t sizeInBytes() const { return m_data.size(); }

    uchar *bits() { return m_data.empty() ? nullptr : m_data.data(); }
    const uchar *bits() const { return m_data.empty() ? nullptr : m_data.data(); }
    const uchar *constBits() const { return bits(); }
    uchar *scanLine(int y) { return m_data.empty() ? nullptr : m_data.data() + size_t(y) * size_t(m_stride); }
    const uchar *scanLine(int y) const { return m_data.empty() ? nullptr : m_data.data() + size_t(y) * size_t(m_stride); }
    const uchar *constScanLine(int y) const { return scanLine(y); }

    void fill(quint32 pixelValue);
    QImage convertToFormat(Format f) const;
    QImage scaled(int w, int h, Qt::AspectRatioMode aspect = Qt::IgnoreAspectRatio,
                  Qt::TransformationMode mode = Qt::FastTransformation) const;
    QImage transformed(const QTransform &t, Qt::TransformationMode mode = Qt::FastTransformation) const;
    QImage mirrored(bool horizontal = false, bool vertical = true) const;
    QImage copy(int x, int y, int w, int h) const;

    static QImage fromData(const uchar *data, int len, const char *format = nullptr);
    static QImage fromData(const QByteArray &data, const char *format = nullptr) { return fromData((const uchar *)data.constData(), data.size(), format); }
    bool save(const QString &path, const char *format = nullptr, int quality = -1) const;

private:
    void init(int w, int h, Format f);
    int m_w = 0, m_h = 0;
    Format m_fmt = Format_Invalid;
    int m_stride = 0;
    std::vector<uchar> m_data;
};

// --------------------------------------------------------------- QIODevice
struct QIODevice {
    enum OpenModeFlag {
        NotOpen = 0, ReadOnly = 1, WriteOnly = 2, ReadWrite = 3,
        Append = 4, Truncate = 8, Text = 16, Unbuffered = 32
    };
};

// ------------------------------------------------------------------- QFile
class QFile {
public:
    QFile() {}
    QFile(const QString &path) : m_path(path) {}
    void setFileName(const QString &p) { m_path = p; }
    QString fileName() const { return m_path; }

    bool open(int flags);
    void close();
    bool isOpen() const { return m_fp != nullptr; }
    bool resize(qint64 size);

    QByteArray readAll();
    qint64 write(const void *data, qint64 len);
    qint64 write(const QByteArray &a) { return write(a.constData(), a.size()); }
    qint64 seek(qint64 pos);

    static bool exists(const QString &path);
    static bool remove(const QString &path);
    static bool rename(const QString &from, const QString &to);
    static QByteArray encodeName(const QString &path) { return path.toUtf8(); }

private:
    QString m_path;
    std::FILE *m_fp = nullptr;
};

// --------------------------------------------------------------- QFileInfo
class QFileInfo {
public:
    QFileInfo() {}
    QFileInfo(const QString &path) : m_path(path) {}
    bool exists() const;
    QString filePath() const { return m_path; }
    QString absoluteFilePath() const { return m_path; }
    qint64 size() const;
    QDateTime lastModified() const;
    QString suffix() const;
    QString fileName() const;
private:
    QString m_path;
};

// -------------------------------------------------------------------- QDir
class QDir {
public:
    QDir() {}
    QDir(const QString &path) : m_path(path) {}
    QString path() const { return m_path; }
    QString absolutePath() const { return m_path; }
    bool mkpath(const QString &sub) const;
    bool exists() const;
    QString filePath(const QString &name) const { return m_path.isEmpty() ? name : (m_path + QString("/") + name); }
    static QString homePath();
    static QString currentPath();
private:
    QString m_path;
};

// ------------------------------------------------------------ QTextStream
class QTextStream {
public:
    QTextStream(QFile *f) : m_f(f) {}
    QTextStream &operator<<(const char *s) { m_f->write(s, (qint64)strlen(s)); return *this; }
    QTextStream &operator<<(const QString &s) { QByteArray u = s.toUtf8(); m_f->write(u); return *this; }
    QTextStream &operator<<(int v) { char b[32]; snprintf(b, sizeof(b), "%d", v); return (*this) << b; }
    QTextStream &operator<<(uint v) { char b[32]; snprintf(b, sizeof(b), "%u", v); return (*this) << b; }
    QTextStream &operator<<(long long v) { char b[32]; snprintf(b, sizeof(b), "%lld", v); return (*this) << b; }
    QTextStream &operator<<(unsigned long long v) { char b[32]; snprintf(b, sizeof(b), "%llu", v); return (*this) << b; }
    QTextStream &operator<<(unsigned short v) { char b[16]; snprintf(b, sizeof(b), "%u", (unsigned)v); return (*this) << b; }
    QTextStream &operator<<(double v) { char b[64]; snprintf(b, sizeof(b), "%g", v); return (*this) << b; }
    QTextStream &operator<<(char c) { m_f->write(&c, 1); return *this; }
private:
    QFile *m_f;
};

// ------------------------------------------------------------ QStandardPaths
struct QStandardPaths {
    enum StandardLocation { AppDataLocation = 17, AppLocalDataLocation = 9, HomeLocation = 8, TempLocation = 2 };
    static QString writableLocation(int type);
    static QString findExecutable(const QString &name);
};

// ---------------------------------------------------------------- QProcess
class QProcess {
public:
    enum ExitStatus { NormalExit = 0, CrashExit = 1 };
    QProcess() {}
    void start(const QString &program, const QStringList &args);
    bool waitForFinished(int msecs);
    ExitStatus exitStatus() const { return m_exitStatus; }
    int exitCode() const { return m_exitCode; }
    QByteArray readAllStandardOutput() const { return m_out; }
    QByteArray readAllStandardError() const { return m_err; }
private:
    QByteArray m_out, m_err;
    int m_exitCode = -1;
    ExitStatus m_exitStatus = CrashExit;
};

// ------------------------------------------------------------------ macros
#define QStringLiteral(str) QString(u"" str)
#define QByteArrayLiteral(str) QByteArray(str)
#define QLatin1StringLiteral(str) QLatin1String(str)

inline QString QStringJoin(const QStringList &list, const QString &sep) {
    QString out;
    for (size_t i = 0; i < list.size(); ++i) { if (i) out += sep; out += list[i]; }
    return out;
}

#endif // MINIQT_H
