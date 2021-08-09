/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/qquickstyle.h>

extern void qml_register_types_QtQuick_Controls_Windows();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Windows);

QT_BEGIN_NAMESPACE

class QtQuickControls2WindowsStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2WindowsStylePlugin(QObject *parent = nullptr);
    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
};

QtQuickControls2WindowsStylePlugin::QtQuickControls2WindowsStylePlugin(QObject *parent):
    QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Windows;
    Q_UNUSED(registration);
}

QString QtQuickControls2WindowsStylePlugin::name() const
{
    return QStringLiteral("Windows");
}

void QtQuickControls2WindowsStylePlugin::initializeTheme(QQuickTheme * /*theme*/)
{
}

QT_END_NAMESPACE

#include "qtquickcontrols2windowsstyleplugin.moc"
