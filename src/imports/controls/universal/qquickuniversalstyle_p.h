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

#ifndef QQUICKUNIVERSALSTYLE_P_H
#define QQUICKUNIVERSALSTYLE_P_H

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

#include <QtGui/qcolor.h>
#include <QtLabsControls/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

class QQuickUniversalStylePrivate;

class QQuickUniversalStyle : public QQuickStyle
{
    Q_OBJECT
    Q_PROPERTY(Theme theme READ theme WRITE setTheme RESET resetTheme NOTIFY themeChanged FINAL)
    Q_PROPERTY(QVariant accent READ accent WRITE setAccent RESET resetAccent NOTIFY accentChanged FINAL)

    Q_PROPERTY(QColor altHighColor READ altHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altLowColor READ altLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumColor READ altMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumHighColor READ altMediumHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumLowColor READ altMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseHighColor READ baseHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseLowColor READ baseLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumColor READ baseMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumHighColor READ baseMediumHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumLowColor READ baseMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeAltLowColor READ chromeAltLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackHighColor READ chromeBlackHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackLowColor READ chromeBlackLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackMediumLowColor READ chromeBlackMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackMediumColor READ chromeBlackMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeDisabledHighColor READ chromeDisabledHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeDisabledLowColor READ chromeDisabledLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeHighColor READ chromeHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeLowColor READ chromeLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeMediumColor READ chromeMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeMediumLowColor READ chromeMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeWhiteColor READ chromeWhiteColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor listLowColor READ listLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor listMediumColor READ listMediumColor NOTIFY paletteChanged FINAL)

public:
    explicit QQuickUniversalStyle(QObject *parent = Q_NULLPTR);

    static QQuickUniversalStyle *qmlAttachedProperties(QObject *object);

    enum Theme { Light, Dark };
    Q_ENUM(Theme)

    Theme theme() const;
    void setTheme(Theme theme);
    void inheritTheme(Theme theme);
    void propagateTheme();
    void resetTheme();

    enum Accent {
        Lime,
        Green,
        Emerald,
        Teal,
        Cyan,
        Cobalt,
        Indigo,
        Violet,
        Pink,
        Magenta,
        Crimson,
        Red,
        Orange,
        Amber,
        Yellow,
        Brown,
        Olive,
        Steel,
        Mauve,
        Taupe
    };
    Q_ENUM(Accent)

    QVariant accent() const;
    void setAccent(const QVariant &accent);
    void inheritAccent(QRgb accent);
    void propagateAccent();
    void resetAccent();

    QColor altHighColor() const;
    QColor altLowColor() const;
    QColor altMediumColor() const;
    QColor altMediumHighColor() const;
    QColor altMediumLowColor() const;
    QColor baseHighColor() const;
    QColor baseLowColor() const;
    QColor baseMediumColor() const;
    QColor baseMediumHighColor() const;
    QColor baseMediumLowColor() const;
    QColor chromeAltLowColor() const;
    QColor chromeBlackHighColor() const;
    QColor chromeBlackLowColor() const;
    QColor chromeBlackMediumLowColor() const;
    QColor chromeBlackMediumColor() const;
    QColor chromeDisabledHighColor() const;
    QColor chromeDisabledLowColor() const;
    QColor chromeHighColor() const;
    QColor chromeLowColor() const;
    QColor chromeMediumColor() const;
    QColor chromeMediumLowColor() const;
    QColor chromeWhiteColor() const;
    QColor listLowColor() const;
    QColor listMediumColor() const;

    enum SystemColor {
        AltHigh,
        AltLow,
        AltMedium,
        AltMediumHigh,
        AltMediumLow,
        BaseHighColor,
        BaseLow,
        BaseMedium,
        BaseMediumHigh,
        BaseMediumLow,
        ChromeAltLow,
        ChromeBlackHigh,
        ChromeBlackLow,
        ChromeBlackMediumLow,
        ChromeBlackMedium,
        ChromeDisabledHigh,
        ChromeDisabledLow,
        ChromeHigh,
        ChromeLow,
        ChromeMedium,
        ChromeMediumLow,
        ChromeWhite,
        ListLow,
        ListMedium
    };

    QColor getColor(SystemColor role) const;

Q_SIGNALS:
    void themeChanged();
    void accentChanged();
    void paletteChanged();

protected:
    void parentStyleChange(QQuickStyle *newParent, QQuickStyle *oldParent) Q_DECL_OVERRIDE;

private:
    void init();

    bool m_hasTheme;
    bool m_hasAccent;
    QQuickUniversalStyle::Theme m_theme;
    QRgb m_accent;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickUniversalStyle, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKUNIVERSALSTYLE_P_H
