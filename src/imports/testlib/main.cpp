/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "quicktestevent_p.h"
#include "quicktestutil_p.h"

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTest/private/quicktestresult_p.h>
#include <QtQuickTest/private/qtestoptions_p.h>

QML_DECLARE_TYPE(QuickTestResult)
QML_DECLARE_TYPE(QuickTestEvent)
QML_DECLARE_TYPE(QuickTestUtil)

extern void qml_register_types_QtTest();
GHS_KEEP_REFERENCE(qml_register_types_QtTest);

QT_BEGIN_NAMESPACE

class QTestQmlModule : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    QTestQmlModule(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtTest;
        Q_UNUSED(registration);
    }
};

QT_END_NAMESPACE

#include "main.moc"
