// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "commandlistener.h"
#include "constants.h"
#include <QtCore/QTextStream>

void CommandListener::readCommand()
{
    emit command(QTextStream(stdin).readLine());
}

#include "moc_commandlistener.cpp"
