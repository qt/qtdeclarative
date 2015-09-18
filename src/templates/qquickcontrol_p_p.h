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

#ifndef QQUICKCONTROL_P_P_H
#define QQUICKCONTROL_P_P_H

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

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickAccessibleAttached;

class Q_QUICKTEMPLATES_EXPORT QQuickControlPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickControl)

public:
    QQuickControlPrivate();

    static QQuickControlPrivate *get(QQuickControl *control)
    {
        return control->d_func();
    }

    void mirrorChange() Q_DECL_OVERRIDE;

    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);

    void resizeBackground();
    void resizeContent();

    void updateFont(const QFont &);
    static void updateFontRecur(QQuickItem *item, const QFont &);
    inline void setFont_helper(const QFont &f) {
        if (font.resolve() == f.resolve() && font == f)
            return;
        updateFont(f);
    }
    void resolveFont();
    static QFont naturalControlFont(const QQuickItem *);

    QFont font;
    bool hasTopPadding;
    bool hasLeftPadding;
    bool hasRightPadding;
    bool hasBottomPadding;
    qreal padding;
    qreal topPadding;
    qreal leftPadding;
    qreal rightPadding;
    qreal bottomPadding;
    qreal spacing;
    Qt::LayoutDirection layoutDirection;
    QQuickItem *background;
    QQuickItem *contentItem;
    QQuickAccessibleAttached *accessibleAttached;
    int accessibleRole;
};

Q_DECLARE_TYPEINFO(QQuickControlPrivate, Q_COMPLEX_TYPE);

QT_END_NAMESPACE

#endif // QQUICKCONTROL_P_P_H
