// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

QtObject {
    id: self

    property Component c: QtObject { objectName: "tracked" }
    property QtObject hidden: c.createObject()

    // Hands out the only remaining reference to "hidden", wrapped in a deeply
    // nested map. While this function is running, the object is kept alive by
    // the AOT-compiled function's tracked locals. Once it has returned, the
    // returned value is in flight: it lives in coerceAndCall()'s buffer, which
    // the garbage collector does not scan.
    //
    // The nesting matters. Converting the returned map to JS recurses once per
    // level and allocates on every level, so a collector that runs during the
    // conversion runs with plenty of C++ stack churn on top of the frame this
    // function just vacated.
    function takeHidden() : var {
        var m = { o: hidden }
        for (var i = 0; i < 300; ++i)
            m = { n: m }
        hidden = null
        return m
    }
}
