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

#ifndef QQUICKDRAWER_P_H
#define QQUICKDRAWER_P_H

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

#include <QtLabsTemplates/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickPropertyAnimation;
class QQuickDrawerPrivate;

class Q_LABSTEMPLATES_EXPORT QQuickDrawer : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(Qt::Edge edge READ edge WRITE setEdge NOTIFY edgeChanged FINAL)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)
    // TODO: make this a proper transition
    Q_PROPERTY(QQuickPropertyAnimation *animation READ animation WRITE setAnimation NOTIFY animationChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "contentItem")

public:
    explicit QQuickDrawer(QQuickItem *parent = Q_NULLPTR);

    Qt::Edge edge() const;
    void setEdge(Qt::Edge edge);

    qreal position() const;
    void setPosition(qreal position);

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

    QQuickPropertyAnimation *animation() const;
    void setAnimation(QQuickPropertyAnimation *animation);

public Q_SLOTS:
    void open();
    void close();

Q_SIGNALS:
    void clicked();
    void edgeChanged();
    void positionChanged();
    void contentItemChanged();
    void animationChanged();

protected:
    bool childMouseEventFilter(QQuickItem *child, QEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseUngrabEvent() Q_DECL_OVERRIDE;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    virtual qreal positionAt(const QPointF &point) const;

private:
    Q_DISABLE_COPY(QQuickDrawer)
    Q_DECLARE_PRIVATE(QQuickDrawer)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickDrawer)

#endif // QQUICKDRAWER_P_H
