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
    caption: qsTr("Inset")

    SectionLayout {
        Label {
            text: qsTr("Vertical")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Top")
                tooltip: qsTr("Top inset for the background.")
                width: 42
            }
            SpinBox {
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.topInset
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Bottom")
                tooltip: qsTr("Bottom inset for the background.")
                width: 42
            }
            SpinBox {
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.bottomInset
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Horizontal")
        }
        SecondColumnLayout {
            Label {
                text: qsTr("Left")
                tooltip: qsTr("Left inset for the background.")
                width: 42
            }
            SpinBox {
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.leftInset
                Layout.fillWidth: true
            }
            Item {
                width: 4
                height: 4
            }

            Label {
                text: qsTr("Right")
                tooltip: qsTr("Right inset for the background.")
                width: 42
            }
            SpinBox {
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.rightInset
                Layout.fillWidth: true
            }
        }
    }
}
