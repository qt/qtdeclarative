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
        caption: qsTr("PageIndicator")

        SectionLayout {
            Label {
                text: qsTr("Count")
                tooltip: qsTr("The number of pages.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.count
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Current")
                tooltip: qsTr("The index of the current page.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.currentIndex
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Interactive")
                tooltip: qsTr("Whether the control is interactive.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.interactive.valueToString
                    backendValue: backendValues.interactive
                    Layout.fillWidth: true
                }
            }
        }
    }

    ControlSection {
        width: parent.width
    }

    PaddingSection {
        width: parent.width
    }
}
