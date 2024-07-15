// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQuickControls2Material/private/qquickmaterialstyle_p.h>
#include <QtQuickTest/quicktest.h>

namespace {
static inline Qt::ColorScheme toColorScheme(QQuickMaterialStyle::Theme theme)
{
    switch (theme) {
    case QQuickMaterialStyle::Theme::System:
        return Qt::ColorScheme::Unknown;
    case QQuickMaterialStyle::Light:
        return Qt::ColorScheme::Light;
    case QQuickMaterialStyle::Dark:
        return Qt::ColorScheme::Dark;
    default:
        Q_UNREACHABLE_RETURN(Qt::ColorScheme::Unknown);
    }
}

static inline QQuickMaterialStyle::Theme toMaterialTheme(Qt::ColorScheme colorScheme)
{
    switch (colorScheme) {
    case Qt::ColorScheme::Unknown:
        return QQuickMaterialStyle::System;
    case Qt::ColorScheme::Light:
        return QQuickMaterialStyle::Light;
    case Qt::ColorScheme::Dark:
        return QQuickMaterialStyle::Dark;
    default:
        Q_UNREACHABLE_RETURN(QQuickMaterialStyle::System);
    }
}
}


class MockPlatformTheme : public QPlatformTheme
{
    Qt::ColorScheme colorScheme() const override
    {
        return m_colorScheme;
    }
    void requestColorScheme(Qt::ColorScheme theme) override
    {
        m_colorScheme = theme;
        QWindowSystemInterfacePrivate::ThemeChangeEvent tce{nullptr};
        QGuiApplicationPrivate::processThemeChanged(&tce);
    }

private:
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
};


class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickMaterialStyle::Theme platformTheme READ platformTheme WRITE setPlatformTheme CONSTANT FINAL)

public slots:
    void applicationAvailable()
    {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
    }

    void qmlEngineAvailable(QQmlEngine*)
    {
        qmlRegisterSingletonInstance("Qt.test", 1, 0, "TestHelper", this);

        QGuiApplicationPrivate::platform_theme = new MockPlatformTheme;
    }

public:
    void setPlatformTheme(QQuickMaterialStyle::Theme theme)
    {
        QGuiApplicationPrivate::platform_theme->requestColorScheme(toColorScheme(theme));
    }

    QQuickMaterialStyle::Theme platformTheme() const
    {
        return toMaterialTheme(QGuiApplicationPrivate::platform_theme->colorScheme());
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qquickmaterialstyle, Setup)

#include "tst_qquickmaterialstyle.moc"
