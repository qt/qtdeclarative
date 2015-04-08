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

import QtQuick 2.6
import QtQuick.Controls 2.0

AbstractPageIndicator {
    id: control

    property alias spacing: row.spacing

    property Component delegate: Rectangle {
        implicitWidth: 8
        implicitHeight: 8

        radius: width / 2
        color: control.Theme.shadowColor // TODO

        opacity: index === currentIndex ? 0.75 : 0.25
        Behavior on opacity { OpacityAnimator { duration: 100 } }
    }

    Accessible.role: Accessible.Indicator

    contentWidth: row.implicitWidth
    contentHeight: row.implicitHeight

    implicitWidth: Math.max(background ? background.implicitWidth : 0, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0, contentHeight + topPadding + bottomPadding)

    padding: Theme.padding

    contentItem: Row {
        id: row

        x: control.leftPadding
        y: control.topPadding
        width: parent.width - control.leftPadding - control.rightPadding
        height: parent.height - control.topPadding - control.bottomPadding

        spacing: control.Theme.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}
