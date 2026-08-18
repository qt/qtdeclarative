// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:insignificant

#ifndef QMLINTEGRATION_H
#define QMLINTEGRATION_H

#include <QtCore/qglobal.h>

// forward declarations of structs and functions defined in QtQml
QT_BEGIN_NAMESPACE
namespace QQmlPrivate {
    template<typename> struct QmlSingleton;
    template<class, class, bool> struct QmlAttached;
    template<class> struct QmlAttachedAccessor;
    template<class> struct QmlExtended;
    template<typename> struct QmlInterface;
    template<class>
    struct QmlExtendedNamespace;
    template<class>
    struct QmlUncreatable;
    template<class>
    struct QmlAnonymous;
    template<class>
    struct QmlSequence;
    template<class>
    struct QmlResolved;
}

template <typename T> class QList;

template<typename... Args>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor,
                                  QList<int> *qmlTypeIds = nullptr);

QT_END_NAMESPACE


#define QML_PRIVATE_NAMESPACE \
    QT_PREPEND_NAMESPACE(QQmlPrivate)

#if QT_DEPRECATED_SINCE(6, 12)
QT_BEGIN_NAMESPACE
template <typename... Args>
QT_DEPRECATED_VERSION_X_6_12("You don't need this.")
void qmlDeprecatedRegisterTypesAndRevisions(const char *uri, int versionMajor,
                                            QList<int> *qmlTypeIds = nullptr)
{
    qmlRegisterTypesAndRevisions(uri, versionMajor, qmlTypeIds);
}
QT_END_NAMESPACE
#  define QML_REGISTER_TYPES_AND_REVISIONS \
      QT_PREPEND_NAMESPACE(qmlDeprecatedRegisterTypesAndRevisions)
#endif

#define QML_ELEMENT \
    Q_CLASSINFO("QML.Element", "auto")


#define QML_ANONYMOUS \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsAnonymous{yes = true}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlAnonymous; \
    inline constexpr void qt_qmlMarker_anonymous() {}

#define QML_NAMED_ELEMENT(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_UNCREATABLE(REASON) \
    Q_CLASSINFO("QML.Creatable", "false") \
    Q_CLASSINFO("QML.UncreatableReason", REASON) \
    enum class QmlIsUncreatable {yes = true}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlUncreatable; \
    inline constexpr void qt_qmlMarker_uncreatable() {}

#define QML_VALUE_TYPE(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_CONSTRUCTIBLE_VALUE \
    Q_CLASSINFO("QML.Creatable", "true") \
    Q_CLASSINFO("QML.CreationMethod", "construct")

#define QML_STRUCTURED_VALUE \
    Q_CLASSINFO("QML.Creatable", "true") \
    Q_CLASSINFO("QML.CreationMethod", "structured")

#define QML_SINGLETON \
    Q_CLASSINFO("QML.Singleton", "true") \
    enum class QmlIsSingleton {yes = true}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlSingleton; \
    inline constexpr void qt_qmlMarker_singleton() {}

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
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlAttachedAccessor; \
    inline constexpr void qt_qmlMarker_attached() {}

#define QML_EXTENDED(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    using QmlExtendedType = EXTENDED_TYPE; \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlExtended; \
    inline constexpr void qt_qmlMarker_extended() {}

#define QML_EXTENDED_NAMESPACE(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.ExtensionIsNamespace", "true") \
    static constexpr const QMetaObject *qmlExtendedNamespace() { return &EXTENDED_NAMESPACE::staticMetaObject; } \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlExtendedNamespace; \
    inline constexpr void qt_qmlMarker_extendedNamespace() {}

#define QML_NAMESPACE_EXTENDED(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE)

#define QML_INTERFACE \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsInterface {yes = true}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface; \
    inline constexpr void qt_qmlMarker_interface() {}

#define QML_IMPLEMENTS_INTERFACES(INTERFACES) \
    Q_INTERFACES(INTERFACES) \
    enum class QmlIsInterface {yes = false}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface;

#define QML_SEQUENTIAL_CONTAINER(VALUE_TYPE) \
    Q_CLASSINFO("QML.Sequence", #VALUE_TYPE) \
    using QmlSequenceValueType = VALUE_TYPE; \
    enum class QmlIsSequence {yes = true}; \
    template<typename> friend struct QML_PRIVATE_NAMESPACE::QmlSequence; \
    inline constexpr void qt_qmlMarker_sequence() {}

#define QML_UNAVAILABLE \
    QML_FOREIGN(QQmlTypeNotAvailable)

#define QML_FOREIGN(FOREIGN_TYPE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_TYPE) \
    using QmlForeignType = FOREIGN_TYPE; \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlResolved; \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_FOREIGN_NAMESPACE(FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.ForeignIsNamespace", "true") \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_CUSTOMPARSER \
    Q_CLASSINFO("QML.HasCustomParser", "true")

#define QML_USING(ORIGINAL) \
    using QmlUsing ## ORIGINAL = ORIGINAL; \
    Q_CLASSINFO("QML.Using", #ORIGINAL)

#endif
