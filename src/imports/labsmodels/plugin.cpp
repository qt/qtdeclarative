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

#include <private/qqmlmodelsmodule_p.h>

#if QT_CONFIG(qml_table_model)
#include "qqmltablemodel_p.h"
#include "qqmltablemodelcolumn_p.h"
#endif
#if QT_CONFIG(qml_delegate_model)
#include "qqmldelegatecomponent_p.h"
#endif

extern void qml_register_types_Qt_labs_qmlmodels();
GHS_KEEP_REFERENCE(qml_register_types_Qt_labs_qmlmodels);

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule Qt.labs.qmlmodels 1.0
    \title Qt QML Models experimental QML Types
    \ingroup qmlmodules
    \brief Provides QML experimental types for data models.
    \since 5.12

    This QML module contains experimental QML types related to data models.

    To use the types in this module, import the module with the following line:

    \code
    import Qt.labs.qmlmodels 1.0
    \endcode
*/

//![class decl]
class QtQmlLabsModelsPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQmlLabsModelsPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_Qt_labs_qmlmodels;
        Q_UNUSED(registration);
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
