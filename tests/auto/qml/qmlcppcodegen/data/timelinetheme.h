// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TIMELINETHEME_H
#define TIMELINETHEME_H

#include "theme.h"

#include <QtQml/qqmlregistration.h>

class TimelineTheme : public Utils::Theme
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Theme)
    QML_SINGLETON

public:
    explicit TimelineTheme(QObject *parent = nullptr);
};

#endif // TIMELINETHEME_H
