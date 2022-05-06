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

Section {
    id: section
    caption: qsTr("Button")

    SectionLayout {

        Label {
            text: qsTr("Flat")
            tooltip: qsTr("Whether the button is flat.")
            disabledState: !backendValues.flat.isAvailable
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.flat.valueToString
                backendValue: backendValues.flat
                Layout.fillWidth: true
                enabled: backendValue.isAvailable
            }
        }
        Label {
            text: qsTr("Highlighted")
            tooltip: qsTr("Whether the button is highlighted.")
            disabledState: !backendValues.highlighted.isAvailable
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.highlighted.valueToString
                backendValue: backendValues.highlighted
                Layout.fillWidth: true
                enabled: backendValue.isAvailable
            }
        }
    }
}
