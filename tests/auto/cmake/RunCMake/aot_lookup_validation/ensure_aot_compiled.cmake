# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(READ "${CMAKE_CURRENT_BINARY_DIR}/Main/.rcc/qmlcache/main_Main_qml.cpp.aotstats" CONTENTS)

# There is only one function. Check if codegenResult == Success
string(FIND "${CONTENTS}" "\"codegenResult\": 0" FOUND)

if(FOUND EQUAL -1)
  message(FATAL_ERROR "The function was not compiled to C++")
endif()
