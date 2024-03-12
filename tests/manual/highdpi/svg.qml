// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    Row {
        Column {
            // should render a sharp image on retina displays: (sourceSize is in device-independent pixels)
            Image {
                sourceSize.width: 100
                sourceSize.height: 100
                source : "heart.svg"
            }

            // should render a sharp image on retina displays: if sourceSize is omitted
            // the svg viewBox size will be used; in this case 550x500
            Image {
                width : 100
                height : 100
                source : "heart.svg"
            }
        }

        // should render a sharp image on retina displays: (sourceSize is in device-independent pixels)
        Image {
            sourceSize.width: 700
            sourceSize.height: 700
            source : "heart.svg"
        }

        // should NOT render a sharp image on retina displays: if sourceSize is omitted
        // the svg viewBox size will be used; in this case 550x500
        Image {
            width : 700
            height : 700
            source : "heart.svg"
        }
    }
}
