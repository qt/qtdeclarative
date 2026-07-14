// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGCOMMANDLISTENER_P_H
#define QQMLDEBUGCOMMANDLISTENER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qtextstream.h>

QT_BEGIN_NAMESPACE

// Reads commands from standard input. Meant to be moved to a worker thread so
// that the blocking read does not stall the main event loop.
class QQmlDebugCommandListener : public QObject
{
    Q_OBJECT
public:
    void readCommand();

Q_SIGNALS:
    void command(const QString &command);
    void endOfInput();

private:
    QTextStream m_input{ stdin };
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCOMMANDLISTENER_P_H
