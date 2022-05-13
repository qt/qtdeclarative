// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Image {
    id: rootItem

    property bool created: false
    property string image

    property double scaledBottom: y + (height + height*scale) / 2
    property bool onLand: scaledBottom > (window.height / 2 + window.centerOffset)

    source: image
    opacity: onLand ? 1 : 0.25
    scale: Math.max((y + height - 250) * 0.01, 0.3)

    onCreatedChanged: {
        if (created && !onLand)
            rootItem.destroy();
        else
            z = scaledBottom;
    }

    onYChanged: z = scaledBottom;
}
