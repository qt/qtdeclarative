// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    width: 700; height: 400

//![0]
    Text {
            textFormat: Text.RichText
            text: "See the <a href=\"http://qt-project.org\">Qt Project website</a>."
            onLinkActivated: (link)=> console.log(link + " link activated")
    }
//![0]

}

