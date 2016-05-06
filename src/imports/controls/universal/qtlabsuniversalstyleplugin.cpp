/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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
#include "qquickuniversalfocusrectangle_p.h"
#include "qquickuniversalimageprovider_p.h"
#include "qquickuniversalprogressring_p.h"
#include "qquickuniversalprogressstrip_p.h"
#include "qquickuniversalstyle_p.h"
#include "qquickuniversaltheme_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtLabsControls/private/qquickstyleselector_p.h>

static inline void initResources()
{
    Q_INIT_RESOURCE(qtlabsuniversalstyleplugin);
}

QT_BEGIN_NAMESPACE

class QtLabsUniversalStylePlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    ~QtLabsUniversalStylePlugin();
    void registerTypes(const char *uri) Q_DECL_OVERRIDE;
    void initializeEngine(QQmlEngine *engine, const char *uri) Q_DECL_OVERRIDE;

private:
    QQuickProxyTheme *theme;
};

QtLabsUniversalStylePlugin::~QtLabsUniversalStylePlugin()
{
    if (theme) {
        QPlatformTheme *old = theme->theme();
        QGuiApplicationPrivate::platform_theme = old;
        delete theme;
    }
}

void QtLabsUniversalStylePlugin::registerTypes(const char *uri)
{
    qmlRegisterUncreatableType<QQuickUniversalStyle>(uri, 1, 0, "Universal", tr("Universal is an attached property"));
}

void QtLabsUniversalStylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQuickStyleSelector selector;
    if (selector.style() == QLatin1String("universal")) {
        QPlatformTheme *old = QGuiApplicationPrivate::platform_theme;
        if (old) {
            QQuickProxyTheme *theme = new QQuickUniversalTheme(old);
            QGuiApplicationPrivate::platform_theme = theme;
        }
    }

    initResources();
    engine->addImageProvider(QStringLiteral("universal"), new QQuickUniversalImageProvider);

    QByteArray import = QByteArray(uri) + ".impl";
    qmlRegisterType<QQuickUniversalFocusRectangle>(import, 1, 0, "FocusRectangle");
    qmlRegisterType<QQuickUniversalProgressRing>(import, 1, 0, "ProgressRing");
    qmlRegisterType<QQuickUniversalProgressRingAnimator>(import, 1, 0, "ProgressRingAnimator");
    qmlRegisterType<QQuickUniversalProgressStrip>(import, 1, 0, "ProgressStrip");
    qmlRegisterType<QQuickUniversalProgressStripAnimator>(import, 1, 0, "ProgressStripAnimator");
}

QT_END_NAMESPACE

#include "qtlabsuniversalstyleplugin.moc"
