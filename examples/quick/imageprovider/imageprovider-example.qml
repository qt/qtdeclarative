// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import "ImageProviderCore" // import the plugin that registers the color image provider

//![0]
Column {
    Image { source: "image://colors/yellow" }
    Image { source: "image://colors/red" }
}
//![0]

