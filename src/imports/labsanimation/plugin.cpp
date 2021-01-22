/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

#include "qquickboundaryrule_p.h"

extern void qml_register_types_Qt_labs_animation();
GHS_KEEP_REFERENCE(qml_register_types_Qt_labs_animation);

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule Qt.labs.animation 1.0
    \title Qt Quick experimental animation types
    \ingroup qmlmodules
    \brief Provides QML experimental types for animation
    \since 5.14

    This QML module contains experimental QML types related to animation.

    To use the types in this module, import the module with the following line:

    \code
    import Qt.labs.animation 1.0
    \endcode
*/

//![class decl]
class QtLabsAnimationPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtLabsAnimationPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_Qt_labs_animation;
        Q_UNUSED(registration);
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
