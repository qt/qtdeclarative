/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0

Item {
    id: top

    BorderImage {
        id: noSource
        source: ""
    }

    property string srcImage: "colors.png"

    BorderImage {
        id: clearSource
        source: srcImage
    }

    BorderImage {
        id: resized
        source: "colors.png"
        width: 300
        height: 300
    }

    BorderImage {
        id: smooth
        source: "colors.png"
        smooth: true
        width: 300
        height: 300
    }

    BorderImage {
        id: tileModes1
        source: "colors.png"
        width: 100
        height: 300
        horizontalTileMode: BorderImage.Repeat
        verticalTileMode: BorderImage.Repeat
    }

    BorderImage {
        id: tileModes2
        source: "colors.png"
        width: 300
        height: 150
        horizontalTileMode: BorderImage.Round
        verticalTileMode: BorderImage.Round
    }

    TestCase {
        name: "BorderImage"

        function test_noSource() {
            compare(noSource.source, "")
            compare(noSource.width, 0)
            compare(noSource.height, 0)
            compare(noSource.horizontalTileMode, BorderImage.Stretch)
            compare(noSource.verticalTileMode, BorderImage.Stretch)
        }

        function test_imageSource_data() {
            return [
                {
                    tag: "local",
                    source: "colors.png",
                    remote: false,
                    error: ""
                },
                {
                    tag: "local not found",
                    source: "no-such-file.png",
                    remote: false,
                    error: "SUBinline:1:21: QML BorderImage: Cannot open: SUBno-such-file.png"
                }
                // TODO: remote tests that need to use http
            ]
        }

        function test_imageSource(row) {
            var expectError = (row.error.length != 0)
            if (expectError) {
                var parentUrl = Qt.resolvedUrl(".")
                ignoreWarning(row.error.replace(/SUB/g, parentUrl))
            }

            var img = Qt.createQmlObject
                ('import QtQuick 1.0; BorderImage { source: "' +
                    row.source + '" }', top)

            if (row.remote)
                tryCompare(img, "status", BorderImage.Loading)

            if (!expectError) {
                tryCompare(img, "status", BorderImage.Ready)
                compare(img.width, 120)
                compare(img.height, 120)
                compare(img.horizontalTileMode, BorderImage.Stretch)
                compare(img.verticalTileMode, BorderImage.Stretch)
            } else {
                tryCompare(img, "status", BorderImage.Error)
            }

            img.destroy()
        }

        function test_clearSource() {
            compare(clearSource.source, Qt.resolvedUrl("colors.png"))
            compare(clearSource.width, 120)
            compare(clearSource.height, 120)

            srcImage = ""
            compare(clearSource.source, "")
            compare(clearSource.width, 0)
            compare(clearSource.height, 0)
        }

        function test_resized() {
            compare(resized.width, 300)
            compare(resized.height, 300)
            compare(resized.horizontalTileMode, BorderImage.Stretch)
            compare(resized.verticalTileMode, BorderImage.Stretch)
        }

        function test_smooth() {
            compare(smooth.smooth, true)
            compare(smooth.width, 300)
            compare(smooth.height, 300)
            compare(smooth.horizontalTileMode, BorderImage.Stretch)
            compare(smooth.verticalTileMode, BorderImage.Stretch)
        }

        function test_tileModes() {
            compare(tileModes1.width, 100)
            compare(tileModes1.height, 300)
            compare(tileModes1.horizontalTileMode, BorderImage.Repeat)
            compare(tileModes1.verticalTileMode, BorderImage.Repeat)

            compare(tileModes2.width, 300)
            compare(tileModes2.height, 150)
            compare(tileModes2.horizontalTileMode, BorderImage.Round)
            compare(tileModes2.verticalTileMode, BorderImage.Round)
        }

        function test_sciSource_data() {
            return [
                {
                    tag: "local",
                    source: "colors-round.sci",
                    remote: false,
                    valid: true
                },
                {
                    tag: "local not found",
                    source: "no-such-file.sci",
                    remote: false,
                    valid: false
                }
                // TODO: remote tests that need to use http
            ]
        }

        function test_sciSource(row) {
            var img = Qt.createQmlObject
                ('import QtQuick 1.0; BorderImage { source: "' +
                    row.source + '"; width: 300; height: 300 }', top)

            if (row.remote)
                tryCompare(img, "status", BorderImage.Loading)

            compare(img.source, Qt.resolvedUrl(row.source))
            compare(img.width, 300)
            compare(img.height, 300)

            if (row.valid) {
                tryCompare(img, "status", BorderImage.Ready)
                compare(img.border.left, 10)
                compare(img.border.top, 20)
                compare(img.border.right, 30)
                compare(img.border.bottom, 40)
                compare(img.horizontalTileMode, BorderImage.Round)
                compare(img.verticalTileMode, BorderImage.Repeat)
            } else {
                tryCompare(img, "status", BorderImage.Error)
            }

            img.destroy()
        }


        function test_invalidSciFile() {
            ignoreWarning("QSGGridScaledImage: Invalid tile rule specified. Using Stretch.") // for "Roun"
            ignoreWarning("QSGGridScaledImage: Invalid tile rule specified. Using Stretch.") // for "Repea"

            var component = Qt.createComponent("InvalidSciFile.qml")
            var invalidSciFile = component.createObject(top)

            compare(invalidSciFile.status, Image.Error)
            compare(invalidSciFile.width, 300)
            compare(invalidSciFile.height, 300)
            compare(invalidSciFile.horizontalTileMode, BorderImage.Stretch)
            compare(invalidSciFile.verticalTileMode, BorderImage.Stretch)
        }
    }
}
