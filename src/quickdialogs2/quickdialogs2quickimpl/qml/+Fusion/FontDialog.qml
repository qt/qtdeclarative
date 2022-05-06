/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

FontDialogImpl {
    id: control

    implicitWidth: Math.max(control.implicitBackgroundWidth + control.leftInset + control.rightInset,
                            control.contentWidth + control.leftPadding + control.rightPadding,
                            control.implicitHeaderWidth,
                            control.implicitFooterWidth)
    implicitHeight: Math.max(control.implicitBackgroundHeight + control.topInset + control.bottomInset,
                             control.contentHeight + control.topPadding + control.bottomPadding
                             + (control.implicitHeaderHeight > 0 ? control.implicitHeaderHeight + control.spacing : 0)
                             + (control.implicitFooterHeight > 0 ? control.implicitFooterHeight + control.spacing : 0))

    leftPadding: 20
    rightPadding: 20
    // Ensure that the background's border is visible.
    leftInset: -1
    rightInset: -1
    topInset: -1
    bottomInset: -1

    standardButtons: T.Dialog.Ok | T.Dialog.Cancel

    FontDialogImpl.buttonBox: buttonBox
    FontDialogImpl.familyListView: content.familyListView
    FontDialogImpl.styleListView: content.styleListView
    FontDialogImpl.sizeListView: content.sizeListView
    FontDialogImpl.sampleEdit: content.sampleEdit
    FontDialogImpl.writingSystemComboBox: writingSystemComboBox
    FontDialogImpl.underlineCheckBox: content.underline
    FontDialogImpl.strikeoutCheckBox: content.strikeout
    FontDialogImpl.familyEdit: content.familyEdit
    FontDialogImpl.styleEdit: content.styleEdit
    FontDialogImpl.sizeEdit: content.sizeEdit

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
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

    Overlay.modal: Rectangle {
        color: Fusion.topShadow
    }

    Overlay.modeless: Rectangle {
        color: Fusion.topShadow
    }

    header: Label {
        text: control.title
        horizontalAlignment: Label.AlignHCenter
        elide: Label.ElideRight
        font.bold: true
        padding: 6
    }

    contentItem: FontDialogContent {
        id: content
    }

    footer: RowLayout {
        id: rowLayout
        spacing: 12

        Label {
            text: qsTr("Writing System")

            Layout.leftMargin: 12
            Layout.topMargin: 6
            Layout.bottomMargin: 6
        }
        ComboBox{
            id: writingSystemComboBox

            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
        }

        DialogButtonBox {
            id: buttonBox
            standardButtons: control.standardButtons
            spacing: 6
            horizontalPadding: 0
            verticalPadding: 0
            background: null

            Layout.rightMargin: 12
            Layout.topMargin: 6
            Layout.bottomMargin: 6
        }
    }
}
