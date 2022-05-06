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

/*
    Most of the elements in here are the same between styles, so we
    have a reusable component for it and provide some properties to enable style-specific tweaks.
*/
Item {
    id: root
    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight

    required property DialogsQuickImpl.FileDialogDelegate delegate
    required property int fileDetailRowWidth

    property color fileDetailRowTextColor

    Column {
        id: column
        y: (parent.height - height) / 2

        Row {
            spacing: root.delegate.spacing

            IconImage {
                id: iconImage
                source: root.delegate.icon.source
                sourceSize: Qt.size(root.delegate.icon.width, root.delegate.icon.height)
                width: root.delegate.icon.width
                height: root.delegate.icon.height
                color: root.delegate.icon.color
                y: (parent.height - height) / 2
            }
            Label {
                text: root.delegate.fileName
                color: root.delegate.icon.color
                y: (parent.height - height) / 2
            }
        }

        Item {
            id: fileDetailRow
            x: iconImage.width + root.delegate.spacing
            width: fileDetailRowWidth - x - root.delegate.leftPadding
            implicitHeight: childrenRect.height

            Label {
                text: locale.formattedDataSize(root.delegate.fileSize)
                font.pixelSize: root.delegate.font.pixelSize * 0.75
                color: root.fileDetailRowTextColor
            }
            Label {
                text: Qt.formatDateTime(root.delegate.fileModified)
                font.pixelSize: root.delegate.font.pixelSize * 0.75
                color: root.fileDetailRowTextColor
                x: parent.width - width
            }
        }
    }
}
