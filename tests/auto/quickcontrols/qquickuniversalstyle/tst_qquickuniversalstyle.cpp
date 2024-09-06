// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/private/qguiapplication_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControls2Universal/private/qquickuniversalstyle_p.h>

namespace {
static inline Qt::ColorScheme toColorScheme(QQuickUniversalStyle::Theme theme)
{
    switch (theme) {
    case QQuickUniversalStyle::Theme::System:
        return Qt::ColorScheme::Unknown;
    case QQuickUniversalStyle::Light:
        return Qt::ColorScheme::Light;
    case QQuickUniversalStyle::Dark:
        return Qt::ColorScheme::Dark;
    default:
        Q_UNREACHABLE_RETURN(Qt::ColorScheme::Unknown);
    }
}

static inline QQuickUniversalStyle::Theme toMaterialTheme(Qt::ColorScheme colorScheme)
{
    switch (colorScheme) {
    case Qt::ColorScheme::Unknown:
        return QQuickUniversalStyle::System;
    case Qt::ColorScheme::Light:
        return QQuickUniversalStyle::Light;
    case Qt::ColorScheme::Dark:
        return QQuickUniversalStyle::Dark;
    default:
        Q_UNREACHABLE_RETURN(QQuickUniversalStyle::System);
    }
}
}

class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickUniversalStyle::Theme platformTheme READ platformTheme WRITE setPlatformTheme FINAL)

public slots:
    void applicationAvailable()
    {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
    }

    void qmlEngineAvailable(QQmlEngine*)
    {
        qmlRegisterSingletonInstance("Qt.test", 1, 0, "TestHelper", this);

        QGuiApplicationPrivate::platform_theme = new QQuickControlsTestUtils::MockPlatformTheme;
    }

public:
    QQuickUniversalStyle::Theme platformTheme() const
    {
        return toMaterialTheme(QGuiApplicationPrivate::platform_theme->colorScheme());
    }

    void setPlatformTheme(QQuickUniversalStyle::Theme theme)
    {
        QGuiApplicationPrivate::platform_theme->requestColorScheme(toColorScheme(theme));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qquickuniversalstyle, Setup)

#include "tst_qquickuniversalstyle.moc"
