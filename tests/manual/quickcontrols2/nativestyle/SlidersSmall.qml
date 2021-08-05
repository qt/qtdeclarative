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
    title: "Sliders small"
    property int sliderWidth: 200
    property int sliderHeight: 100

    Row {
        spacing: 40

        Column {
            spacing: 15

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
                property bool qqc2_style_small
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 10
                value: 5
                enabled: false
                property bool qqc2_style_small
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 1
            }

            Slider {
                width: sliderWidth
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 2
            }
        }

        Row {
            spacing: 20

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                property bool qqc2_style_small
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                enabled: false
                property bool qqc2_style_small
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                value: 20
                stepSize: 20

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 1
            }

            Slider {
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 100
                stepSize: 5
                value: 65

                property bool qqc2_style_small
                property int qqc2_style_tickPosition: 2
            }
        }
    }
}
