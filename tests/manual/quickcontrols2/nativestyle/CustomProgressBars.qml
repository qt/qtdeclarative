/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic.impl

ControlContainer {
    id: container
    title: "ProgressBars"

    property int time: 0
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            time++
            if (time > 10)
                time = 0
        }
    }

    Row {
        spacing: container.rowSpacing

        ProgressBar {
            id: c3
            width: 100
            from: 0
            to: 10
            value: time
            indeterminate: false
            padding: 5
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 6
                color: "darkgray"
            }
            contentItem: ProgressBarImpl {
                implicitHeight: 6
                implicitWidth: 100
                progress: c3.position
                indeterminate: false
                color: "lightgreen"
            }
        }
    }
}
