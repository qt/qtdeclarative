// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3
import QtTest 1.1

TestCase {
    name: "Third"

    function init_data() {
        return [
            { tag: "init_0" },
            { tag: "skip_1" },
            { tag: "init_2" },
            { tag: "skip_3" },
            { tag: "init_4" },
        ]
    }

    function test_default_tags(data) {
        if (data.tag.startsWith("skip_"))
            skip("skip '" + data.tag + "' tag")
    }

    function test_tags_data() {
         return [
             { tag: "foo" },
             { tag: "bar" },
             { tag: "baz" },
         ]
    }

    function test_tags(data) {
        if (data.tag === "bar")
            skip("skip '" + data.tag + "' tag")
    }
}
