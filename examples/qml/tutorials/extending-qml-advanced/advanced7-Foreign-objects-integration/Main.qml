// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import People
import QtQuick  // For QColor

BirthdayParty {
    id: party
    HappyBirthdaySong on announcement {
        name: party.host.name
    }

    display: ThirdPartyDisplay {
        foregroundColor: "black"
        backgroundColor: "white"
    }

    onPartyStarted: (time) => { console.log("This party started rockin' at " + time); }


    host: Boy {
        name: "Bob Jones"
        shoe { size: 12; color: "white"; brand: "Nike"; price: 90.0 }
    }

    Boy {
        name: "Leo Hodges"
        BirthdayParty.rsvp: Date.fromLocaleString(Qt.locale(), "2023-03-01", "yyyy-MM-dd")
        shoe { size: 10; color: "black"; brand: "Reebok"; price: 59.95 }
    }
    Boy {
        name: "Jack Smith"
        shoe { size: 8; color: "blue"; brand: "Puma"; price: 19.95 }
    }
    Girl {
        name: "Anne Brown"
        BirthdayParty.rsvp: Date.fromLocaleString(Qt.locale(), "2023-03-03", "yyyy-MM-dd")
        shoe.size: 7
        shoe.color: "red"
        shoe.brand: "Marc Jacobs"
        shoe.price: 99.99
    }
}
