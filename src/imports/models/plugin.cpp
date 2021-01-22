/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
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

#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/qqmlextensionplugin.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQml.Models 2.\QtMinorVersion
    \title Qt QML Models QML Types
    \ingroup qmlmodules
    \brief Provides QML types for data models
    \since 5.1

    This QML module contains types for defining data models in QML.

    To use the types in this module, import the module with the following line:

    \qml \QtMinorVersion
    import QtQml.Models 2.\1
    \endqml

    \note QtQml.Models module started at version 2.1 to match the version
    of the parent module, \l{Qt QML}.

    In addition, Qt.labs.qmlmodels provides experimental QML types for models.
    To use these experimental types, import the module with the following line:

    \qml
    import Qt.labs.qmlmodels 1.0
    \endqml

    \section1 QML Types
    \generatelist qmltypesbymodule QtQml.Models

    \section1 Experimental QML Types
    \generatelist qmltypesbymodule Qt.labs.qmlmodels

    \noautolist
*/



//![class decl]
class QtQmlModelsPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQmlModelsPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQml_Models;
        Q_UNUSED(registration);
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
