/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtTest 1.2
import QtQuick 6.0

TestCase {
    id: testCase
    width: 800
    height: 600
    visible: true
    when: windowShown

    property var items: []
    property int items_count: 1000
    property int itemWidth : 50
    property int itemHeight: 50

    // A reference point (added upon request)
    function benchmark_do_nothing_with_palettes() {}

    // This test passes through all items in tree to resolve a color. No extra palettes created.
    // Should be blazingly fast (approx. 0.01 msecs for 999 items).
    function benchmark_color_lookup() {
        // Make last item "visible" and create its palette
        items[items_count - 1].color = items[items_count - 1].palette.button

        compare(items[0].palette.button, items[items_count - 1].palette.button,
                "Color is not propagated to the last element.")
    }

    // This test creates palettes for all elements in the tree.
    function benchmark_create_all_palettes(data) {
        populate_palettes()
        check_palettes()
    }

    // This test creates and resolves palettes for all elements in the tree.
    function benchmark_create_and_resolve_all_palettes() {
        populate_palettes()
        resolve_palettes()
        check_palettes()
    }

    function init() {
        // Re-create all items on each iteration of the benchmark.

        var componentStr = "import QtQuick 6.0;

                            Rectangle {
                                x: mapFromItem(testCase, testCase.randomX(), testCase.randomY()).x
                                y: mapFromItem(testCase, testCase.randomX(), testCase.randomY()).y

                                color: \"#7F696969\"

                                width: testCase.itemWidth
                                height: testCase.itemHeight
                            }";
        items.push(createTemporaryQmlObject(componentStr, testCase))
        for (var i = 1; i < items_count; ++i) {
            items.push(createTemporaryQmlObject(componentStr, items[i - 1]))
        }

        // Create a pallet for item 0
        items[0].palette.button = randomRgba()

        // Make item "visible" (can be overlapped by children)
        items[0].color = items[0].palette.button
    }

    function cleanup() {
        // Explicitly remove all "temporary" items to make sure that a memory
        // will be released after each iteration of the benchmark.
        for (var i = 0; i < items_count; ++i) {
            items[i].destroy();
        }

        items = [];
    }

    function randomColorComponent() {
        return Math.floor(Math.random() * 256) / 255.0;
    }

    function randomRgba() {
        return Qt.rgba(randomColorComponent(),
                       randomColorComponent(),
                       randomColorComponent(),
                       randomColorComponent());
    }

    function randomCoordinate(len, itemLen) { return Math.floor(Math.random() * (len - itemLen + 1)); }
    function randomX() { return randomCoordinate(width, itemWidth); }
    function randomY() { return randomCoordinate(height, itemHeight); }

    function populate_palettes() {
        for (var i = 1; i < items_count; ++i) {
            items[i].color = items[i].palette.button
        }
    }

    function check_palettes() {
        for (var j = 1; j < items_count; ++j) {
            compare(items[j - 1].palette.button, items[j].palette.button,
                    "Color is not propagated to the next child element.")
        }
    }

    function resolve_palettes() {
        // The loop is just in case. Doesn't affect the benchmark
        do {
            var randomColor = randomRgba()
        } while (items[0].palette.button === randomColor)

        items[0].palette.button = randomColor
    }
}
