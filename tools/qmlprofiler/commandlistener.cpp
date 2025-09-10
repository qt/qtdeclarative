// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandlistener.h"
#include "constants.h"
#include <QtCore/QTextStream>

void CommandListener::readCommand()
{
    emit command(QTextStream(stdin).readLine());
}

#include "moc_commandlistener.cpp"
