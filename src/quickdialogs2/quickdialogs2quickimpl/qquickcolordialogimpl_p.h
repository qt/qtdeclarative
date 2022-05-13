// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKCOLORDIALOGIMPL_P_H
#define QQUICKCOLORDIALOGIMPL_P_H

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

#include <QtQuickTemplates2/private/qquickdialog_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickDialogButtonBox;
class QQuickAbstractColorPicker;
class QQuickSlider;

class QQuickColorDialogImplAttached;
class QQuickColorDialogImplAttachedPrivate;
class QQuickColorDialogImplPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickColorDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY colorChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY colorChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY colorChanged)
    Q_PROPERTY(qreal lightness READ lightness WRITE setLightness NOTIFY colorChanged)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY colorChanged FINAL)
    Q_PROPERTY(int red READ red WRITE setRed NOTIFY colorChanged FINAL)
    Q_PROPERTY(int green READ green WRITE setGreen NOTIFY colorChanged FINAL)
    Q_PROPERTY(int blue READ blue WRITE setBlue NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool isHsl READ isHsl WRITE setHsl NOTIFY specChanged FINAL)
    Q_PROPERTY(bool showAlpha READ showAlpha NOTIFY showAlphaChanged)
    QML_NAMED_ELEMENT(ColorDialogImpl)
    QML_ATTACHED(QQuickColorDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 4)

public:
    explicit QQuickColorDialogImpl(QObject *parent = nullptr);

    static QQuickColorDialogImplAttached *qmlAttachedProperties(QObject *object);

    QSharedPointer<QColorDialogOptions> options() const;
    void setOptions(const QSharedPointer<QColorDialogOptions> &options);

    QColor color() const;
    void setColor(const QColor &c);

    int red() const;
    void setRed(int red);

    int green() const;
    void setGreen(int green);

    int blue() const;
    void setBlue(int blue);

    qreal alpha() const;
    void setAlpha(qreal alpha);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    qreal value() const;
    void setValue(qreal value);

    qreal lightness() const;
    void setLightness(qreal lightness);

    bool isHsl() const;
    void setHsl(bool hsl);

    bool showAlpha();

    Q_INVOKABLE void invokeEyeDropper();

Q_SIGNALS:
    void colorChanged(const QColor &color);
    void specChanged();
    void showAlphaChanged();

private:
    static std::pair<qreal, qreal> getSaturationAndValue(qreal saturation, qreal lightness);
    static std::pair<qreal, qreal> getSaturationAndLightness(qreal saturation, qreal value);
    Q_DISABLE_COPY(QQuickColorDialogImpl)
    Q_DECLARE_PRIVATE(QQuickColorDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickColorDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox NOTIFY buttonBoxChanged FINAL)
    Q_PROPERTY(QQuickAbstractButton *eyeDropperButton READ eyeDropperButton WRITE setEyeDropperButton NOTIFY eyeDropperButtonChanged FINAL)
    Q_PROPERTY(QQuickAbstractColorPicker *colorPicker READ colorPicker WRITE setColorPicker NOTIFY colorPickerChanged)
    Q_PROPERTY(QQuickSlider *alphaSlider READ alphaSlider WRITE setAlphaSlider NOTIFY alphaSliderChanged)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickabstractbutton_p.h>)
    Q_MOC_INCLUDE(<QtQuickTemplates2/private/qquickslider_p.h>)
    Q_MOC_INCLUDE("qquickabstractcolorpicker_p.h")

public:
    explicit QQuickColorDialogImplAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttonBox);

    QQuickAbstractButton *eyeDropperButton() const;
    void setEyeDropperButton(QQuickAbstractButton *eyeDropperButton);

    QQuickAbstractColorPicker *colorPicker() const;
    void setColorPicker(QQuickAbstractColorPicker *colorPicker);

    QQuickSlider *alphaSlider() const;
    void setAlphaSlider(QQuickSlider *alphaSlider);

Q_SIGNALS:
    void buttonBoxChanged();
    void eyeDropperButtonChanged();
    void colorPickerChanged();
    void alphaSliderChanged();

private:
    Q_DISABLE_COPY(QQuickColorDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickColorDialogImplAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickColorDialogImpl)

#endif // QQUICKCOLORDIALOGIMPL_P_H
