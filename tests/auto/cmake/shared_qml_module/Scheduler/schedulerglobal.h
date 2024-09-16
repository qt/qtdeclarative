// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SCHEDULEGLOBAL_H
#define SCHEDULEGLOBAL_H

#include <QtGlobal>

#if defined(SCHEDULER_LIBRARY)
#define SCHEDULER_EXPORT Q_DECL_EXPORT
#else
#define SCHEDULER_EXPORT Q_DECL_IMPORT
#endif

#endif // SCHEDULEGLOBAL_H
