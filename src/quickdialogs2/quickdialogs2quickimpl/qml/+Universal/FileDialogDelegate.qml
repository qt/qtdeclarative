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
import QtQuick.Controls.impl
import QtQuick.Controls.Universal
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FileDialogDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: 12

    padding: 12
    topPadding: padding - 1
    bottomPadding: padding + 1

    icon.width: 20
    icon.height: 20
    icon.color: Color.transparent(Universal.foreground, enabled ? 1.0 : 0.2)
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/"
        + (fileIsDir ? "folder" : "file") + "-icon-square.png"

    file: fileUrl

    required property int index
    required property string fileName
    required property url fileUrl
    required property int fileSize
    required property date fileModified
    required property bool fileIsDir

    required property int fileDetailRowWidth

    contentItem: FileDialogDelegateLabel {
        delegate: control
        fileDetailRowTextColor: control.icon.color
        fileDetailRowWidth: control.fileDetailRowWidth
    }

    background: Rectangle {
        visible: control.down || control.highlighted || control.visualFocus || control.hovered
        color: control.down ? control.Universal.listMediumColor :
               control.hovered ? control.Universal.listLowColor : control.Universal.altMediumLowColor

        Rectangle {
            width: parent.width
            height: parent.height
            visible: control.visualFocus || control.highlighted
            color: control.Universal.accent
            opacity: control.Universal.theme === Universal.Light ? 0.4 : 0.6
        }
    }
}
