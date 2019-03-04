/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: https://www.gnu.org/licenses/fdl-1.3.html.
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.14
import QtQuick.Controls 2.14

//! [file]
ApplicationWindow {
    width: 640
    height: 480
    visible: true

    // Used as an example of a backend - this would usually be
    // e.g. a C++ type exposed to QML.
    QtObject {
        id: backend
        property int modifier
    }

    ComboBox {
        textRole: "text"
        valueRole: "value"
        // When an item is selected, update the backend.
        onActivated: backend.modifier = currentValue
        // Set the initial currentIndex to the value stored in the backend.
        Component.onCompleted: currentIndex = indexOfValue(backend.modifier)
        model: [
            { value: Qt.NoModifier, text: qsTr("No modifier") },
            { value: Qt.ShiftModifier, text: qsTr("Shift") },
            { value: Qt.ControlModifier, text: qsTr("Control") }
        ]
    }
}
//! [file]
