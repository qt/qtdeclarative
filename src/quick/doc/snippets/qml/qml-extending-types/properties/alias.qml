// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// Button.qml
import QtQuick

Item {
    property alias buttonText: textItem.text

    width: 200; height: 50

    Text { id: textItem }
}
//![0]
