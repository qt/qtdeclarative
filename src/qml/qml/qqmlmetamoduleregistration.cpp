// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtqmlglobal_p.h>
#include <qqmlmoduleregistration.h>
#include <qqml.h>

QT_BEGIN_NAMESPACE

// Provide the type registration for QtQml here, in libQtQml.so.
// This way we get a completely functional QtQml module and don't have to
// rely on the plugin to be loaded.
// In CMakeLists.txt we've specified NO_GENERATE_QMLTYPES to prevent
// the generation of an extra type registration file.
Q_QML_PRIVATE_EXPORT void qml_register_types_QtQml()
{
    // ### Qt7: Handle version 6 like version 2.
    qmlRegisterModule("QtQml", 2, 0);
    qmlRegisterModule("QtQml", 2, 254);
    qmlRegisterModule("QtQml", QT_VERSION_MAJOR, 0);
    qmlRegisterModule("QtQml", QT_VERSION_MAJOR, QT_VERSION_MINOR);
}

static const QQmlModuleRegistration registration("QtQml", qml_register_types_QtQml);

QT_END_NAMESPACE
