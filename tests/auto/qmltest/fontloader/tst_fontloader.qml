// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.1
import QtTest 1.1

Item {
    id: top

    FontLoader {
        id: fontloader
    }

    FontLoader {
        id: fontswitch
    }

    TextInput {
        id: testinput
        font.family: fontloader.name
    }



    TestCase {
        name: "FontLoader"

        function test_fontloading() {
            compare(fontloader.status, FontLoader.Null)
            compare(testinput.font.family, "")
            fontloader.source = "tarzeau_ocr_a.ttf";
            tryCompare(fontloader, 'status', FontLoader.Ready)
            compare(testinput.font.family, "OCRA")
            fontloader.source = "dummy.ttf";
            tryCompare(fontloader, 'status', FontLoader.Error)
            compare(testinput.font.family, "")
        }

        function test_fontswitching() {
            compare(fontswitch.status, FontLoader.Null)
            fontswitch.source = "tarzeau_ocr_a.ttf";
            tryCompare(fontswitch, 'status', FontLoader.Ready)
            compare(fontswitch.name, "OCRA")
            fontswitch.source = "daniel.ttf";
            tryCompare(fontswitch, 'status', FontLoader.Ready)
            compare(fontswitch.name, "Daniel")
            fontswitch.source = "tarzeau_ocr_a.ttf";
            tryCompare(fontswitch, 'status', FontLoader.Ready)
            compare(fontswitch.name, "OCRA")
        }
    }
}
