// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mystyle.h"

// If no value was inherited from a parent or explicitly set, the "global" values are used.
static MyStyle::Theme globalTheme = MyStyle::Light;

MyStyle::MyStyle(QObject *parent)
    : QQuickAttachedPropertyPropagator(parent)
    , m_theme(globalTheme)
{
    // A static function could be called here that reads globalTheme from a
    // settings file once at startup. That value would override the global
    // value. This is similar to what the Imagine and Material styles do, for
    // example.

    initialize();
}

MyStyle *MyStyle::qmlAttachedProperties(QObject *object)
{
    return new MyStyle(object);
}

MyStyle::Theme MyStyle::theme() const
{
    return m_theme;
}

void MyStyle::setTheme(Theme theme)
{
    // When this function is called, we know that the user has explicitly
    // set a theme on this attached object. We set this to true even if
    // the effective theme didn't change, because it's important that
    // the user's specified value is respected (and not inherited from
    // from the parent).
    m_explicitTheme = true;
    if (m_theme == theme)
        return;

    m_theme = theme;
    propagateTheme();
    themeChange();

}

void MyStyle::inheritTheme(Theme theme)
{
    if (m_explicitTheme || m_theme == theme)
        return;

    m_theme = theme;
    propagateTheme();
    themeChange();
}

void MyStyle::propagateTheme()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        MyStyle *myStyle = qobject_cast<MyStyle *>(child);
        if (myStyle)
            myStyle->inheritTheme(m_theme);
    }
}

void MyStyle::resetTheme()
{
    if (!m_explicitTheme)
        return;

    m_explicitTheme = false;
    MyStyle *myStyle = qobject_cast<MyStyle *>(attachedParent());
    inheritTheme(myStyle ? myStyle->theme() : globalTheme);
}

void MyStyle::themeChange()
{
    emit themeChanged();
    // Emit any other change signals for properties that depend on the theme here...
}

QColor MyStyle::windowColor() const
{
    return m_theme == Light ? QColor::fromRgb(0xf0f0f0) : QColor::fromRgb(0x303030);
}

QColor MyStyle::windowTextColor() const
{
    return m_theme == Light ? QColor::fromRgb(0x5c5c5c) : QColor::fromRgb(0xe0e0e0);
}

QColor MyStyle::buttonColor() const
{
    return m_theme == Light ? QColor::fromRgb(0xc2e1ff) : QColor::fromRgb(0x74bbff);
}

QColor MyStyle::buttonTextColor() const
{
    return m_theme == Light ? QColor::fromRgb(0x5c5c5c) : QColor::fromRgb(0xffffff);
}

QColor MyStyle::toolBarColor() const
{
    return m_theme == Light ? QColor::fromRgb(0x4da6ff) : QColor::fromRgb(0x0066cc);
}

QColor MyStyle::popupColor() const
{
    return windowColor().lighter(120);
}

QColor MyStyle::popupBorderColor() const
{
    const QColor winColor = windowColor();
    return m_theme == Light ? winColor.darker(140) : winColor.lighter(140);
}

QColor MyStyle::backgroundDimColor() const
{
    const QColor winColor = windowColor().darker();
    return QColor::fromRgb(winColor.red(), winColor.green(), winColor.blue(), 100);
}

void MyStyle::attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent)
{
    Q_UNUSED(oldParent);
    MyStyle *attachedParentStyle = qobject_cast<MyStyle *>(newParent);
    if (attachedParentStyle) {
        inheritTheme(attachedParentStyle->theme());
        // Do any other inheriting here...
    }
}
