// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import People
import QtQuick  // For QColor

//! [begin]
BirthdayParty {
//! [begin]

//! [rsvp]
    Boy {
        name: "Robert Campbell"
        BirthdayParty.rsvp: "2009-07-01"
    }
//! [rsvp]
    // ![1]
    Boy {
        name: "Leo Hodges"
        shoe { size: 10; color: "black"; brand: "Reebok"; price: 59.95 }

        BirthdayParty.rsvp: "2009-07-06"
    }
    // ![1]
    host: Boy {
        name: "Jack Smith"
        shoe { size: 8; color: "blue"; brand: "Puma"; price: 19.95 }
    }
//! [end]
}
//! [end]

