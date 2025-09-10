// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKBASICSTYLE_P_H
#define QQUICKBASICSTYLE_P_H

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

#include <QtCore/qobject.h>
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Basic/qtquickcontrols2basicexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2BASIC_EXPORT QQuickBasicStyle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor CONSTANT FINAL)
    Q_PROPERTY(QColor overlayModalColor READ overlayModalColor CONSTANT FINAL)
    Q_PROPERTY(QColor overlayDimColor READ overlayDimColor CONSTANT FINAL)
    Q_PROPERTY(QColor textColor READ textColor CONSTANT FINAL)
    Q_PROPERTY(QColor textDarkColor READ textDarkColor CONSTANT FINAL)
    Q_PROPERTY(QColor textLightColor READ textLightColor CONSTANT FINAL)
    Q_PROPERTY(QColor textLinkColor READ textLinkColor CONSTANT FINAL)
    Q_PROPERTY(QColor textSelectionColor READ textSelectionColor CONSTANT FINAL)
    Q_PROPERTY(QColor textDisabledColor READ textDisabledColor CONSTANT FINAL)
    Q_PROPERTY(QColor textDisabledLightColor READ textDisabledLightColor CONSTANT FINAL)
    Q_PROPERTY(QColor textPlaceholderColor READ textPlaceholderColor CONSTANT FINAL)
    Q_PROPERTY(QColor focusColor READ focusColor CONSTANT FINAL)
    Q_PROPERTY(QColor focusLightColor READ focusLightColor CONSTANT FINAL)
    Q_PROPERTY(QColor focusPressedColor READ focusPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor buttonColor READ buttonColor CONSTANT FINAL)
    Q_PROPERTY(QColor buttonPressedColor READ buttonPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor buttonCheckedColor READ buttonCheckedColor CONSTANT FINAL)
    Q_PROPERTY(QColor buttonCheckedPressedColor READ buttonCheckedPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor buttonCheckedFocusColor READ buttonCheckedFocusColor CONSTANT FINAL)
    Q_PROPERTY(QColor toolButtonColor READ toolButtonColor CONSTANT FINAL)
    Q_PROPERTY(QColor tabButtonColor READ tabButtonColor CONSTANT FINAL)
    Q_PROPERTY(QColor tabButtonPressedColor READ tabButtonPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor tabButtonCheckedPressedColor READ tabButtonCheckedPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor delegateColor READ delegateColor CONSTANT FINAL)
    Q_PROPERTY(QColor delegatePressedColor READ delegatePressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor delegateFocusColor READ delegateFocusColor CONSTANT FINAL)
    Q_PROPERTY(QColor indicatorPressedColor READ indicatorPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor indicatorDisabledColor READ indicatorDisabledColor CONSTANT FINAL)
    Q_PROPERTY(QColor indicatorFrameColor READ indicatorFrameColor CONSTANT FINAL)
    Q_PROPERTY(QColor indicatorFramePressedColor READ indicatorFramePressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor indicatorFrameDisabledColor READ indicatorFrameDisabledColor CONSTANT FINAL)
    Q_PROPERTY(QColor frameDarkColor READ frameDarkColor CONSTANT FINAL)
    Q_PROPERTY(QColor frameLightColor READ frameLightColor CONSTANT FINAL)
    Q_PROPERTY(QColor scrollBarColor READ scrollBarColor CONSTANT FINAL)
    Q_PROPERTY(QColor scrollBarPressedColor READ scrollBarPressedColor CONSTANT FINAL)
    Q_PROPERTY(QColor progressBarColor READ progressBarColor CONSTANT FINAL)
    Q_PROPERTY(QColor pageIndicatorColor READ pageIndicatorColor CONSTANT FINAL)
    Q_PROPERTY(QColor separatorColor READ separatorColor CONSTANT FINAL)
    Q_PROPERTY(QColor disabledDarkColor READ disabledDarkColor CONSTANT FINAL)
    Q_PROPERTY(QColor disabledLightColor READ disabledLightColor CONSTANT FINAL)
    QML_NAMED_ELEMENT(Basic)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickBasicStyle(QObject *parent = nullptr);

    QColor backgroundColor() const;
    QColor overlayModalColor() const;
    QColor overlayDimColor() const;
    QColor textColor() const;
    QColor textDarkColor() const;
    QColor textLightColor() const;
    QColor textLinkColor() const;
    QColor textSelectionColor() const;
    QColor textDisabledColor() const;
    QColor textDisabledLightColor() const;
    QColor textPlaceholderColor() const;
    QColor focusColor() const;
    QColor focusLightColor() const;
    QColor focusPressedColor() const;
    QColor buttonColor() const;
    QColor buttonPressedColor() const;
    QColor buttonCheckedColor() const;
    QColor buttonCheckedPressedColor() const;
    QColor buttonCheckedFocusColor() const;
    QColor toolButtonColor() const;
    QColor tabButtonColor() const;
    QColor tabButtonPressedColor() const;
    QColor tabButtonCheckedPressedColor() const;
    QColor delegateColor() const;
    QColor delegatePressedColor() const;
    QColor delegateFocusColor() const;
    QColor indicatorPressedColor() const;
    QColor indicatorDisabledColor() const;
    QColor indicatorFrameColor() const;
    QColor indicatorFramePressedColor() const;
    QColor indicatorFrameDisabledColor() const;
    QColor frameDarkColor() const;
    QColor frameLightColor() const;
    QColor scrollBarColor() const;
    QColor scrollBarPressedColor() const;
    QColor progressBarColor() const;
    QColor pageIndicatorColor() const;
    QColor separatorColor() const;
    QColor disabledDarkColor() const;
    QColor disabledLightColor() const;
};

QT_END_NAMESPACE

#endif // QQUICKBASICSTYLE_P_H
