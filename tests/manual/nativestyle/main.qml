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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: qsTr("Desktop Gallery")

    TabBar {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        anchors.rightMargin: 40

        TabButton {
            text: qsTr("Default controls")
        }

        TabButton {
            text: qsTr("Customized controls")
        }
    }

    StackLayout {
        currentIndex: bar.currentIndex
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20

        ScrollView {
            contentWidth: availableWidth

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 20

                Buttons { }
                CheckBoxes { }
                RadioButtons { }
                SpinBoxes { }
                TextFields { }
                TextAreas { }
                ComboBoxes { }
                Dials { }
                Frames { }
                ProgressBars { }
                ScrollBars { }
                Sliders { }
                SlidersSmall { }
                SlidersMini { }
            }
        }

        ScrollView {
            contentWidth: availableWidth

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 20

                CustomButtons { }
                CustomCheckBoxes { }
                CustomRadioButtons { }
                CustomSpinBoxes { }
                CustomTextFields { }
                CustomTextAreas { }
                CustomComboBoxes { }
                CustomDials { }
                CustomFrames { }
                CustomProgressBars { }
                CustomScrollBars { }
                CustomSliders { }
            }
        }
    }

}
