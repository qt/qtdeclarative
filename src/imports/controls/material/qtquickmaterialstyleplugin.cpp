/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include <QtQuickControls/private/qquickstyleplugin_p.h>

#include "qquickmaterialstyle_p.h"
#include "qquickmaterialtheme_p.h"
#include "qquickmaterialprogressring_p.h"
#include "qquickmaterialprogressstrip_p.h"

#include <QtQuickControls/private/qquickstyleselector_p.h>

static inline void initResources()
{
    Q_INIT_RESOURCE(qtquickmaterialstyleplugin);
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_Qt_labs_controls_material);
#endif
}

QT_BEGIN_NAMESPACE

class QtQuickMaterialStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    QtQuickMaterialStylePlugin(QObject *parent = nullptr);

    void registerTypes(const char *uri) override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;

    QString name() const override;
    QQuickProxyTheme *createTheme() const override;
};

QtQuickMaterialStylePlugin::QtQuickMaterialStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    initResources();
}

void QtQuickMaterialStylePlugin::registerTypes(const char *uri)
{
    qmlRegisterUncreatableType<QQuickMaterialStyle>(uri, 1, 0, "Material", tr("Material is an attached property"));
}

void QtQuickMaterialStylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQuickStylePlugin::initializeEngine(engine, uri);

    QByteArray import = QByteArray(uri) + ".impl";
    qmlRegisterType<QQuickMaterialProgressRing>(import, 1, 0, "ProgressRing");
    qmlRegisterType<QQuickMaterialProgressStrip>(import, 1, 0, "ProgressStrip");
    qmlRegisterType<QQuickMaterialRingAnimator>(import, 1, 0, "RingAnimator");
    qmlRegisterType<QQuickMaterialStripAnimator>(import, 1, 0, "StripAnimator");
    qmlRegisterType(QUrl(baseUrl().toString() + QStringLiteral("/Ripple.qml")), import, 1, 0, "Ripple");
    qmlRegisterType(QUrl(baseUrl().toString() + QStringLiteral("/SliderHandle.qml")), import, 1, 0, "SliderHandle");
}

QString QtQuickMaterialStylePlugin::name() const
{
    return QStringLiteral("material");
}

QQuickProxyTheme *QtQuickMaterialStylePlugin::createTheme() const
{
    return new QQuickMaterialTheme;
}

QT_END_NAMESPACE

#include "qtquickmaterialstyleplugin.moc"
