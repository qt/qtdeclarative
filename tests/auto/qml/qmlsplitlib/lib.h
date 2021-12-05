/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
