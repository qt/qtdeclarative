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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

#include "qquickfolderlistmodel.h"

extern void qml_register_types_Qt_labs_folderlistmodel();
GHS_KEEP_REFERENCE(qml_register_types_Qt_labs_folderlistmodel);

QT_BEGIN_NAMESPACE

//![class decl]
class QmlFolderListModelPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QmlFolderListModelPlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_Qt_labs_folderlistmodel;
        Q_UNUSED(registration);
    }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Qt.labs.folderlistmodel"));

        // Major version 1 only has a single revision, 0.
        qmlRegisterType<QQuickFolderListModel>(uri, 1, 0, "FolderListModel");
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
