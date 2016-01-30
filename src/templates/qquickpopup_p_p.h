/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#ifndef QQUICKPOPUP_P_P_H
#define QQUICKPOPUP_P_P_H

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

#include "qquickpopup_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickTransition;
class QQuickTransitionManager;
class QQuickPopup;
class QQuickPopupPrivate;
class QQuickPopupItemPrivate;

class QQuickPopupTransitionManager : public QQuickTransitionManager
{
public:
    QQuickPopupTransitionManager(QQuickPopupPrivate *popup);

    void transitionEnter();
    void transitionExit();

protected:
    void finished() Q_DECL_OVERRIDE;

private:
    enum TransitionState {
        Off, Enter, Exit
    };

    TransitionState state;
    QQuickPopupPrivate *popup;
};

class QQuickPopupItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit QQuickPopupItem(QQuickPopup *popup);

protected:
    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseUngrabEvent() Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;
    void itemChange(ItemChange change, const ItemChangeData &data) Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(QQuickPopupItem)
};

class QQuickPopupPositioner : public QQuickItemChangeListener
{
public:
    explicit QQuickPopupPositioner(QQuickPopupPrivate *popup);
    ~QQuickPopupPositioner();

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    QQuickItem *parentItem() const;
    void setParentItem(QQuickItem *parent);

    void repositionPopup();

protected:
    void itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &);
    void itemParentChanged(QQuickItem *, QQuickItem *parent);
    void itemChildRemoved(QQuickItem *, QQuickItem *child);
    void itemDestroyed(QQuickItem *item);

private:
    void removeAncestorListeners(QQuickItem *item);
    void addAncestorListeners(QQuickItem *item);

    bool isAncestor(QQuickItem *item) const;

    qreal m_x;
    qreal m_y;
    QQuickItem *m_parentItem;
    QQuickPopupPrivate *m_popup;
};

class QQuickPopupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopup)

public:
    QQuickPopupPrivate();

    static QQuickPopupPrivate *get(QQuickPopup *popup)
    {
        return popup->d_func();
    }

    void init();
    bool tryClose(QQuickItem *item, QMouseEvent *event);

    void finalizeEnterTransition();
    void finalizeExitTransition();

    void resizeBackground();
    void resizeContent();

    QMarginsF getMargins() const;

    void setTopMargin(qreal value, bool reset = false);
    void setLeftMargin(qreal value, bool reset = false);
    void setRightMargin(qreal value, bool reset = false);
    void setBottomMargin(qreal value, bool reset = false);

    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);

    bool focus;
    bool modal;
    bool complete;
    bool hasTopMargin;
    bool hasLeftMargin;
    bool hasRightMargin;
    bool hasBottomMargin;
    bool hasTopPadding;
    bool hasLeftPadding;
    bool hasRightPadding;
    bool hasBottomPadding;
    qreal margins;
    qreal topMargin;
    qreal leftMargin;
    qreal rightMargin;
    qreal bottomMargin;
    qreal padding;
    qreal topPadding;
    qreal leftPadding;
    qreal rightPadding;
    qreal bottomPadding;
    qreal contentWidth;
    qreal contentHeight;
    QQuickPopup::ClosePolicy closePolicy;
    QQuickItem *parentItem;
    QQuickItem *background;
    QQuickItem *contentItem;
    QQuickTransition *enter;
    QQuickTransition *exit;
    QQuickPopupItem *popupItem;
    QQuickPopupPositioner positioner;
    QQuickPopupTransitionManager transitionManager;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUP_P_P_H

