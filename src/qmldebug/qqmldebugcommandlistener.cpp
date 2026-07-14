// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmldebugcommandlistener_p.h"

QT_BEGIN_NAMESPACE

void QQmlDebugCommandListener::readCommand()
{
    const QString line = m_input.readLine();
    if (line.isNull())
        emit endOfInput();
    else
        emit command(line);
}

QT_END_NAMESPACE

#include "moc_qqmldebugcommandlistener_p.cpp"
