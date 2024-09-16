// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: rootId
    function recursive(n: int): int {
        if (n > 3)
            return 1 + recursive(recursive(x-1) + recursive(x-2) - recursive(x-3));
        else
            return recursive(0);
    }

    property int helloRecursive: recursive(42)

    Rectangle {
        function f() {
            return rootId.recursive(123)
        }
    }

    RecursiveInOtherFile {
        id: fromOtherFile
    }

    property int helloRecursiveFromOtherFile: fromOtherFile.recursive(42)
}
