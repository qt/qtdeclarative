// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    function switchStatement(){
        const animals = "cat";
        const no = 0;
        switch (animals) {
            case "cat":
                switch (no) {
                    case 0: return "patron";
                    case 1: return "mafik";
                    default: return "none";
                }
            case "dog": {
                // check if qqmljsscope is created for this case
                const name = "tekila";
                let another = "mutantx";
                return name;
            }
            default: return "monster";
            case "moreCases!":
                return "moreCaseClauses?"
        }
    }
}
