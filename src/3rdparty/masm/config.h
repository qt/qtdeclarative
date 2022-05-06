/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
