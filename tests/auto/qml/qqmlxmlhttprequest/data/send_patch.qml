// Copyright (C) 2017 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    property string url

    property bool dataOK: false
    property bool headerOK: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open("PATCH", url);
        x.setRequestHeader("If-Match","\"ETagNumber\"");

        // Test to the end
        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.HEADERS_RECEIVED) {
                headerOK = (x.getResponseHeader("Content-Location") == "/qqmlxmlhttprequest.cpp") &&
                    (x.getResponseHeader("ETag") == "\"ETagNumber\"") &&
                    (x.status == "204");
            } else if (x.readyState == XMLHttpRequest.DONE) {
                dataOK = (x.responseText === "");
            }
        }

        var body = "--- a/qqmlxmlhttprequest.cpp\n" +
            "+++ b/qqmlxmlhttprequest.cpp\n" +
            "@@ -1238,11 +1238,13 @@\n" +
            "-    } else if (m_method == QLatin1String(\"OPTIONS\")) {\n" +
            "+    } else if (m_method == QLatin1String(\"OPTIONS\") ||\n" +
            "+            (m_method == QLatin1String(\"PATCH\"))) {\n"

        x.send(body);
    }
}
