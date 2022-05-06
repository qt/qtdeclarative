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
    caption: qsTr("AbstractButton")

    SectionLayout {
        Label {
            text: qsTr("Text")
            tooltip: qsTr("The text displayed on the button.")
        }
        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.text
                Layout.fillWidth: true
            }
        }

         Label {
            text: qsTr("Display")
            tooltip: qsTr("Determines how the icon and text are displayed within the button.")
            disabledState: !backendValues.display.isAvailable
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.display
                model: [ "IconOnly", "TextOnly", "TextBesideIcon" ]
                scope: "AbstractButton"
                Layout.fillWidth: true
                enabled: backendValue.isAvailable
            }
        }

        Label {
            visible: checkable
            text: qsTr("Checkable")
            tooltip: qsTr("Whether the button is checkable.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.checkable.valueToString
                backendValue: backendValues.checkable
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Checked")
            tooltip: qsTr("Whether the button is checked.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.checked.valueToString
                backendValue: backendValues.checked
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Exclusive")
            tooltip: qsTr("Whether the button is exclusive.")
            disabledState: !backendValues.autoExclusive.isAvailable
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.autoExclusive.valueToString
                backendValue: backendValues.autoExclusive
                Layout.fillWidth: true
                enabled: backendValue.isAvailable
            }
        }

        Label {
            text: qsTr("Auto-Repeat")
            tooltip: qsTr("Whether the button repeats pressed(), released() and clicked() signals while the button is pressed and held down.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.autoRepeat.valueToString
                backendValue: backendValues.autoRepeat
                Layout.fillWidth: true
            }
        }
    }
}
