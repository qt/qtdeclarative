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

import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

import "." as DialogsImpl

FileDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    leftPadding: 20
    rightPadding: 20
    // Ensure that the background's border is visible.
    leftInset: -1
    rightInset: -1
    topInset: -1
    bottomInset: -1

    standardButtons: T.Dialog.Open | T.Dialog.Cancel

    /*
        We use attached properties because we want to handle logic in C++, and:
        - We can't assume the footer only contains a DialogButtonBox (which would allow us
          to connect up to it in QQuickFileDialogImpl); it also needs to hold a ComboBox
          and therefore the root footer item will be e.g. a layout item instead.
        - We don't want to create our own "FileDialogButtonBox" (in order to be able to handle the logic
          in C++) because we'd need to copy (and hence duplicate code in) DialogButtonBox.qml.
    */
    FileDialogImpl.buttonBox: buttonBox
    FileDialogImpl.nameFiltersComboBox: nameFiltersComboBox
    FileDialogImpl.fileDialogListView: fileDialogListView
    FileDialogImpl.breadcrumbBar: breadcrumbBar

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
        color: control.palette.window
        border.color: control.palette.dark
    }

    header: Pane {
        palette.window: control.palette.light
        padding: 20

        contentItem: Column {
            spacing: 12

            Label {
                objectName: "dialogTitleBarLabel"
                width: parent.width
                text: control.title
                visible: control.title.length > 0
                horizontalAlignment: Label.AlignHCenter
                elide: Label.ElideRight
                font.bold: true
            }

            DialogsImpl.FolderBreadcrumbBar {
                id: breadcrumbBar
                width: parent.width
                fileDialog: control

                KeyNavigation.tab: fileDialogListView
            }
        }
    }

    contentItem: ListView {
        id: fileDialogListView
        objectName: "fileDialogListView"
        clip: true
        focus: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        model: FolderListModel {
            folder: control.currentFolder
            nameFilters: control.selectedNameFilter.globs
            showDirsFirst: true
        }
        delegate: DialogsImpl.FileDialogDelegate {
            objectName: "fileDialogDelegate" + index
            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            fileDialog: control
            fileDetailRowWidth: nameFiltersComboBox.width

            KeyNavigation.backtab: breadcrumbBar
            KeyNavigation.tab: nameFiltersComboBox
        }
    }

    footer: Rectangle {
        color: control.palette.light
        implicitWidth: rowLayout.implicitWidth
        implicitHeight: rowLayout.implicitHeight

        RowLayout {
            id: rowLayout
            width: parent.width
            height: parent.height
            spacing: 20

            ComboBox {
                // OK to use IDs here, since users shouldn't be overriding this stuff.
                id: nameFiltersComboBox
                model: control.nameFilters

                Layout.leftMargin: 20
                Layout.fillWidth: true
            }

            DialogButtonBox {
                id: buttonBox
                standardButtons: control.standardButtons
                palette.window: control.palette.light
                spacing: 12
                horizontalPadding: 0
                verticalPadding: 20

                Layout.rightMargin: 20
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
