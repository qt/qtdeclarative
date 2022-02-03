/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKABSTRACTCOLORPICKER_P_H
#define QQUICKABSTRACTCOLORPICKER_P_H

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
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickAbstractColorPickerPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickAbstractColorPicker : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY colorChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY colorChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY colorChanged)
    Q_PROPERTY(qreal lightness READ lightness WRITE setLightness NOTIFY colorChanged)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQuickItem *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHandleHeight READ implicitHandleHeight NOTIFY implicitHandleHeightChanged FINAL)
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,handle")
    QML_NAMED_ELEMENT(AbstractColorPicker)
    QML_ADDED_IN_VERSION(6, 4)
    QML_UNCREATABLE("AbstractColorPicker is abstract.")

public:
    QColor color() const;
    void setColor(const QColor &c);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    qreal value() const;
    void setValue(qreal value);

    qreal lightness() const;
    void setLightness(qreal lightness);

    qreal alpha() const;
    void setAlpha(qreal alpha);

    bool isPressed() const;
    void setPressed(bool pressed);

    QQuickItem *handle() const;
    void setHandle(QQuickItem *handle);

    qreal implicitHandleWidth() const;
    qreal implicitHandleHeight() const;

Q_SIGNALS:
    void colorChanged(const QColor &color);
    void pressedChanged();
    void handleChanged();
    void implicitHandleWidthChanged();
    void implicitHandleHeightChanged();

    void colorPicked(const QColor &color);

protected:
    QQuickAbstractColorPicker(QQuickAbstractColorPickerPrivate &dd, QQuickItem *parent);

    virtual QColor colorAt(const QPointF &pos) = 0;
    void componentComplete() override;

private:
    void updateColor(const QPointF &pos);
    static std::pair<qreal, qreal> getSaturationAndValue(qreal saturation, qreal lightness);
    static std::pair<qreal, qreal> getSaturationAndLightness(qreal saturation, qreal value);
    Q_DISABLE_COPY(QQuickAbstractColorPicker)
    Q_DECLARE_PRIVATE(QQuickAbstractColorPicker)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCOLORPICKER_P_H
