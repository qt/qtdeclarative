/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "ScrollBars"

    Row {
        spacing: container.rowSpacing

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            background: Rectangle {
                color: "lightgray"
                border.color: "gray"
                border.width: 1
            }
        }

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            contentItem: Rectangle {
                color: "lightgreen"
                border.color: "green"
                border.width: 1
            }
        }

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            background: Rectangle {
                color: "lightgray"
                border.color: "gray"
                border.width: 1
            }
            contentItem: Rectangle {
                implicitWidth: 15
                color: "lightgreen"
                border.color: "green"
                border.width: 1
            }
        }

        Column {
            spacing: container.rowSpacing

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                background: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
            }

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                contentItem: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
            }

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                background: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
                contentItem: Rectangle {
                    implicitHeight: 15
                    color: "lightgreen"
                    border.color: "green"
                    border.width: 1
                }
            }
        }
    }

}
