// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MYSTYLE_H
#define MYSTYLE_H

#include <QColor>
#include <QQmlEngine>
#include <QQuickAttachedPropertyPropagator>

#include "mystyle_export.h"

class MYSTYLE_EXPORT MyStyle : public QQuickAttachedPropertyPropagator
{
    Q_OBJECT
    // Provide a RESET function in order to allow an item to set MyStyle.theme to undefined
    // in order to use its parent's (or global) theme after one was explicitly set on it.
    Q_PROPERTY(Theme theme READ theme WRITE setTheme RESET resetTheme NOTIFY themeChanged FINAL)
    // As the values of these properties only depend on the theme, they can all use the theme
    // property's change signal.
    Q_PROPERTY(QColor windowColor READ windowColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor windowTextColor READ windowTextColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor buttonColor READ buttonColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor buttonTextColor READ buttonTextColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor toolBarColor READ toolBarColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor popupColor READ popupColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor popupBorderColor READ popupBorderColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor backgroundDimColor READ backgroundDimColor NOTIFY themeChanged FINAL)

    QML_ELEMENT
    QML_ATTACHED(MyStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(1, 0)

public:
    enum Theme {
        Light,
        Dark
    };

    Q_ENUM(Theme)

    explicit MyStyle(QObject *parent = nullptr);

    static MyStyle *qmlAttachedProperties(QObject *object);

    Theme theme() const;
    void setTheme(Theme theme);
    void inheritTheme(Theme theme);
    void propagateTheme();
    void resetTheme();
    void themeChange();

    QColor windowColor() const;
    QColor windowTextColor() const;
    QColor buttonColor() const;
    QColor buttonTextColor() const;
    QColor toolBarColor() const;
    QColor popupColor() const;
    QColor popupBorderColor() const;
    QColor backgroundDimColor() const;

Q_SIGNALS:
    void themeChanged();

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent) override;

private:
    // Whether a color value was explicitly set on the specific object that this attached style object represents.
    bool m_explicitTheme = false;
    // The actual values for this item, whether explicit, inherited or globally set.
    Theme m_theme = Light;
};

#endif // MYSTYLE_H
