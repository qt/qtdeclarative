// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SPLITLIB_LIB_H
#define SPLITLIB_LIB_H
#include "tst_qmlsplitlib_library_export.h"

#include <QtCore/qglobal.h>
#include <QObject>

// FIXME: should at some point be in qtbase or separate repo
// taken from qqmlregistration.h
#define QML_ELEMENT \
    Q_CLASSINFO("QML.Element", "auto")

#define QML_PRIVATE_NAMESPACE \
    QT_PREPEND_NAMESPACE(QQmlPrivate)

#define QML_REGISTER_TYPES_AND_REVISIONS \
    QT_PREPEND_NAMESPACE(qmlRegisterTypesAndRevisions)

QT_BEGIN_NAMESPACE
namespace QQmlPrivate {
    template<typename, typename> struct QmlSingleton;

}
template<typename... Args>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor, QList<int> *);
QT_END_NAMESPACE

#define QML_SINGLETON \
    Q_CLASSINFO("QML.Singleton", "true") \
    enum class QmlIsSingleton {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSingleton; \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_NAMED_ELEMENT(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

class TST_QMLSPLITLIB_LIBRARY_EXPORT SplitLib : public QObject
{
public:
    Q_OBJECT
    QML_ELEMENT

    Q_INVOKABLE bool transmogrify();
};


class TST_QMLSPLITLIB_LIBRARY_EXPORT Foo : public QObject
{
public:
    Q_OBJECT
    QML_NAMED_ELEMENT(Bar)
    QML_SINGLETON
};

#endif
