/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

    background: Rectangle {}
    contentItem: ListView {
        id: listView
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

        required property int index
        required property string folderName
    }
    separatorDelegate: IconImage {
        id: iconImage
        source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/crumb-separator-icon-round.png"
        sourceSize: Qt.size(8, 8)
        width: 8
        height: control.contentItem.height
        y: (control.height - height) / 2
    }
    upButton: ToolButton {
        x: control.leftPadding
        y: control.topPadding
        icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/up-icon-thick-square.png"
        icon.width: 16
        icon.height: 16
        focusPolicy: Qt.TabFocus
    }
    textField: TextField {
        text: control.fileDialog.selectedFile
    }
}
