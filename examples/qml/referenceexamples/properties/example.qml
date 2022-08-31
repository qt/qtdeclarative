// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import People

// ![0]
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
}
// ![0]
