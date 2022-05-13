// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlprofilereventtype_p.h"
#include "qqmlprofilerclientdefinitions_p.h"

#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventType &type)
{
    quint8 message;
    quint8 rangeType;
    stream >> type.m_displayName >> type.m_data >> type.m_location >> message >> rangeType
           >> type.m_detailType;
    type.m_message = static_cast<Message>(message);
    type.m_rangeType = static_cast<RangeType>(rangeType);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventType &type)
{
    return stream << type.m_displayName << type.m_data << type.m_location
                  << static_cast<quint8>(type.m_message) << static_cast<quint8>(type.m_rangeType)
                  << type.m_detailType;
}

ProfileFeature QQmlProfilerEventType::feature() const
{
    switch (m_message) {
    case Event: {
        switch (m_detailType) {
        case Mouse:
        case Key:
            return ProfileInputEvents;
        case AnimationFrame:
            return ProfileAnimations;
        default:
            return MaximumProfileFeature;
        }
    }
    case PixmapCacheEvent:
        return ProfilePixmapCache;
    case SceneGraphFrame:
        return ProfileSceneGraph;
    case MemoryAllocation:
        return ProfileMemory;
    case DebugMessage:
        return ProfileDebugMessages;
    default:
        break;
    }

    switch (m_rangeType) {
        case Painting:
            return ProfilePainting;
        case Compiling:
            return ProfileCompiling;
        case Creating:
            return ProfileCreating;
        case Binding:
            return ProfileBinding;
        case HandlingSignal:
            return ProfileHandlingSignal;
        case Javascript:
            return ProfileJavaScript;
        default:
            return MaximumProfileFeature;
    }
}


QT_END_NAMESPACE
