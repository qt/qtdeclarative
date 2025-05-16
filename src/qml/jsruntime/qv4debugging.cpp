// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qv4debugging_p.h"

#if QT_CONFIG(qml_debug)

QT_BEGIN_NAMESPACE

QV4::Debugging::Debugger::~Debugger()
    = default;

QT_END_NAMESPACE

#include "moc_qv4debugging_p.cpp"

#endif // QT_CONFIG(qml_debug)
