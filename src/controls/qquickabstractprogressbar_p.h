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

#ifndef QQUICKABSTRACTPROGRESSBAR_P_H
#define QQUICKABSTRACTPROGRESSBAR_P_H

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

#include <QtQuickControls/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractProgressBarPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractProgressBar : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY indeterminateChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)

public:
    explicit QQuickAbstractProgressBar(QQuickItem *parent = Q_NULLPTR);

    qreal value() const;
    void setValue(qreal value);

    qreal visualPosition() const;

    bool isIndeterminate() const;
    void setIndeterminate(bool indeterminate);

    Qt::LayoutDirection layoutDirection() const;
    Qt::LayoutDirection effectiveLayoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection direction);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

Q_SIGNALS:
    void valueChanged();
    void visualPositionChanged();
    void indeterminateChanged();
    void layoutDirectionChanged();
    void effectiveLayoutDirectionChanged();
    void indicatorChanged();

protected:
    void mirrorChange() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractProgressBar)
    Q_DECLARE_PRIVATE(QQuickAbstractProgressBar)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTPROGRESSBAR_P_H
