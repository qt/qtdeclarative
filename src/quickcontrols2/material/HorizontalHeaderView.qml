/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

T.HorizontalHeaderView {
    id: control

    implicitWidth: syncView ? syncView.width : 0
    implicitHeight: contentHeight

    delegate: Rectangle {
        // Qt6: add cellPadding (and font etc) as public API in headerview
        readonly property real cellPadding: 8

        implicitWidth: text.implicitWidth + (cellPadding * 2)
        implicitHeight: Math.max(control.height, text.implicitHeight + (cellPadding * 2))
        color: control.Material.backgroundColor

        Text {
            id: text
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole]
                                        : model[control.textRole])
                                   : modelData
            width: parent.width
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: enabled ? control.Material.foreground : control.Material.hintTextColor
        }
    }
}
