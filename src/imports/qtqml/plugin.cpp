/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
