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
    SectionLayout {
        Label {
            text: qsTr("Check State")
            tooltip: qsTr("The current check state.")
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.checkState
                model: [ "Unchecked", "PartiallyChecked", "Checked" ]
                scope: "Qt"
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Tri-state")
            tooltip: qsTr("Whether the checkbox has three states.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.tristate.valueToString
                backendValue: backendValues.tristate
                Layout.fillWidth: true
            }
        }
    }
}
