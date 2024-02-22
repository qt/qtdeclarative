// Copyright (C) 2021 Alexander Akulich <akulichalexander@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtTest 1.0

TestCase {
    name: "FuzzyCompare"

    function test_differentColors()
    {
        const color1 = Qt.rgba(0, 0, 0, 0)
        const color2 = Qt.rgba(1, 1, 1, 1)
        expectFail("", "The colors are very different so fuzzyCompare should fail")
        fuzzyCompare(color1, color2, 1)
    }
}
