/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtQml/qqml.h>

#include "qquickimageselector_p.h"
#include "qquickimaginestyle_p.h"
#include "qquickimaginetheme_p.h"
#include "qquickninepatchimage_p.h"

static inline void initResources()
{
    Q_INIT_RESOURCE(qmake_qtquickcontrols2imaginestyle);
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_QtQuick_Controls_2_Imagine);
#endif
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtQuickControlsImagine, "qt.quick.controls.imagine")

class QtQuickControls2ImagineStylePlugin: public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2ImagineStylePlugin(QObject *parent = nullptr);

    void registerTypes(const char *uri) override;

    QString name() const override;
    QQuickProxyTheme *createTheme() const override;
};

QtQuickControls2ImagineStylePlugin::QtQuickControls2ImagineStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    initResources();
}

void QtQuickControls2ImagineStylePlugin::registerTypes(const char *uri)
{
    qmlRegisterModule(uri, 2, QT_VERSION_MINOR - 7); // Qt 5.10 -> 2.3, 5.11 -> 2.4, ...
    qmlRegisterUncreatableType<QQuickImagineStyle>(uri, 2, 3, "Imagine", tr("Imagine is an attached property"));

    QByteArray import = QByteArray(uri) + ".impl";
    qmlRegisterModule(import, 2, QT_VERSION_MINOR - 7); // Qt 5.10 -> 2.3, 5.11 -> 2.4, ...

    qmlRegisterType<QQuickAnimatedImageSelector>(import, 2, 3, "AnimatedImageSelector");
    qmlRegisterType<QQuickImageSelector>(import, 2, 3, "ImageSelector");
    qmlRegisterType<QQuickNinePatchImage>(import, 2, 3, "NinePatchImage");
    qmlRegisterType<QQuickNinePatchImageSelector>(import, 2, 3, "NinePatchImageSelector");
}

QString QtQuickControls2ImagineStylePlugin::name() const
{
    return QStringLiteral("imagine");
}

QQuickProxyTheme *QtQuickControls2ImagineStylePlugin::createTheme() const
{
    return new QQuickImagineTheme;
}

QT_END_NAMESPACE

#include "qtquickcontrols2imaginestyleplugin.moc"
