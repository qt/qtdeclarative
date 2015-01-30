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

#ifndef QQUICKABSTRACTCHECKABLE_P_H
#define QQUICKABSTRACTCHECKABLE_P_H

#include <QtQuickControls/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractCheckablePrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractCheckable : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged FINAL)

public:
    explicit QQuickAbstractCheckable(QQuickItem *parent = Q_NULLPTR);

    bool isChecked() const;
    void setChecked(bool checked);

    bool isExclusive() const;
    void setExclusive(bool exclusive);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

    Qt::LayoutDirection layoutDirection() const;
    Qt::LayoutDirection effectiveLayoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection direction);

public Q_SLOTS:
    void toggle();

Q_SIGNALS:
    void checkedChanged();
    void indicatorChanged();
    void layoutDirectionChanged();
    void effectiveLayoutDirectionChanged();

protected:
    QQuickAbstractCheckable(QQuickAbstractCheckablePrivate &dd, QQuickItem *parent);

    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void mirrorChange() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractCheckable)
    Q_DECLARE_PRIVATE(QQuickAbstractCheckable)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCHECKABLE_P_H
