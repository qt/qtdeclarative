// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "slow.h"

#include <QThread>

SlowStuff::SlowStuff()
{
    QThread::usleep(500000);
}

