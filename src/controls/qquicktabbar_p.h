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

#ifndef QQUICKTABBAR_P_H
#define QQUICKTABBAR_P_H

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

#include <QtQuickControls/private/qquickcontainer_p.h>
#include <QtQml/qqmllist.h>

QT_BEGIN_NAMESPACE

class QQuickTabBarPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickTabBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(QVariant contentModel READ contentModel CONSTANT FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> contentChildren READ contentChildren FINAL)
    Q_CLASSINFO("DefaultProperty", "contentData")

public:
    explicit QQuickTabBar(QQuickItem *parent = Q_NULLPTR);
    ~QQuickTabBar();

    int count() const;
    int currentIndex() const;
    QQuickItem *currentItem() const;

    QVariant contentModel() const;
    QQmlListProperty<QObject> contentData();
    QQmlListProperty<QQuickItem> contentChildren();

    Q_INVOKABLE QQuickItem *itemAt(int index) const;
    Q_INVOKABLE void addItem(QQuickItem *item);
    Q_INVOKABLE void insertItem(int index, QQuickItem *item);
    Q_INVOKABLE void moveItem(int from, int to);
    Q_INVOKABLE void removeItem(int index);

public Q_SLOTS:
    void setCurrentIndex(int index);

Q_SIGNALS:
    void countChanged();
    void currentIndexChanged();
    void currentItemChanged();

protected:
    void updatePolish() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;
    void itemChange(ItemChange change, const ItemChangeData &data) Q_DECL_OVERRIDE;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) Q_DECL_OVERRIDE;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickTabBar)
    Q_DECLARE_PRIVATE(QQuickTabBar)
};

QT_END_NAMESPACE

#endif // QQUICKTABBAR_P_H
