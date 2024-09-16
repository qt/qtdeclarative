// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

#include <qglobal.h>

#define WTF_MAKE_NONCOPYABLE(x) Q_DISABLE_COPY(x)

#endif // NONCOPYABLE_H
