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
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts

MessageDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            rowLayout.implicitWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    leftPadding: 24
    rightPadding: 24

    Material.elevation: 24

    MessageDialogImpl.buttonBox: buttonBox
    MessageDialogImpl.detailedTextButton: detailedTextButton

    background: Rectangle {
        implicitWidth: 320
        implicitHeight: 160
        radius: 2
        color: control.Material.dialogColor

        layer.enabled: control.Material.elevation > 0
        layer.effect: ElevationEffect {
            elevation: control.Material.elevation
        }
    }

    header: Label {
        text: control.title
        visible: control.title.length > 0
        elide: Label.ElideRight
        font.bold: true
        font.pixelSize: 16

        leftPadding: 24
        rightPadding: 24
        topPadding: 24
        bottomPadding: 24
    }

    contentItem: ColumnLayout {
        Label {
            id: textLabel
            objectName: "textLabel"
            text: control.text

            Layout.margins: 16
        }
        Label {
            id: informativeTextLabel
            objectName: "informativeTextLabel"
            text: control.informativeText

            Layout.margins: 16
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

                Layout.leftMargin: 20
            }

            DialogButtonBox {
                id: buttonBox
                objectName: "buttonBox"
                spacing: 12
                horizontalPadding: 0
                verticalPadding: 20

                Layout.fillWidth: true
                Layout.leftMargin: detailedTextButton.visible ? 12 : 20
                Layout.rightMargin: 20
            }
        }

        TextArea {
            id: detailedTextArea
            objectName: "detailedText"
            text: control.detailedText
            visible: control.showDetailedText
            wrapMode: TextEdit.WordWrap
            readOnly: true
            padding: 12

            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20

            background: Rectangle {
                color: Qt.rgba(1,1,1,1)
                radius: 3
                border.color: Qt.darker(control.palette.light)
                border.width: 1
            }
        }
    }

    Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.5)
    }

    Overlay.modeless: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.12)
    }
}
