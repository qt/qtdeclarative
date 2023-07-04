// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtNetwork

QtObject {
    property int local: NetworkInformation.Reachability.Local
    property int reachability: NetworkInformation.reachability
    property bool isBehindCaptivePortal: NetworkInformation.isBehindCaptivePortal
    property int ethernet: NetworkInformation.TransportMedium.Ethernet
    property int transportMedium: NetworkInformation.transportMedium
    property bool isMetered: NetworkInformation.isMetered
}
