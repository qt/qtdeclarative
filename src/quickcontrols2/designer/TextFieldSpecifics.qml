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

    Section {
        width: parent.width
        caption: qsTr("TextField")

        SectionLayout {
            Label {
                text: qsTr("Placeholder")
                tooltip: qsTr("Placeholder text displayed when the editor is empty.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.placeholderText
                    Layout.fillWidth: true
                }

            }

            Label {
                text: qsTr("Hover")
                tooltip: qsTr("Whether text field accepts hover events.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.hoverEnabled.valueToString
                    backendValue: backendValues.hoverEnabled
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        width: parent.width
        caption: qsTr("Placeholder Text Color")

        ColorEditor {
            caption: qsTr("Placeholder Text Color")
            backendValue: backendValues.placeholderTextColor
            supportGradient: false
        }
    }

    StandardTextSection {
        width: parent.width
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
