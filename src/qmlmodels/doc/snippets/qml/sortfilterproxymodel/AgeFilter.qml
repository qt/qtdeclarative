// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQml.Models

//! [age-filter]
FunctionFilter {
   function filter(age: int) : bool {
      return age > 30
   }
}
//! [age-filter]
