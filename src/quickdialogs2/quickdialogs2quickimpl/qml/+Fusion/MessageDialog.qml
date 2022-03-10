/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Fusion
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts

MessageDialogImpl {
    id: control

    implicitWidth: Math.max(control.implicitBackgroundWidth + control.leftInset + control.rightInset,
                            control.contentWidth + control.leftPadding + control.rightPadding,
                            control.implicitHeaderWidth,
                            rowLayout.implicitWidth)
    implicitHeight: Math.max(control.implicitBackgroundHeight + control.topInset + control.bottomInset,
                             control.contentHeight + control.topPadding + control.bottomPadding
                             + (control.implicitHeaderHeight > 0 ? control.implicitHeaderHeight + control.spacing : 0)
                             + (control.implicitFooterHeight > 0 ? control.implicitFooterHeight + control.spacing : 0))

    padding: 6
    horizontalPadding: 12

    MessageDialogImpl.buttonBox: buttonBox
    MessageDialogImpl.detailedTextButton: detailedTextButton

    background: Rectangle {
        implicitWidth: 320
        implicitHeight: 120
        color: control.palette.window
        border.color: control.palette.mid
        radius: 2

        Rectangle {
            z: -1
            x: 1
            y: 1
            width: parent.width
            height: parent.height
            color: control.palette.shadow
            opacity: 0.2
            radius: 2
        }
    }

    header: Label {
        text: control.title
        horizontalAlignment: Label.AlignHCenter
        elide: Label.ElideRight
        font.bold: true
        padding: 6
    }

    contentItem: ColumnLayout {
        Label {
            id: textLabel
            objectName: "textLabel"
            text: control.text
            visible: text.length > 0

            Layout.margins: 12
        }
        Label {
            id: informativeTextLabel
            objectName: "informativeTextLabel"
            text: control.informativeText
            visible: text.length > 0

            Layout.margins: 12
        }
    }

    footer: ColumnLayout {
        id: columnLayout

        RowLayout {
            id: rowLayout

            Button {
                id: detailedTextButton
                objectName: "detailedTextButton"
                text: control.showDetailedText ? qsTr("Hide Details...") : qsTr("Show Details...")

                Layout.leftMargin: 12
            }

            DialogButtonBox {
                id: buttonBox
                objectName: "buttonBox"
                spacing: 6
                horizontalPadding: 0
                verticalPadding: 12

                Layout.fillWidth: true
                Layout.leftMargin: detailedTextButton.visible ? 6 : 12
                Layout.rightMargin: 12
            }
        }

        TextArea {
            id: detailedTextArea
            objectName: "detailedText"
            text: control.detailedText
            visible: control.showDetailedText
            wrapMode: TextEdit.WordWrap
            readOnly: true

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 12

            background: Rectangle {
                color: Qt.rgba(1,1,1,1)
                radius: 3
                border.color: Qt.darker(control.palette.light)
                border.width: 1
            }
        }
    }

    Overlay.modal: Rectangle {
        color: Fusion.topShadow
    }

    Overlay.modeless: Rectangle {
        color: Fusion.topShadow
    }
}
