// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// ![0]
import QtQuick
import People

BirthdayParty {
    host: Person {
        name: "Bob Jones"
        shoeSize: 12
    }
    guests: [
        Person { name: "Leo Hodges" },
        Person { name: "Jack Smith" },
        Person { name: "Anne Brown" }
    ]

    Component.onCompleted: invite("William Green")
}
// ![0]
