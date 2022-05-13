// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// script.mjs
import { factorial } from "factorial.mjs"
export { factorial }

export function showCalculations(value) {
    console.log(
        "Call factorial() from script.js:",
        factorial(value));
}
//![0]
