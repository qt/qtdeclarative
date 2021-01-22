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

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQml/qqmlextensionplugin.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtQmlModels/private/qqmlmodelsmodule_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQml 2.\QtMinorVersion
    \title Qt QML Base Types
    \ingroup qmlmodules
    \brief Provides basic QML types
    \since 5.0

    This QML module contains basic QML types.

    To use the types in this module, import the module with the following line:

    \qml \QtMinorVersion
    import QtQml 2.\1
    \endqml
*/

//![class decl]
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class QtQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    QtQmlPlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQml;
        Q_UNUSED(registration);
    }

    void registerTypes(const char *) override { QQmlModelsModule::registerQmlTypes(); }
};
#else
class QtQmlPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQmlPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQml;
        Q_UNUSED(registration);
    }
};
#endif
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
