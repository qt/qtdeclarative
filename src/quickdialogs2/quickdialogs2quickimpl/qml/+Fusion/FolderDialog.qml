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

import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

import "." as DialogsImpl

FolderDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    padding: 6
    horizontalPadding: 12

    standardButtons: T.Dialog.Open | T.Dialog.Cancel

    FolderDialogImpl.folderDialogListView: folderDialogListView
    FolderDialogImpl.breadcrumbBar: breadcrumbBar

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

    header: ColumnLayout {
        spacing: 0

        Label {
            objectName: "dialogTitleBarLabel"
            text: control.title
            horizontalAlignment: Label.AlignHCenter
            elide: Label.ElideRight
            font.bold: true
            padding: 6

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: control.title.length > 0 ? 0 : 12
            Layout.preferredHeight: control.title.length > 0 ? implicitHeight : 0
        }

        DialogsImpl.FolderBreadcrumbBar {
            id: breadcrumbBar
            dialog: control

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12

            KeyNavigation.tab: folderDialogListView
        }
    }

    contentItem: Frame {
        padding: 0
        verticalPadding: 1

        ListView {
            id: folderDialogListView
            objectName: "fileDialogListView"
            anchors.fill: parent
            clip: true
            focus: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            model: FolderListModel {
                folder: control.currentFolder
                showFiles: false
                sortCaseSensitive: false
            }
            delegate: DialogsImpl.FolderDialogDelegate {
                objectName: "folderDialogDelegate" + index
                x: 1
                width: ListView.view.width - 2
                highlighted: ListView.isCurrentItem
                dialog: control

                KeyNavigation.backtab: breadcrumbBar
                KeyNavigation.tab: control.footer
            }
        }
    }

    footer: DialogButtonBox {
        id: buttonBox
        standardButtons: control.standardButtons
        spacing: 6
        leftPadding: 0
        rightPadding: 12
        topPadding: 0
        bottomPadding: 12
        background: null
    }

    T.Overlay.modal: Rectangle {
        color: Fusion.topShadow
    }

    T.Overlay.modeless: Rectangle {
        color: Fusion.topShadow
    }
}
