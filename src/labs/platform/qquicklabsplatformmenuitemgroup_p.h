/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#ifndef QQUICKLABSPLATFORMMENUITEMGROUP_P_H
#define QQUICKLABSPLATFORMMENUITEMGROUP_P_H

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
#include <QtCore/qlist.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMenuItem;
class QQuickLabsPlatformMenuItemGroupPrivate;

class QQuickLabsPlatformMenuItemGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuItem *checkedItem READ checkedItem WRITE setCheckedItem NOTIFY checkedItemChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickLabsPlatformMenuItem> items READ items NOTIFY itemsChanged FINAL)

public:
    explicit QQuickLabsPlatformMenuItemGroup(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenuItemGroup();

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isExclusive() const;
    void setExclusive(bool exclusive);

    QQuickLabsPlatformMenuItem *checkedItem() const;
    void setCheckedItem(QQuickLabsPlatformMenuItem *item);

    QQmlListProperty<QQuickLabsPlatformMenuItem> items();

    Q_INVOKABLE void addItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void removeItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void clear();

Q_SIGNALS:
    void triggered(QQuickLabsPlatformMenuItem *item);
    void hovered(QQuickLabsPlatformMenuItem *item);

    void enabledChanged();
    void visibleChanged();
    void exclusiveChanged();
    void checkedItemChanged();
    void itemsChanged();

private:
    QQuickLabsPlatformMenuItem *findCurrent() const;
    void updateCurrent();
    void activateItem();
    void hoverItem();

    static void items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, QQuickLabsPlatformMenuItem *obj);
    static qsizetype items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop);
    static QQuickLabsPlatformMenuItem *items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, qsizetype index);
    static void items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop);

    bool m_enabled;
    bool m_visible;
    bool m_exclusive;
    QQuickLabsPlatformMenuItem *m_checkedItem;
    QList<QQuickLabsPlatformMenuItem*> m_items;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformMenuItemGroup)

#endif // QQUICKLABSPLATFORMMENUITEMGROUP_P_H
