/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
