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

ControlContainer {
    id: container
    title: "Sliders"
    property int sliderWidth: 300
    property int sliderHeight: 140

    Row {
        spacing: 40

        Column {
            spacing: 15

            Slider {
                id: customHandle
                width: sliderWidth
                height: 20
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    id: handle
                    width: 12
                    height: customHandle.height
                    color: "white"
                    border.width: 2

                    x: customHandle.visualPosition * (customHandle.availableWidth - width)
                    y: (customHandle.availableHeight - height) / 2
                }
            }

            Slider {
                id: customBackground
                width: sliderWidth
                from: 0
                to: 10
                background: Rectangle {
                    implicitHeight: 5
                    color: "lightgray"
                    border.width: 1
                }
            }

            Slider {
                id: customAll
                width: sliderWidth
                height: 20
                from: 0
                to: 10
                background: Rectangle {
                    implicitHeight: customAll.height
                    color: "lightgray"
                    border.width: 1
                }
                handle: Rectangle {
                    width: 12
                    height: customAll.height
                    color: "white"
                    border.width: 2

                    x: customAll.visualPosition * (customAll.availableWidth - width)
                    y: (customAll.availableHeight - height) / 2
                }
            }
        }

        Row {
            spacing: 20

            Slider {
                id: customVHandle
                width: 20
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    height: 12
                    width: customVHandle.width
                    color: "white"
                    border.width: 2

                    x: (customVHandle.availableWidth - width) / 2
                    y: customVHandle.visualPosition * (customVHandle.availableHeight - height)
                }
            }

            Slider {
                id: customVBackground
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                background: Rectangle {
                    implicitWidth: 5
                    color: "lightgray"
                    border.width: 1
                }
            }

            Slider {
                id: customVAll
                width: 20
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    height: 12
                    width: customVHandle.width
                    color: "white"
                    border.width: 2

                    x: (customVAll.availableWidth - width) / 2
                    y: customVAll.visualPosition * (customVAll.availableHeight - height)
                }
                background: Rectangle {
                    implicitWidth: 5
                    color: "lightgray"
                    border.width: 1
                }
            }
        }
    }
}
