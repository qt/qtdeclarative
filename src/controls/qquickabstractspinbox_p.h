/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#ifndef QQUICKABSTRACTSPINBOX_P_H
#define QQUICKABSTRACTSPINBOX_P_H

#include <QtQuickControls/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QValidator;
class QQuickTextInput;
class QQuickAbstractSpinBoxPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractSpinBox : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal step READ step WRITE setStep NOTIFY stepChanged FINAL)
    Q_PROPERTY(QQuickTextInput *input READ input WRITE setInput NOTIFY inputChanged FINAL)
    Q_PROPERTY(QValidator *validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL)
    Q_PROPERTY(QQuickItem *upButton READ upButton WRITE setUpButton NOTIFY upButtonChanged FINAL)
    Q_PROPERTY(QQuickItem *downButton READ downButton WRITE setDownButton NOTIFY downButtonChanged FINAL)
    Q_PROPERTY(SubControl pressed READ pressed WRITE setPressed NOTIFY pressedChanged FINAL)

public:
    explicit QQuickAbstractSpinBox(QQuickItem *parent = Q_NULLPTR);

    qreal value() const;
    void setValue(qreal value);

    qreal step() const;
    void setStep(qreal step);

    QQuickTextInput *input() const;
    void setInput(QQuickTextInput *input);

    QValidator *validator() const;
    void setValidator(QValidator *validator);

    QQuickItem *upButton() const;
    void setUpButton(QQuickItem *button);

    QQuickItem *downButton() const;
    void setDownButton(QQuickItem *button);

    enum SubControl {
        None,
        UpButton,
        DownButton
    };
    Q_ENUM(SubControl)

    SubControl pressed() const;
    void setPressed(SubControl control);

public Q_SLOTS:
    void increment();
    void decrement();

Q_SIGNALS:
    void valueChanged();
    void stepChanged();
    void inputChanged();
    void validatorChanged();
    void upButtonChanged();
    void downButtonChanged();
    void pressedChanged();

protected:
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseUngrabEvent() Q_DECL_OVERRIDE;

    void componentComplete() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractSpinBox)
    Q_DECLARE_PRIVATE(QQuickAbstractSpinBox)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTSPINBOX_P_H
