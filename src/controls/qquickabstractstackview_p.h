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

#ifndef QQUICKABSTRACTSTACKVIEW_P_H
#define QQUICKABSTRACTSTACKVIEW_P_H

#include <QtQuickControls/private/qquickabstractcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQmlV4Function;
class QQuickAbstractStackViewPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractStackView : public QQuickAbstractContainer
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged FINAL) // TODO: hide setBusy
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged FINAL) // TODO: hide setDepth
    Q_PROPERTY(QQuickItem *currentItem READ currentItem WRITE setCurrentItem NOTIFY currentItemChanged FINAL) // TODO: hide setCurrentItem
    Q_PROPERTY(QVariant initialItem READ initialItem WRITE setInitialItem FINAL)
    Q_ENUMS(Operation)

public:
    explicit QQuickAbstractStackView(QQuickItem *parent = Q_NULLPTR);
    ~QQuickAbstractStackView();

    bool busy() const;
    void setBusy(bool busy); // TODO: hide

    int depth() const;
    void setDepth(int depth); // TODO: hide

    QQuickItem *currentItem() const;
    void setCurrentItem(QQuickItem *item); // TODO: hide

    QVariant initialItem() const;
    void setInitialItem(const QVariant &item);

    enum Operation {
        Transition,
        Immediate
    };

    Q_INVOKABLE QQuickItem *qpush(QQmlV4Function *args);
    Q_INVOKABLE QQuickItem *qpop(QQmlV4Function *args);

public Q_SLOTS:
    void qclear();

Q_SIGNALS:
    void busyChanged();
    void depthChanged();
    void currentItemChanged();

protected:
    void componentComplete() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractStackView)
    Q_DECLARE_PRIVATE(QQuickAbstractStackView)
};

class QQuickStackAttachedPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickStackAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_ENUMS(Status)

public:
    explicit QQuickStackAttached(QObject *parent = Q_NULLPTR);

    static QQuickStackAttached *qmlAttachedProperties(QObject *object);

    enum Status {
        Inactive = 0,
        Deactivating = 1,
        Activating = 2,
        Active = 3
    };

    Status status() const;
    void setStatus(Status status);

Q_SIGNALS:
    void statusChanged();

private:
    Q_DISABLE_COPY(QQuickStackAttached)
    Q_DECLARE_PRIVATE(QQuickStackAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickStackAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKABSTRACTSTACKVIEW_P_H
