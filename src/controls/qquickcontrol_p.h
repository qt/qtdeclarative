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

#ifndef QQUICKCONTROL_P_H
#define QQUICKCONTROL_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickControls/private/qtquickcontrolsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyle;
class QQuickControlPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickControl : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(QQuickStyle *style READ style WRITE setStyle RESET resetStyle NOTIFY styleChanged FINAL)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)

public:
    explicit QQuickControl(QQuickItem *parent = Q_NULLPTR);

    qreal topPadding() const;
    void setTopPadding(qreal padding);

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);

    qreal rightPadding() const;
    void setRightPadding(qreal padding);

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);

    QQuickStyle *style() const;
    void setStyle(QQuickStyle *style);
    void resetStyle();

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    enum Attribute {
        Attr_HasStyle = 1,
        Attr_Count
    };

    bool testAttribute(Attribute attribute) const;
    void setAttribute(Attribute attribute, bool on = true);

Q_SIGNALS:
    void styleChanged();
    void backgroundChanged();
    void topPaddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void bottomPaddingChanged();

protected:
    QQuickControl(QQuickControlPrivate &dd, QQuickItem *parent);

    bool event(QEvent *event) Q_DECL_OVERRIDE;
    void itemChange(ItemChange, const ItemChangeData &data) Q_DECL_OVERRIDE;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

    bool isMirrored() const;
    virtual void mirrorChange();

private:
    Q_DISABLE_COPY(QQuickControl)
    Q_DECLARE_PRIVATE(QQuickControl)
};

QT_END_NAMESPACE

#endif // QQUICKCONTROL_P_H
