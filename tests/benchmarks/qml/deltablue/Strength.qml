// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
pragma Singleton
import QtQml

QtObject {
    enum Value {
        REQUIRED         = 0,
        STRONG_PREFERRED = 1,
        PREFERRED        = 2,
        STRONG_DEFAULT   = 3,
        NORMAL           = 4,
        WEAK_DEFAULT     = 5,
        WEAKEST          = 6
    }

    function stronger(s1: int, s2: int) : bool {
        return s1 < s2;
    }

    function weaker(s1: int, s2: int) : bool {
        return s1 > s2;
    }

    function weakestOf(s1: int, s2: int) : int {
        return weaker(s1, s2) ? s1 : s2;
    }

    function strongestOf(s1: int, s2: int) : int {
        return stronger(s1, s2) ? s1 : s2;
    }

    function nextWeaker(s: int) : int {
        switch (s) {
            case 0: return Strength.WEAKEST;
            case 1: return Strength.WEAK_DEFAULT;
            case 2: return Strength.NORMAL;
            case 3: return Strength.STRONG_DEFAULT;
            case 4: return Strength.PREFERRED;
            case 5: return Strength.REQUIRED;
        }
    }
}
