// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtQml/QQmlEngineExtensionPlugin>
#include <qdebug.h>

//![plugin]
class QExampleQmlPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
};
//![plugin]

#include "plugin.moc"
