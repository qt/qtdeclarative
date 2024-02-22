// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

ComboBox {
//! [closePolicy]
    popup.closePolicy: Popup.CloseOnEscape
//! [closePolicy]
//! [modal]
    popup.modal: true
//! [modal]
}
