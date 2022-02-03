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

#ifndef QQUICKCOLORDIALOGIMPL_P_P_H
#define QQUICKCOLORDIALOGIMPL_P_P_H

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

#include "qquickcolordialogimpl_p.h"
#include "qquickabstractcolorpicker_p.h"

#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>

QT_BEGIN_NAMESPACE

class QQuickEyeDropperEventFilter : public QObject
{
public:
    enum class LeaveReason { Default, Cancel };
    explicit QQuickEyeDropperEventFilter(std::function<void(QPoint, LeaveReason)> callOnLeave,
                                         std::function<void(QPoint)> callOnUpdate)
        : m_leave(callOnLeave), m_update(callOnUpdate)
    {
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    std::function<void(QPoint, LeaveReason)> m_leave;
    std::function<void(QPoint)> m_update;
    QPoint m_lastPosition;
};

class QQuickColorDialogImplPrivate : public QQuickDialogPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickColorDialogImpl);

public:
    explicit QQuickColorDialogImplPrivate();
    ~QQuickColorDialogImplPrivate();

    static QQuickColorDialogImplPrivate *get(QQuickColorDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickColorDialogImplAttached *attachedOrWarn();

    void handleClick(QQuickAbstractButton *button) override;

    void eyeDropperEnter();
    void eyeDropperLeave(const QPoint &pos, QQuickEyeDropperEventFilter::LeaveReason actionOnLeave);
    void eyeDropperPointerMoved(const QPoint &pos);

    void alphaSliderMoved();

    QSharedPointer<QColorDialogOptions> options;
    struct
    {
        qreal h = .0;
        qreal s = .0;
        union {
            qreal v = 1.0;
            qreal l;
        };
        qreal a = 1.0;
    } m_hsva;
    std::unique_ptr<QQuickEyeDropperEventFilter> eyeDropperEventFilter;
    QPointer<QQuickWindow> m_eyeDropperWindow;
    QColor m_eyeDropperPreviousColor;
    bool m_eyeDropperMode = false;
    bool m_showAlpha = false;
    bool m_hsl = false;
};

class QQuickColorDialogImplAttachedPrivate : public QObjectPrivate
{
public:
    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickAbstractButton> eyeDropperButton;
    QPointer<QQuickAbstractColorPicker> colorPicker;
    QPointer<QQuickSlider> alphaSlider;

    Q_DECLARE_PUBLIC(QQuickColorDialogImplAttached)
};

QT_END_NAMESPACE

#endif // QQUICKCOLORDIALOGIMPL_P_P_H
