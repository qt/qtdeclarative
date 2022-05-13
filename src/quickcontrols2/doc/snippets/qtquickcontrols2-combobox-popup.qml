// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick.Controls

ComboBox {
//! [closePolicy]
    popup.closePolicy: Popup.CloseOnEscape
//! [closePolicy]
//! [modal]
    popup.modal: true
//! [modal]
}
