// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COMMANDLISTENER_H
#define COMMANDLISTENER_H

#include <QtCore/QThread>

class CommandListener : public QObject {
    Q_OBJECT
public:
    void readCommand();

Q_SIGNALS:
    void command(const QString &command);
};

#endif // COMMANDLISTENER_H
