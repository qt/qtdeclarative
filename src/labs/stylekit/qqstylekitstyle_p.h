// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITSTYLE_P_H
#define QQSTYLEKITSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqstylekitreader_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitcustomtheme_p.h"
#include "qqstylekitdebug_p.h"
#include "qqstylekitstyleandthemebase_p.h"

#include <QtQml/QtQml>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>

QT_BEGIN_NAMESPACE

class QQStyleKitTheme;
class QQStyleKitPropertyResolver;

class QQStyleKitStyle : public QQStyleKitStyleAndThemeBase
{
    Q_OBJECT
    Q_PROPERTY(QQuickPalette *palette READ palette NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QQStyleKitStyle *fallbackStyle READ fallbackStyle WRITE setFallbackStyle NOTIFY fallbackStyleChanged FINAL)
    Q_PROPERTY(QQmlComponent *light READ light WRITE setLight NOTIFY lightChanged FINAL)
    Q_PROPERTY(QQmlComponent *dark READ dark WRITE setDark NOTIFY darkChanged FINAL)
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged FINAL)
    Q_PROPERTY(QStringList themeNames READ themeNames NOTIFY themeNamesChanged FINAL)
    Q_PROPERTY(QStringList customThemeNames READ customThemeNames NOTIFY customThemeNamesChanged FINAL)
    Q_PROPERTY(QQStyleKitTheme *theme READ theme NOTIFY themeChanged FINAL)

    Q_CLASSINFO("DeferredPropertyNames", "fallbackStyle")
    QML_NAMED_ELEMENT(BaseStyle)

public:
    enum Constants {
        Stretch = -1, // Use all available space
    };
    Q_ENUM(Constants)

    QQStyleKitStyle(QObject *parent = nullptr);
    ~QQStyleKitStyle();

    QQuickPalette *palette() const;

    QQStyleKitStyle *fallbackStyle() const;
    void setFallbackStyle(QQStyleKitStyle *fallbackStyle);

    QQmlComponent *light() const;
    void setLight(QQmlComponent *lightTheme);

    QQmlComponent *dark() const;
    void setDark(QQmlComponent *darkTheme);

    QList<QQStyleKitCustomTheme *> customThemes() const;
    QStringList themeNames() const;
    QStringList customThemeNames() const;

    void setThemeName(const QString &themeName);
    QString themeName() const;
    QQStyleKitTheme *theme() const;

    bool loaded() const;

    static QQStyleKitStyle *current();

    QPalette paletteForControlType(QQStyleKitExtendableControlType type) const;
    QFont fontForControlType(QQStyleKitExtendableControlType type) const;

    // For now, used by qqcontrolstowidgetstyle
    Q_INVOKABLE QList<QObject *> customThemesAsList();

signals:
    void paletteChanged();
    void fallbackStyleChanged();
    void lightChanged();
    void darkChanged();
    void themeChanged();
    void themeNameChanged();
    void themeNamesChanged();
    void customThemeNamesChanged();

protected:
    void componentComplete() override;

private:
    void parseThemes();
    void recreateTheme();
    void executeFallbackStyle(bool complete = false);
    void syncFromQPalette(const QPalette &palette);
    QPalette effectivePalette() const;

    Q_DISABLE_COPY(QQStyleKitStyle)

    bool m_completed = false;
    bool m_isUpdatingPalette = false;

    QQuickDeferredPointer<QQStyleKitStyle> m_fallbackStyle;
    QPointer<QQmlComponent> m_light;
    QPointer<QQmlComponent> m_dark;
    QPointer<QQStyleKitTheme> m_theme;
    QPointer<QQmlComponent> m_currentThemeComponent;
    QQuickPalette *m_paletteProxy = nullptr;
    QPalette m_effectivePalette;
    QPointer<QQuickPalette> m_palette;
    QString m_themeName;
    QString m_effectiveThemeName;
    QStringList m_themeNames;
    QStringList m_customThemeNames;

    friend class QQStyleKitAttached;
    friend class QQStyleKitPropertyGroup;
    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitDebug;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITSTYLE_P_H
