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
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FolderBreadcrumbBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + (upButton ? upButton.implicitWidth + upButtonSpacing : 0)
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    upButtonSpacing: 20
    padding: 1

    background: Rectangle {
        border.color: control.palette.button
    }
    contentItem: ListView {
        currentIndex: control.currentIndex
        model: control.contentModel
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
        interactive: false
        clip: true
    }
    buttonDelegate: Button {
        id: buttonDelegateRoot
        text: folderName
        flat: true

        // The default of 100 is a bit too wide for short directory names.
        Binding {
            target: buttonDelegateRoot.background
            property: "implicitWidth"
            value: 40
        }

        required property int index
        required property string folderName
    }
    separatorDelegate: IconImage {
        id: iconImage
        source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/crumb-separator-icon-round.png"
        sourceSize: Qt.size(8, 8)
        width: 8
        height: control.contentItem.height
        color: control.palette.button
        y: (control.height - height) / 2
    }
    upButton: ToolButton {
        x: control.leftPadding
        y: control.topPadding
        icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/up-icon-round.png"
        icon.width: 16
        icon.height: 16
        width: height
        focusPolicy: Qt.TabFocus
    }
    textField: TextField {
        text: control.fileDialog.selectedFile
    }
}
