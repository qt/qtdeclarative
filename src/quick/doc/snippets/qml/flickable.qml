// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Flickable {
    width: 200; height: 200
    contentWidth: image.width; contentHeight: image.height

    Image { id: image; source: "bigImage.png" }
}
//! [document]
