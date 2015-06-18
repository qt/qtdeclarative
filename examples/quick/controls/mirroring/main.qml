/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
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
                color: Theme.accentColor
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
                color: Theme.frameColor
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
                    width: window.width > window.height || contentWidth > preferredWidth ? flow.width : preferredWidth
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
                    width: window.width > window.height || contentWidth > preferredWidth ? flow.width : preferredWidth
                    ExclusiveGroup { id: eg }
                    ColumnLayout {
                        width: parent.width
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
                        spacing: Theme.spacing
                        readonly property real availableWidth: (flow.width - 12) / 2
                        readonly property real contentWidth: okButton.implicitWidth + cancelButton.implicitWidth + 12
                        readonly property real buttonWidth: implicitWidth > availableWidth ? (width / 2 - spacing) : (width / 2 - 2 * spacing) / 2
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
                        spacing: Theme.spacing
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
                        spacing: Theme.spacing
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
