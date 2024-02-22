# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#! [0]
find_package(Qt6 REQUIRED COMPONENTS Qml)
target_link_libraries(mytarget PRIVATE Qt6::Qml)
#! [0]

#! [1]
find_package(Qt6 REQUIRED COMPONENTS QmlIntegration)
target_link_libraries(mytarget PRIVATE Qt6::QmlIntegration)
#! [1]

