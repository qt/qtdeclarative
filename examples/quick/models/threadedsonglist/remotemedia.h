// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef REMOTEMEDIA_H
#define REMOTEMEDIA_H
#include "mediaelement.h"

#include <QList>

class RemoteMedia
{
public:
    explicit RemoteMedia();

    /* Fast access to total count of items (so UI knows the total item list size). */
    static int count();

    /* Getter (100 milliseconds per item, function call is blocking the thread) */
    QList<MediaElement> getElements(int amount, int offset) const;

private:
    QList<MediaElement> m_items;
};

#endif // REMOTEMEDIA_H
