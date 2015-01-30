/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick 2.4
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls - Mirroring Example"

    ListView {
        id: listview
        anchors.fill: parent

        LayoutMirroring.enabled: headerItem.mirror
        LayoutMirroring.childrenInherit: true

        headerPositioning: ListView.PullBackHeader
        header: Rectangle {
            property alias mirror: mirrorToggle.checked

            z: 2
            width: parent.width
            height: label.implicitHeight + 96

            Label {
                id: label
                text: "Beyond the essentials."
                color: window.style.accentColor
                anchors.fill: parent
                anchors.margins: 48
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                font.pointSize: 26
            }

            ToggleButton {
                id: mirrorToggle
                text: "Mirror"
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                layoutDirection: Qt.RightToLeft
                LayoutMirroring.enabled: false
            }

            Rectangle {
                width: parent.width
                height: 1
                anchors.bottom: parent.bottom
                color: window.style.frameColor
            }
        }

        model: VisualItemModel {

            Item { width: 1; height: 24 }

            Flow {
                id: flow
                spacing: 12
                width: Math.min(window.width, window.height) - 24
                anchors.horizontalCenter: parent.horizontalCenter

                GroupBox {
                    title: "CheckBox"
                    readonly property real preferredWidth: (flow.width - 12) / 2
                    width: window.width > window.height || implicitWidth > preferredWidth ? flow.width : preferredWidth
                    ColumnLayout {
                        width: parent.width
                        CheckBox {
                            width: parent.width
                            text: "E-mail"
                            checked: true
                        }
                        CheckBox {
                            width: parent.width
                            text: "Calendar"
                            checked: true
                        }
                        CheckBox {
                            width: parent.width
                            text: "Contacts"
                        }
                    }
                }

                GroupBox {
                    title: "RadioButton"
                    readonly property real preferredWidth: (flow.width - 12) / 2
                    width: window.width > window.height || implicitWidth > preferredWidth ? flow.width : preferredWidth
                    ColumnLayout {
                        width: parent.width
                        ExclusiveGroup { id: eg }
                        RadioButton {
                            width: parent.width
                            text: "Portrait"
                            Exclusive.group: eg
                        }
                        RadioButton {
                            width: parent.width
                            text: "Landscape"
                            Exclusive.group: eg
                        }
                        RadioButton {
                            width: parent.width
                            text: "Automatic"
                            checked: true
                            Exclusive.group: eg
                        }
                    }
                }

                GroupBox {
                    title: "Button"
                    width: flow.width
                    Row {
                        width: parent.width
                        spacing: window.style.spacing
                        readonly property real availableWidth: (flow.width - 12) / 2
                        readonly property real contentWidth: okButton.implicitWidth + cancelButton.implicitWidth + 12
                        readonly property real buttonWidth: contentWidth > availableWidth ? (width / 2 - spacing) : (width / 2 - 2 * spacing) / 2
                        Button {
                            id: okButton
                            text: "Ok"
                            width: parent.buttonWidth
                        }
                        Button {
                            id: cancelButton
                            text: "Cancel"
                            width: parent.buttonWidth
                        }
                    }
                }

                GroupBox {
                    title: "Switch"
                    width: flow.width
                    Column {
                        width: parent.width
                        Switch {
                            width: parent.width
                            text: "Wifi"
                            checked: true
                        }
                        Switch {
                            width: parent.width
                            text: "Bluetooth"
                        }
                    }
                }

                GroupBox {
                    title: "ProgressBar"
                    width: flow.width
                    Column {
                        width: parent.width
                        spacing: window.style.spacing
                        ProgressBar {
                            width: parent.width
                            indeterminate: true
                        }
                        ProgressBar {
                            width: parent.width
                            value: slider.position
                        }
                    }
                }

                GroupBox {
                    title: "Slider"
                    width: flow.width
                    Column {
                        width: parent.width
                        spacing: window.style.spacing
                        Slider {
                            id: slider
                            value: 0.4
                            width: parent.width
                        }
                        Slider {
                            width: parent.width
                            snapMode: AbstractSlider.SnapAlways
                            stepSize: 0.2
                            value: 0.8
                        }
                    }
                }
            }

            Item { width: 1; height: 12 }
        }

        AbstractScrollIndicator.vertical: ScrollIndicator { anchors.right: parent.right }
    }
}
