// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickchecklabel_p.h"

QT_BEGIN_NAMESPACE

QQuickCheckLabel::QQuickCheckLabel(QQuickItem *parent) :
    QQuickText(parent)
{
    setHAlign(AlignLeft);
    setVAlign(AlignVCenter);
    setElideMode(ElideRight);
}

QT_END_NAMESPACE

#include "moc_qquickchecklabel_p.cpp"
