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
    upButtonSpacing: 6

    contentItem: ListView {
        id: listView
        currentIndex: control.currentIndex
        model: control.contentModel
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
        interactive: false
        clip: true

        Rectangle {
            anchors.fill: parent
            color: control.palette.light
            border.color: control.palette.mid
            radius: 2
            z: -1
        }
    }
    buttonDelegate: Button {
        id: buttonDelegateRoot
        text: folderName
        flat: true

        // The default of 100 is a bit too wide for short directory names.
        Binding {
            target: buttonDelegateRoot.background
            property: "implicitWidth"
            value: 24
        }

        required property int index
        required property string folderName
    }
    separatorDelegate: IconImage {
        id: iconImage
        source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/crumb-separator-icon-round.png"
        sourceSize: Qt.size(8, 8)
        width: 8 + 6
        height: control.contentItem.height
        color: control.palette.dark
        y: (control.height - height) / 2
    }
    upButton: Button {
        x: control.leftPadding
        y: control.topPadding
        icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/up-icon-round.png"
        icon.width: 16
        icon.height: 16
        width: height
        height: Math.max(implicitHeight, control.contentItem.height)
        focusPolicy: Qt.TabFocus
    }
    textField: TextField {
        text: control.fileDialog.selectedFile
    }
}
