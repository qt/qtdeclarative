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
    caption: qsTr("Padding")

    SectionLayout {
        Label {
            text: qsTr("Top")
            tooltip: qsTr("Padding between the content and the top edge of the control.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.topPadding
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Left")
            tooltip: qsTr("Padding between the content and the left edge of the control.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.leftPadding
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Right")
            tooltip: qsTr("Padding between the content and the right edge of the control.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.rightPadding
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Bottom")
            tooltip: qsTr("Padding between the content and the bottom edge of the control.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.bottomPadding
                Layout.fillWidth: true
            }
        }
    }
}
