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
import QtQuick.Controls.impl as ControlsImpl
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FileDialogDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 12
    spacing: 8
    topPadding: 0
    bottomPadding: 0

    file: fileUrl

    icon.width: 16
    icon.height: 16
    icon.color: highlighted ? palette.highlightedText : palette.text
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/"
        + (fileIsDir ? "folder" : "file") + "-icon-round.png"

    // We don't use index here, but in C++. Since we're using required
    // properties, the index context property will not be injected, so we can't
    // use its QQmlContext to access it.
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
        implicitWidth: 100
        implicitHeight: 40
        visible: control.down || control.highlighted || control.visualFocus
        color: Color.blend(control.down ? control.palette.midlight : control.palette.light,
                                          control.palette.highlight, control.highlighted ? 0.15 : 0.0)
    }
}
