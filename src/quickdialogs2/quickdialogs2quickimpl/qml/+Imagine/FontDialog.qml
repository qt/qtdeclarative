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
import QtQuick.Controls.impl
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

FontDialogImpl {
    id: control

    // Can't set implicitWidth of the NinePatchImage background, so we do it here.
    implicitWidth: Math.max(600,
                            implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(400,
                             implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

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

    background: NinePatchImage {
        source: Imagine.url + "dialog-background"
        NinePatchImageSelector on source {
            states: [
                {"modal": control.modal},
                {"dim": control.dim}
            ]
        }
    }

    Overlay.modal: NinePatchImage {
        source: Imagine.url + "dialog-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": true}
            ]
        }
    }

    Overlay.modeless: NinePatchImage {
        source: Imagine.url + "dialog-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": false}
            ]
        }
    }

    header: Label {
        text: control.title
        elide: Label.ElideRight
        font.bold: true

        leftPadding: 16
        rightPadding: 16
        topPadding: 12
        visible: control.title.length > 0

        background: NinePatchImage {
            width: parent.width
            height: parent.height

            source: Imagine.url + "dialog-title"
            NinePatchImageSelector on source {
                states: [
                    {"modal": control.modal},
                    {"dim": control.dim}
                ]
            }
        }
    }

    contentItem: FontDialogContent {
        id: content
        rowSpacing: 16
    }

    footer: RowLayout {
        id: rowLayout
        spacing: 20

        Label {
            text: qsTr("Writing System")
            Layout.leftMargin: 16
            Layout.bottomMargin: 16
        }
        ComboBox{
            id: writingSystemComboBox

            Layout.fillWidth: true
            Layout.bottomMargin: 16
        }

        DialogButtonBox {
            id: buttonBox
            standardButtons: control.standardButtons
            spacing: 12
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
        }
    }
}
