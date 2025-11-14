// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQml.Models

//! [age-filter]
FunctionFilter {
   component RoleData: QtObject { property int age }
   function filter(data: RoleData) : bool {
      return data.age > 30
   }
}
//! [age-filter]
