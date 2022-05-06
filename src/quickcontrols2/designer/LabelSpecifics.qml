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
import HelperWidgets
import QtQuick.Layouts

Column {
    width: parent.width

    StandardTextSection {
        width: parent.width
        showIsWrapping: true
        showFormatProperty: true
        showVerticalAlignment: true
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Text Color")

        ColorEditor {
            caption: qsTr("Text Color")
            backendValue: backendValues.color
            supportGradient: false
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Style Color")

        ColorEditor {
            caption: qsTr("Style Color")
            backendValue: backendValues.styleColor
            supportGradient: false
        }
    }

    FontSection {
        width: parent.width
    }

    PaddingSection {
        width: parent.width
    }

    InsetSection {
        width: parent.width
    }
}
