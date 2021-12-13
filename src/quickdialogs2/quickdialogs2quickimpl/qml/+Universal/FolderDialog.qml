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
import QtQuick.Controls.Universal
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

    padding: 24
    verticalPadding: 18

    standardButtons: T.Dialog.Open | T.Dialog.Cancel

    FolderDialogImpl.folderDialogListView: folderDialogListView
    FolderDialogImpl.breadcrumbBar: breadcrumbBar

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
        color: control.Universal.chromeMediumLowColor
        border.color: control.Universal.chromeHighColor
        border.width: 1 // FlyoutBorderThemeThickness
    }

    header: ColumnLayout {
        spacing: 12

        Label {
            text: control.title
            elide: Label.ElideRight
            // TODO: QPlatformTheme::TitleBarFont
            font.pixelSize: 20

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 18
            Layout.fillWidth: true
            Layout.preferredHeight: control.title.length > 0 ? implicitHeight : 0

            background: Rectangle {
                // FlyoutBorderThemeThickness
                x: 1
                y: 1
                color: control.Universal.chromeMediumLowColor
                width: parent.width - 2
                height: parent.height - 1
            }
        }

        DialogsImpl.FolderBreadcrumbBar {
            id: breadcrumbBar
            dialog: control

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.preferredWidth: 400
            Layout.fillWidth: true
        }
    }

    contentItem: ListView {
        id: folderDialogListView
        objectName: "folderDialogListView"
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        model: FolderListModel {
            folder: control.currentFolder
            showFiles: false
            sortCaseSensitive: false
        }
        delegate: DialogsImpl.FolderDialogDelegate {
            objectName: "folderDialogDelegate" + index
            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            dialog: control
        }
    }

    footer: DialogButtonBox {
        id: buttonBox
        standardButtons: control.standardButtons
        spacing: 12
        leftPadding: 24
        rightPadding: 24
        topPadding: 6
        bottomPadding: 24
    }

    T.Overlay.modal: Rectangle {
        color: control.Universal.baseLowColor
    }

    T.Overlay.modeless: Rectangle {
        color: control.Universal.baseLowColor
    }
}
