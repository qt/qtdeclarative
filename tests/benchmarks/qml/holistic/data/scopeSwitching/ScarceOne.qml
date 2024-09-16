// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import Qt.test 1.0

Item {
    id: scarceResourceConsumer

    property MyScarceResourceProvider a: MyScarceResourceProvider { id: scarceResourceProvider }

    property variant ssr;
    property variant lsr;

    function copyScarceResources() {
        ssr = scarceResourceProvider.smallScarceResource;
        lsr = scarceResourceProvider.largeScarceResource;
    }
}
