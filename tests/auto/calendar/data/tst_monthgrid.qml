/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtTest 1.0
import Qt.labs.calendar 1.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "MonthGrid"

    Component {
        id: component
        MonthGrid { }
    }

    function test_locale() {
        var control = component.createObject(testCase, {month: 0, year: 2013})

        compare(control.contentItem.children.length, 6 * 7 + 1)

        // January 2013
        compare(control.month, 0)
        compare(control.year, 2013)

        // en_GB
        control.locale = Qt.locale("en_GB")
        compare(control.locale.name, "en_GB")

        //            M   T   W   T   F   S   S
        var en_GB = [31,  1,  2,  3,  4,  5,  6,
                      7,  8,  9, 10, 11, 12, 13,
                     14, 15, 16, 17, 18, 19, 20,
                     21, 22, 23, 24, 25, 26, 27,
                     28, 29, 30, 31,  1,  2,  3,
                      4,  5,  6,  7,  8,  9, 10]

        for (var i = 0; i < 42; ++i)
            compare(control.contentItem.children[i].text, en_GB[i].toString())

        // en_US
        control.locale = Qt.locale("en_US")
        compare(control.locale.name, "en_US")

        //            S   M   T   W   T   F   S
        var en_US = [30, 31,  1,  2,  3,  4,  5,
                      6,  7,  8,  9, 10, 11, 12,
                     13, 14, 15, 16, 17, 18, 19,
                     20, 21, 22, 23, 24, 25, 26,
                     27, 28, 29, 30, 31,  1,  2,
                      3,  4,  5,  6,  7,  8,  9]

        for (var j = 0; j < 42; ++j)
            compare(control.contentItem.children[j].text, en_US[j].toString())

        control.destroy()
    }

    function test_range() {
        var control = component.createObject(testCase)

        control.month = 0
        compare(control.month, 0)

        ignoreWarning(Qt.resolvedUrl("tst_monthgrid.qml") + ":55:9: QML AbstractMonthGrid: month -1 is out of range [0...11]")
        control.month = -1
        compare(control.month, 0)

        control.month = 11
        compare(control.month, 11)

        ignoreWarning(Qt.resolvedUrl("tst_monthgrid.qml") + ":55:9: QML AbstractMonthGrid: month 12 is out of range [0...11]")
        control.month = 12
        compare(control.month, 11)

        control.year = -271820
        compare(control.year, -271820)

        ignoreWarning(Qt.resolvedUrl("tst_monthgrid.qml") + ":55:9: QML AbstractMonthGrid: year -271821 is out of range [-271820...275759]")
        control.year = -271821
        compare(control.year, -271820)

        control.year = 275759
        compare(control.year, 275759)

        ignoreWarning(Qt.resolvedUrl("tst_monthgrid.qml") + ":55:9: QML AbstractMonthGrid: year 275760 is out of range [-271820...275759]")
        control.year = 275760
        compare(control.year, 275759)

        control.destroy()
    }

    function test_bce() {
        var control = component.createObject(testCase)

        compare(control.contentItem.children.length, 6 * 7 + 1)

        // fi_FI
        control.locale = Qt.locale("fi_FI")
        compare(control.locale.name, "fi_FI")

        // January 1 BCE
        control.month = 0
        compare(control.month, 0)
        control.year = -1
        compare(control.year, -1)

        //              M   T   W   T   F   S   S
        var jan1bce = [27, 28, 29, 30, 31,  1,  2,
                        3,  4,  5,  6,  7,  8,  9,
                       10, 11, 12, 13, 14, 15, 16,
                       17, 18, 19, 20, 21, 22, 23,
                       24, 25, 26, 27, 28, 29, 30,
                       31,  1,  2,  3,  4,  5,  6]

        for (var i = 0; i < 42; ++i)
            compare(control.contentItem.children[i].text, jan1bce[i].toString())

        // February 1 BCE
        control.month = 1
        compare(control.month, 1)
        control.year = -1
        compare(control.year, -1)

        //              M   T   W   T   F   S   S
        var feb1bce = [31,  1,  2,  3,  4,  5,  6,
                        7,  8,  9, 10, 11, 12, 13,
                       14, 15, 16, 17, 18, 19, 20,
                       21, 22, 23, 24, 25, 26, 27,
                       28, 29,  1,  2,  3,  4,  5,
                        6,  7,  8,  9, 10, 11, 12]

        for (var j = 0; j < 42; ++j)
            compare(control.contentItem.children[j].text, feb1bce[j].toString())

        control.destroy()
    }

    function test_font() {
        var control = component.createObject(testCase)

        verify(control.contentItem.children[0])

        control.font.pixelSize = 123
        compare(control.contentItem.children[0].font.pixelSize, 123)

        control.destroy()
    }
}
