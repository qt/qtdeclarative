/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#ifndef QQUICKABSTRACTBUTTON_P_P_H
#define QQUICKABSTRACTBUTTON_P_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQuickButtonGroup;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickAbstractButtonPrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickAbstractButton)

    static QQuickAbstractButtonPrivate *get(QQuickAbstractButton *button)
    {
        return button->d_func();
    }

    void setPressPoint(const QPointF &point);
    void setMovePoint(const QPointF &point);

    void handlePress(const QPointF &point, ulong timestamp) override;
    void handleMove(const QPointF &point, ulong timestamp) override;
    void handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    virtual bool acceptKeyClick(Qt::Key key) const;

    bool isPressAndHoldConnected();
    bool isDoubleClickConnected();
    void startPressAndHold();
    void stopPressAndHold();

    void startRepeatDelay();
    void startPressRepeat();
    void stopPressRepeat();

#if QT_CONFIG(shortcut)
    void grabShortcut();
    void ungrabShortcut();
#endif

    QQuickAbstractButton *findCheckedButton() const;
    QList<QQuickAbstractButton *> findExclusiveButtons() const;

    void actionTextChange();
    void setText(const QString &text, bool isExplicit);

    void updateEffectiveIcon();

    void click();
    void trigger(bool doubleClick = false);
    void toggle(bool value);

    void cancelIndicator();
    void executeIndicator(bool complete = false);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    // copied from qabstractbutton.cpp
    static const int AUTO_REPEAT_DELAY = 300;
    static const int AUTO_REPEAT_INTERVAL = 100;

    bool explicitText = false;
    bool down = false;
    bool explicitDown = false;
    bool pressed = false;
    bool keepPressed = false;
    bool checked = false;
    bool checkable = false;
    bool autoExclusive = false;
    bool autoRepeat = false;
    bool wasHeld = false;
    bool wasDoubleClick = false;
    int holdTimer = 0;
    int delayTimer = 0;
    int repeatTimer = 0;
    int repeatDelay = AUTO_REPEAT_DELAY;
    int repeatInterval = AUTO_REPEAT_INTERVAL;
#if QT_CONFIG(shortcut)
    int shortcutId = 0;
    QKeySequence shortcut;
#endif
    qreal lastTouchReleaseTimestamp = 0;
    QString text;
    QQuickIcon icon;
    QQuickIcon effectiveIcon;
    QPointF pressPoint;
    QPointF movePoint;
    Qt::MouseButtons pressButtons = Qt::NoButton;
    QQuickAbstractButton::Display display = QQuickAbstractButton::TextBesideIcon;
    QQuickDeferredPointer<QQuickItem> indicator;
    QQuickButtonGroup *group = nullptr;
    QPointer<QQuickAction> action;
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTBUTTON_P_P_H
