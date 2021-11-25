/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKMENUBAR_P_H
#define QQUICKMENUBAR_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBarPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickMenuBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickMenuBar::d_func(), QQmlListProperty<QQuickMenu> menus READ menus NOTIFY menusChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickMenuBar::d_func(), QQmlListProperty<QObject> contentData READ contentData FINAL)
    QML_NAMED_ELEMENT(MenuBar)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMenuBar(QQuickItem *parent = nullptr);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    Q_INVOKABLE QQuickMenu *menuAt(int index) const;
    Q_INVOKABLE void addMenu(QQuickMenu *menu);
    Q_INVOKABLE void insertMenu(int index, QQuickMenu *menu);
    Q_INVOKABLE void removeMenu(QQuickMenu *menu);
    Q_INVOKABLE QQuickMenu *takeMenu(int index);

Q_SIGNALS:
    void delegateChanged();
    void menusChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

    bool isContent(QQuickItem *item) const override;
    void itemAdded(int index, QQuickItem *item) override;
    void itemMoved(int index, QQuickItem *item) override;
    void itemRemoved(int index, QQuickItem *item) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuBar)
    Q_DECLARE_PRIVATE(QQuickMenuBar)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMenuBar)

#endif // QQUICKMENUBAR_P_H
