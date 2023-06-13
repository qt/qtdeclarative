// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
export function backendStatusUpdate(backendStatus) {
    if (backendStatus === Backend.Error) {
        console.warn("Error!")
        return
    }

    console.log("Backend loaded successfully")
}
//! [file]
