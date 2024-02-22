// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import QtCore

QtObject {
    property int wordSize: SystemInformation.wordSize
    property int byteOrder: SystemInformation.byteOrder
    property string buildCpuArchitecture: SystemInformation.buildCpuArchitecture
    property string currentCpuArchitecture: SystemInformation.currentCpuArchitecture
    property string buildAbi: SystemInformation.buildAbi
    property string kernelType: SystemInformation.kernelType
    property string kernelVersion: SystemInformation.kernelVersion
    property string productType: SystemInformation.productType
    property string productVersion: SystemInformation.productVersion
    property string prettyProductName: SystemInformation.prettyProductName
    property string machineHostName: SystemInformation.machineHostName
    property var machineUniqueId: SystemInformation.machineUniqueId
    property var bootUniqueId: SystemInformation.bootUniqueId
}
