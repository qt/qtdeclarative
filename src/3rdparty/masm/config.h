// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MASM_CONFIG_H
#define MASM_CONFIG_H

#include <wtf/Platform.h>
#ifdef __cplusplus
#include <wtf/Vector.h>
#include <wtf/FastAllocBase.h>
#include <wtf/RefPtr.h>
#include <cmath>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif // _MSC_VER
#else // !__cplusplus

#include <math.h>

#ifdef _MSC_VER
#define inline
#include <stdio.h>
#endif // _MSC_VER

#endif // __cplusplus
#include <limits.h>

#endif // MASM_CONFIG_H
