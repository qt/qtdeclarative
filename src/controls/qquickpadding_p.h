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

#ifndef QQUICKPADDING_P_H
#define QQUICKPADDING_P_H

#include <QtCore/qobject.h>
#include <QtQuickControls/private/qtquickcontrolsglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS_EXPORT QQuickPadding : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal left MEMBER left NOTIFY leftChanged)
    Q_PROPERTY(qreal top MEMBER top NOTIFY topChanged)
    Q_PROPERTY(qreal right MEMBER right NOTIFY rightChanged)
    Q_PROPERTY(qreal bottom MEMBER bottom NOTIFY bottomChanged)

public:
    QQuickPadding(QObject *parent = Q_NULLPTR) : QObject(parent),
        top(0), left(0), right(0), bottom(0)
    {
    }

    qreal top;
    qreal left;
    qreal right;
    qreal bottom;

Q_SIGNALS:
    void leftChanged();
    void topChanged();
    void rightChanged();
    void bottomChanged();
};

QT_END_NAMESPACE

#endif // QQUICKPADDING_P_H
