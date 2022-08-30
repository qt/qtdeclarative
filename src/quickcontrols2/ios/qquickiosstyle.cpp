// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickiosstyle_p.h"

#include <QtCore/qsettings.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QString, GlobalPath, (QLatin1String("qrc:/qt-project.org/imports/QtQuick/Controls/iOS/images/")))

static QQuickIOSStyle::Theme qquickios_effective_theme()
{
    return QQuickStylePrivate::isDarkSystemTheme()
            ? QQuickIOSStyle::Dark
            : QQuickIOSStyle::Light;
}

QQuickIOSStyle::QQuickIOSStyle(QObject *parent)
    : QQuickAttachedObject(parent)
{
    init();
    m_theme = qquickios_effective_theme();
}

QQuickIOSStyle *QQuickIOSStyle::qmlAttachedProperties(QObject *object)
{
    return new QQuickIOSStyle(object);
}

QUrl QQuickIOSStyle::url() const
{
    return *GlobalPath();
}

void QQuickIOSStyle::init()
{
    // static bool globalsInitialized = false;
    // if (!globalsInitialized) {
    //     QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(QStringLiteral("iOS"));
    //     globalsInitialized = true;
    // }
    QQuickAttachedObject::init(); // TODO: lazy init?
}

QQuickIOSStyle::Theme QQuickIOSStyle::theme() const
{
    return m_theme;
}

QT_END_NAMESPACE
