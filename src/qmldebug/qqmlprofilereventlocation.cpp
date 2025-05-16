// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qqmlprofilereventlocation_p.h"

#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventLocation &location)
{
    return stream >> location.m_filename >> location.m_line >> location.m_column;
}

QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventLocation &location)
{
    return stream << location.m_filename << location.m_line << location.m_column;
}

QT_END_NAMESPACE
