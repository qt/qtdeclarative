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

import QtQuick.Controls
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl
import QtQuick.Controls.impl as ControlsImpl
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FileDialogDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: 12

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

    file: fileUrl

    icon.width: 16
    icon.height: 16
    icon.color: highlighted ? palette.highlightedText : palette.text
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/"
        + (fileIsDir ? "folder" : "file") + "-icon-round.png"

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

    background: NinePatchImage {
        source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/imagine/filedialogdelegate-background"
        NinePatchImageSelector on source {
            states: [
                { "disabled": !control.enabled },
                { "pressed": control.down },
                { "focused": control.visualFocus },
                { "highlighted": control.highlighted },
                { "mirrored": control.mirrored },
                { "hovered": control.enabled && control.hovered }
            ]
        }
    }
}
