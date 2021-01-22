/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLINFO_H
#define QQMLINFO_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtQml/qqmlerror.h>

QT_BEGIN_NAMESPACE

class QQmlInfo;

// declared in namespace to avoid symbol conflicts with QtDeclarative
namespace QtQml {
    Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me);
    Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QQmlError &error);
    Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QList<QQmlError> &errors);

    Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me);
    Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QQmlError &error);
    Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QList<QQmlError> &errors);

    Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me);
    Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QQmlError &error);
    Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QList<QQmlError> &errors);
}
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wheader-hygiene")
// This is necessary to allow for QtQuick1 and QtQuick2 scenes in a single application.
using namespace QtQml;
QT_WARNING_POP

class QQmlInfoPrivate;
class Q_QML_EXPORT QQmlInfo : public QDebug
{
public:
    QQmlInfo(const QQmlInfo &);
    ~QQmlInfo();

    inline QQmlInfo &operator<<(QChar t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(bool t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(char t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed short t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned short t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed int t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned int t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed long t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned long t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(qint64 t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(quint64 t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(float t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(double t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const char* t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const QString & t) { QDebug::operator<<(t.toLocal8Bit().constData()); return *this; }
    inline QQmlInfo &operator<<(const QStringRef & t) { return operator<<(t.toString()); }
    inline QQmlInfo &operator<<(const QLatin1String &t) { QDebug::operator<<(t.latin1()); return *this; }
    inline QQmlInfo &operator<<(const QByteArray & t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const void * t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(QTextStreamFunction f) { QDebug::operator<<(f); return *this; }
    inline QQmlInfo &operator<<(QTextStreamManipulator m) { QDebug::operator<<(m); return *this; }
#ifndef QT_NO_DEBUG_STREAM
    inline QQmlInfo &operator<<(const QUrl &t) { static_cast<QDebug &>(*this) << t; return *this; }
#endif

private:
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlDebug(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlDebug(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlDebug(const QObject *me, const QList<QQmlError> &errors);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlInfo(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlInfo(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlInfo(const QObject *me, const QList<QQmlError> &errors);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlWarning(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlWarning(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo QtQml::qmlWarning(const QObject *me, const QList<QQmlError> &errors);

    QQmlInfo(QQmlInfoPrivate *);
    QQmlInfoPrivate *d;
};

QT_END_NAMESPACE

#endif // QQMLINFO_H
