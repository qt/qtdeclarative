// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "slow.h"

#include <QThread>

SlowStuff::SlowStuff()
{
    QThread::usleep(500000);
}

