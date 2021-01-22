/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
