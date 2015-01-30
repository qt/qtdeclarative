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

#ifndef QQUICKABSTRACTTABVIEW_P_H
#define QQUICKABSTRACTTABVIEW_P_H

#include <QtQuickControls/private/qquickabstractcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractTabBar;
class QQuickAbstractTabViewPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractTabView : public QQuickAbstractContainer
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QQuickAbstractTabBar *tabBar READ tabBar WRITE setTabBar NOTIFY tabBarChanged FINAL)

public:
    explicit QQuickAbstractTabView(QQuickItem *parent = Q_NULLPTR);

    int currentIndex() const;

    QQuickAbstractTabBar *tabBar() const;
    void setTabBar(QQuickAbstractTabBar *bar);

public Q_SLOTS:
    void setCurrentIndex(int index);

Q_SIGNALS:
    void currentIndexChanged(int index);
    void tabBarChanged();

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractTabView)
    Q_DECLARE_PRIVATE(QQuickAbstractTabView)
};

class Q_QUICKCONTROLS_EXPORT QQuickTabAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QQuickAbstractTabView *view READ view WRITE setView NOTIFY viewChanged FINAL)

public:
    explicit QQuickTabAttached(QObject *parent = Q_NULLPTR);

    static QQuickTabAttached *qmlAttachedProperties(QObject *object);

    int index() const;
    void setIndex(int index);

    QString title() const;
    void setTitle(const QString &title);

    bool isVisible() const;
    void setVisible(bool visible);

    QQuickAbstractTabView *view() const;
    void setView(QQuickAbstractTabView *view);

Q_SIGNALS:
    void indexChanged();
    void titleChanged();
    void visibleChanged();
    void viewChanged();

private:
    int m_index;
    bool m_visible;
    QString m_title;
    QQuickAbstractTabView *m_view;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickTabAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKABSTRACTTABVIEW_P_H
