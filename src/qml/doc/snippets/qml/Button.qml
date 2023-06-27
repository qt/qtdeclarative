// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]

//! [property alias]
property alias buttonLabel: label.text
Text {
    id: label
    text: "empty label"
}
    //! [property alias]

//! [id alias]
    property alias buttonImage: image

    Image {id: image}
//! [id alias]
//! [parent end]
}
//! [parent end]

//! [document]


