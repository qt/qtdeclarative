// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "nameconflict.h"
#include <QtCore/qobject.h>

#ifndef NAME_CONFLICT_FLAG // some dummy check
#    error "NAME_CONFLICT_FLAG must be defined for this test"
#endif

class TriggersMoc : public QObject
{
    Q_OBJECT
};

#include "nameconflict.moc"
