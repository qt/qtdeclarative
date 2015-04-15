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

#ifndef QQUICKEXCLUSIVEGROUP_P_H
#define QQUICKEXCLUSIVEGROUP_P_H

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

#include <QtCore/qobject.h>
#include <QtQuickControls/private/qtquickcontrolsglobal_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickExclusiveGroupPrivate;
class QQuickExclusiveAttachedPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickExclusiveGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *current READ current WRITE setCurrent NOTIFY currentChanged)

public:
    explicit QQuickExclusiveGroup(QObject *parent = Q_NULLPTR);

    QObject *current() const;
    void setCurrent(QObject *current);

public Q_SLOTS:
    void addCheckable(QObject *object);
    void removeCheckable(QObject *object);

Q_SIGNALS:
    void currentChanged();

private:
    Q_DISABLE_COPY(QQuickExclusiveGroup)
    Q_DECLARE_PRIVATE(QQuickExclusiveGroup)

    Q_PRIVATE_SLOT(d_func(), void _q_updateCurrent())
};

class Q_QUICKCONTROLS_EXPORT QQuickExclusiveAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickExclusiveGroup *group READ group WRITE setGroup NOTIFY groupChanged FINAL)

public:
    explicit QQuickExclusiveAttached(QObject *parent = Q_NULLPTR);

    static QQuickExclusiveAttached *qmlAttachedProperties(QObject *object);

    QQuickExclusiveGroup *group() const;
    void setGroup(QQuickExclusiveGroup *group);

Q_SIGNALS:
    void groupChanged();

private:
    Q_DISABLE_COPY(QQuickExclusiveAttached)
    Q_DECLARE_PRIVATE(QQuickExclusiveAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickExclusiveAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKEXCLUSIVEGROUP_H
