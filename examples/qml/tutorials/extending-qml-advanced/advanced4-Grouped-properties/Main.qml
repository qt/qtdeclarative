// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import People
import QtQuick  // For QColor

BirthdayParty {
    host: Boy {
        name: "Bob Jones"
        shoe { size: 12; color: "white"; brand: "Bikey"; price: 90.0 }
    }

    Boy {
        name: "Leo Hodges"
        shoe { size: 10; color: "black"; brand: "Thebok"; price: 59.95 }
    }
    Boy {
        name: "Jack Smith"
        shoe {
            size: 8
            color: "blue"
            brand: "Luma"
            price: 19.95
        }
    }
    Girl {
        name: "Anne Brown"
        shoe.size: 7
        shoe.color: "red"
        shoe.brand: "Job Macobs"
        shoe.price: 99.99
    }
}
