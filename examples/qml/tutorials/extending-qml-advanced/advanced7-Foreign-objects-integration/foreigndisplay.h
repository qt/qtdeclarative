// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FOREIGNDISPLAY_H
#define FOREIGNDISPLAY_H

#include "ThirdPartyDisplay.h"

#include <QColor>
#include <QObject>
#include <qqml.h>

class ForeignDisplay : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ThirdPartyDisplay)
    QML_FOREIGN(ThirdPartyDisplay)
};

#endif // FOREIGNDISPLAY_H
