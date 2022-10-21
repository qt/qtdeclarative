/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLREGISTRATION_H
#define QQMLREGISTRATION_H

#include <QtCore/qglobal.h>

// satisfy configure, which warns about public headers not using those
QT_BEGIN_NAMESPACE

#define QML_PRIVATE_NAMESPACE \
    QT_PREPEND_NAMESPACE(QQmlPrivate)

#define QML_REGISTER_TYPES_AND_REVISIONS \
    QT_PREPEND_NAMESPACE(qmlRegisterTypesAndRevisions)

#define QML_ELEMENT \
    Q_CLASSINFO("QML.Element", "auto")

#define QML_ANONYMOUS \
    Q_CLASSINFO("QML.Element", "anonymous")

#define QML_NAMED_ELEMENT(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_VALUE_TYPE(NAME) \
    Q_CLASSINFO("QML.Element", #NAME) \
    QML_UNCREATABLE("Value types cannot be created.")

#define QML_UNCREATABLE(REASON) \
    Q_CLASSINFO("QML.Creatable", "false") \
    Q_CLASSINFO("QML.UncreatableReason", REASON)

#define QML_SINGLETON \
    Q_CLASSINFO("QML.Singleton", "true") \
    enum class QmlIsSingleton {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSingleton; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_SEQUENTIAL_CONTAINER(VALUE_TYPE) \
    Q_CLASSINFO("QML.Sequence", #VALUE_TYPE) \
    using QmlSequenceValueType = VALUE_TYPE; \
    enum class QmlIsSequence {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSequence; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_ADDED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(VERSION))

#define QML_ADDED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_EXTRA_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.ExtraVersion", Q_REVISION(MAJOR, MINOR))

#define QML_REMOVED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(VERSION))

#define QML_REMOVED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_ATTACHED(ATTACHED_TYPE) \
    Q_CLASSINFO("QML.Attached", #ATTACHED_TYPE) \
    using QmlAttachedType = ATTACHED_TYPE; \
    template<class, class, bool> friend struct QML_PRIVATE_NAMESPACE::QmlAttached; \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlAttachedAccessor;

#define QML_EXTENDED(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    using QmlExtendedType = EXTENDED_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtended; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_EXTENDED_NAMESPACE(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE) \
    static constexpr const QMetaObject *qmlExtendedNamespace() { return &EXTENDED_NAMESPACE::staticMetaObject; } \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtendedNamespace; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_FOREIGN(FOREIGN_TYPE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_TYPE) \
    using QmlForeignType = FOREIGN_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlResolved; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_FOREIGN_NAMESPACE(FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_NAMESPACE)

#define QML_INTERFACE \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsInterface {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_IMPLEMENTS_INTERFACES(INTERFACES) \
    Q_INTERFACES(INTERFACES) \
    enum class QmlIsInterface {yes = false}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface;

#define QML_UNAVAILABLE \
    QML_FOREIGN(QQmlTypeNotAvailable)

#define QML_CUSTOMPARSER Q_CLASSINFO("QML.HasCustomParser", "true")

QT_END_NAMESPACE

#endif // QQMLREGISTRATION_H
