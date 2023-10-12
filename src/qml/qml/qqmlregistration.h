// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLREGISTRATION_H
#define QQMLREGISTRATION_H

#include <QtCore/qglobal.h>
#include <QtQmlIntegration/qqmlintegration.h>

// satisfy configure, which warns about public headers not using those
QT_BEGIN_NAMESPACE

#define QML_FOREIGN(FOREIGN_TYPE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_TYPE) \
    using QmlForeignType = FOREIGN_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlResolved; \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_FOREIGN_NAMESPACE(FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.ForeignIsNamespace", "true") \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_CUSTOMPARSER Q_CLASSINFO("QML.HasCustomParser", "true")

QT_END_NAMESPACE

#endif // QQMLREGISTRATION_H
