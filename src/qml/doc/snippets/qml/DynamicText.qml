// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Text {
    id: textElement
    width: 200
    height: 200
    text: "Default text"
    property string dynamicText: "Dynamic text"
    onTextChanged: console.log(text)
}
//![0]
