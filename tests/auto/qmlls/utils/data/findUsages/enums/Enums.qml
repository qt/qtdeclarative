// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {

    enum Cats {
        Patron = 8,
        Mafik = 7,
        Kivrik = 2
    }

    property var inner: QtObject {
        enum Cats {
            Patron = -8, // Shouldn't be found
            Mafik = -7,
            Kivrik = -2
        }
    }

    property int main: Enums.Cats.Patron
    property int innerVal: Enums.Patron
    property int illegal1: Cats.Patron // Shouldn't be found
    property int illegal2: Patron // Shouldn't be found
    property int alien: EnumsFromAnotherFile.FromAnotherUniverse
}
