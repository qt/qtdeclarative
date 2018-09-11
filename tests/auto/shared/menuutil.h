/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QQUICKMENUTUTIL_H
#define QQUICKMENUTUTIL_H

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicklistview_p.h>

/*
    QQuickMenuPrivate::insertItem() culls newly added items to ensure
    that they don't show up when they shouldn't, but now that QQuickMenu
    postpones item creation until after component completion (QTBUG-67559),
    the culled flag being set means that mouse events don't get delivered
    to menu items (see the culled check in QQuickWindowPrivate::pointerTargets()).

    ListView unculls the items in FxViewItem::setVisible(), and waiting until
    polishes are finished is a reliable way of ensuring that that happens
    before we send mouse events.
*/
#define waitForMenuListViewPolish(menu) \
{ \
    const auto listView = qobject_cast<QQuickListView*>((menu)->contentItem()); \
    Q_ASSERT(listView); \
    QTRY_COMPARE(QQuickItemPrivate::get(listView)->polishScheduled, false); \
}

#endif // QQUICKMENUTUTIL_H
