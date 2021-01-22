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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtQml/private/qqmlengine_p.h>
#include <QtQmlModels/private/qqmlmodelsmodule_p.h>
#if QT_CONFIG(qml_worker_script)
#include <QtQmlWorkerScript/private/qqmlworkerscriptmodule_p.h>
#endif
#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

#include <private/qtquick2_p.h>

QT_BEGIN_NAMESPACE

//![class decl]
class QtQuick2Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    QtQuick2Plugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick;
        Q_UNUSED(registration);
    }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick"));
        Q_UNUSED(uri);
        moduleDefined = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQmlEnginePrivate::registerQuickTypes();
        QQmlModelsModule::registerQuickTypes();
#if QT_CONFIG(qml_worker_script)
        QQmlWorkerScriptModule::registerQuickTypes();
#endif
#endif
        QQmlQtQuick2Module::defineModule();
    }

    ~QtQuick2Plugin() override
    {
        if (moduleDefined)
            QQmlQtQuick2Module::undefineModule();
    }

    bool moduleDefined = false;
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
