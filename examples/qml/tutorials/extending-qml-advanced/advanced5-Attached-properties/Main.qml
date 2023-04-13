// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import People
import QtQuick  // For QColor

BirthdayParty {
    Boy {
        name: "Robert Campbell"
        BirthdayParty.rsvp: Date.fromLocaleString(Qt.locale(), "2023-03-01", "yyyy-MM-dd")
    }

    Boy {
        name: "Leo Hodges"
        shoe { size: 10; color: "black"; brand: "Reebok"; price: 59.95 }
        BirthdayParty.rsvp: Date.fromLocaleString(Qt.locale(), "2023-03-03", "yyyy-MM-dd")
    }

    host: Boy {
        name: "Jack Smith"
        shoe { size: 8; color: "blue"; brand: "Puma"; price: 19.95 }
    }
}
