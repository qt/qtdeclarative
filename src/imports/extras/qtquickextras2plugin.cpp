/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Extras module of the Qt Toolkit.
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

#include <QtQml/qqmlextensionplugin.h>
#include <QtCore/qdir.h>

#include "qquickdial_p.h"
#include "qquickdrawer_p.h"
#include "qquickswipeview_p.h"
#include "qquicktumbler_p.h"

QT_BEGIN_NAMESPACE

class QtQuickExtras2Plugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    void registerTypes(const char *uri);
};

void QtQuickExtras2Plugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickDial>(uri, 2, 0, "AbstractDial");
    qmlRegisterType<QQuickDrawer>(uri, 2, 0, "AbstractDrawer");
    qmlRegisterType<QQuickSwipeView>(uri, 2, 0, "AbstractSwipeView");
    qmlRegisterType<QQuickSwipeViewAttached>();
    qmlRegisterType<QQuickTumbler>(uri, 2, 0, "AbstractTumbler");
    qmlRegisterType<QQuickTumblerAttached>();

    QDir baseDir(baseUrl().toLocalFile());
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Dial.qml"))), uri, 2, 0, "Dial");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Drawer.qml"))), uri, 2, 0, "Drawer");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("SwipeView.qml"))), uri, 2, 0, "SwipeView");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Tumbler.qml"))), uri, 2, 0, "Tumbler");
}

QT_END_NAMESPACE

#include "qtquickextras2plugin.moc"
