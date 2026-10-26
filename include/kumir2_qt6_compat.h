#ifndef KUMIR2_QT6_COMPAT_H
#define KUMIR2_QT6_COMPAT_H

#if __has_include(<QtGlobal>)
#if __has_include(<QTextStream>)
#include <QTextStream>
#endif
#if __has_include(<QtCore5Compat/QTextCodec>)
#include <QtCore5Compat/QTextCodec>
inline void setTextStreamCodec(QTextStream &ts, QTextCodec *codec) {
#if QT_VERSION >= 0x060000
    if (codec) {
        ts.setEncoding(QStringConverter::encodingForName(codec->name()).value_or(QStringConverter::Utf8));
    }
#else
    ts.setCodec(codec);
#endif
}
#endif
#include <QtGlobal>
#include <QString>

#include <QIODevice>
#include <QRegularExpression>
#if __has_include(<QTransform>)
#include <QTransform>
typedef QTransform QMatrix;
#endif

#include <QRegularExpressionMatch>
#include <QSet>
#include <QList>

#if __has_include(<QFontMetrics>)
#include <QFontMetrics>
#if QT_VERSION >= 0x050B00
template <typename T>
inline int fontMetricsWidth(const QFontMetrics &fm, const T &str) {
    return fm.horizontalAdvance(str);
}
#else
template <typename T>
inline int fontMetricsWidth(const QFontMetrics &fm, const T &str) {
    return fm.width(str);
}
#endif
#endif

#if __has_include(<QFontMetricsF>)
#include <QFontMetricsF>
#if QT_VERSION >= 0x050B00
template <typename T>
inline qreal fontMetricsWidth(const QFontMetricsF &fm, const T &str) {
    return fm.horizontalAdvance(str);
}
#else
template <typename T>
inline qreal fontMetricsWidth(const QFontMetricsF &fm, const T &str) {
    return fm.width(str);
}
#endif
#endif

#if __has_include(<QWheelEvent>)
#include <QWheelEvent>
inline int wheelEventDelta(const QWheelEvent *event) {
#if QT_VERSION >= 0x050C00
    return event->angleDelta().y();
#else
    return event->delta();
#endif
}
#endif

#if __has_include(<QGraphicsSceneWheelEvent>)
#include <QGraphicsSceneWheelEvent>
inline int wheelEventDelta(const QGraphicsSceneWheelEvent *event) {
    return event->delta();
}
#endif

#if __has_include(<QPalette>)
#include <QPalette>
#ifndef QPalette_Foreground
#define QPalette_Foreground QPalette::WindowText
#endif
#ifndef QPalette_Background
#define QPalette_Background QPalette::Window
#endif
#endif

#if QT_VERSION >= 0x060000
#include <QStringConverter>
#if __has_include(<QtCore5Compat/QRegExp>)
#include <QtCore5Compat/QRegExp>
#endif
#include <QMultiMap>
#include <QMap>

#ifndef QStyleOptionFrameV3
#define QStyleOptionFrameV3 QStyleOptionFrame
#endif

inline void setTextStreamCodec(QTextStream &ts, const char *codecName) {
    auto enc = QStringConverter::encodingForName(codecName);
    if (enc.has_value()) {
        ts.setEncoding(enc.value());
    }
}

inline void setTextStreamCodec(QTextStream &ts, const QString &codecName) {
    setTextStreamCodec(ts, codecName.toLatin1().constData());
}

#ifndef trUtf8
#define trUtf8 tr
#endif

template <typename T>
inline QList<T> qsetToQList(const QSet<T> &set) {
    return QList<T>(set.begin(), set.end());
}

template <typename Key, typename T>
inline QList<T> qmapValues(const QMap<Key, T> &map, const Key &key) {
    QList<T> res;
    for (auto it = map.find(key); it != map.end() && it.key() == key; ++it) {
        res.append(it.value());
    }
    return res;
}
template <typename Key, typename T>
inline QList<T> qmapValues(const QMultiMap<Key, T> &map, const Key &key) {
    return map.values(key);
}

#else

inline void setTextStreamCodec(QTextStream &ts, const char *codecName) {
    ts.setCodec(codecName);
}

inline void setTextStreamCodec(QTextStream &ts, const QString &codecName) {
    ts.setCodec(codecName.toLatin1().constData());
}

template <typename T>
inline QList<T> qsetToQList(const QSet<T> &set) {
    return set.toList();
}

template <typename Key, typename T>
inline QList<T> qmapValues(const QMap<Key, T> &map, const Key &key) {
    return map.values(key);
}

#endif

#endif // __has_include(<QtGlobal>)

#endif // KUMIR2_QT6_COMPAT_H
