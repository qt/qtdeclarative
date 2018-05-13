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

#include "qquickstyleplugin_p.h"
#include "qquickstyle.h"
#include "qquickstyle_p.h"
#include "qquickstyleselector_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickStylePluginPrivate
{
public:
    QQuickTheme *theme = nullptr;
    mutable QScopedPointer<QQuickStyleSelector> selector;
};

QQuickStylePlugin::QQuickStylePlugin(QObject *parent)
    : QQmlExtensionPlugin(parent), d_ptr(new QQuickStylePluginPrivate)
{
}

QQuickStylePlugin::~QQuickStylePlugin()
{
    Q_D(QQuickStylePlugin);
    if (QQuickTheme::current() == d->theme)
        QQuickTheme::setCurrent(nullptr);
}

void QQuickStylePlugin::registerTypes(const char *uri)
{
    Q_UNUSED(uri);
}

void QQuickStylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_D(QQuickStylePlugin);
    Q_UNUSED(engine);
    Q_UNUSED(uri);

    // make sure not to re-create the theme if initializeEngine()
    // is called multiple times, like in case of qml2puppet (QTBUG-54995)
    if (d->theme)
        return;

    if (isCurrent()) {
        d->theme = createTheme();
        if (d->theme) {
#if QT_CONFIG(settings)
            QQuickThemePrivate *p = QQuickThemePrivate::get(d->theme);
            QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(name());
            if (settings) {
                p->defaultFont.reset(QQuickStylePrivate::readFont(settings));
                p->defaultPalette.reset(QQuickStylePrivate::readPalette(settings));
            }
#endif
            QQuickTheme::setCurrent(d->theme);
        }
    }
}

bool QQuickStylePlugin::isCurrent() const
{
    QString style = QQuickStyle::name();
    if (style.isEmpty())
        style = QStringLiteral("Default");

    const QString theme = name();
    return theme.compare(style, Qt::CaseInsensitive) == 0;
}

QString QQuickStylePlugin::name() const
{
    return QString();
}

QQuickTheme *QQuickStylePlugin::createTheme() const
{
    return nullptr;
}

QUrl QQuickStylePlugin::resolvedUrl(const QString &fileName) const
{
    Q_D(const QQuickStylePlugin);
    if (!d->selector) {
        d->selector.reset(new QQuickStyleSelector);
        const QString style = QQuickStyle::name();
        if (!style.isEmpty())
            d->selector->addSelector(style);

        const QString fallback = QQuickStylePrivate::fallbackStyle();
        if (!fallback.isEmpty() && fallback != style)
            d->selector->addSelector(fallback);

        const QString theme = name();
        if (!theme.isEmpty() && theme != style)
            d->selector->addSelector(theme);

        d->selector->setPaths(QQuickStylePrivate::stylePaths(true));
    }
    return d->selector->select(fileName);
}

QT_END_NAMESPACE
