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

#ifndef QQUICKTEXTFIELD_P_H
#define QQUICKTEXTFIELD_P_H

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

#include <QtQuick/private/qquicktextinput_p.h>
#include <QtLabsTemplates/private/qtlabstemplatesglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickText;
class QQuickTextFieldPrivate;
class QQuickMouseEvent;

class Q_LABSTEMPLATES_EXPORT QQuickTextField : public QQuickTextInput
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged) // override
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged FINAL)
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason WRITE setFocusReason NOTIFY focusReasonChanged FINAL)

public:
    explicit QQuickTextField(QQuickItem *parent = Q_NULLPTR);
    ~QQuickTextField();

    QFont font() const;
    void setFont(const QFont &font);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    Qt::FocusReason focusReason() const;
    void setFocusReason(Qt::FocusReason reason);

Q_SIGNALS:
    void fontChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void backgroundChanged();
    void placeholderTextChanged();
    void focusReasonChanged();
    void pressAndHold(QQuickMouseEvent *mouse);

protected:
    void classBegin() Q_DECL_OVERRIDE;

    void itemChange(ItemChange change, const ItemChangeData &value) Q_DECL_OVERRIDE;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) Q_DECL_OVERRIDE;

    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickTextField)
    Q_DECLARE_PRIVATE(QQuickTextField)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextField)

#endif // QQUICKTEXTFIELD_P_H
