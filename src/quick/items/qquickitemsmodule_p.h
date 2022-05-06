/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKITEMSMODULE_P_H
#define QQUICKITEMSMODULE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>
#include <QtGui/qevent.h>
#include <QtGui/qpointingdevice.h>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QQuickItemsModule
{
public:
    static void defineModule();
};

struct QInputDeviceForeign
{
    Q_GADGET
    QML_FOREIGN(QInputDevice)
    QML_NAMED_ELEMENT(InputDevice)
    QML_ADDED_IN_VERSION(6, 0)
    QML_UNCREATABLE("InputDevice is only available via read-only properties.")
};

struct QPointingDeviceForeign
{
    Q_GADGET
    QML_FOREIGN(QPointingDevice)
    QML_NAMED_ELEMENT(PointerDevice)
    QML_ADDED_IN_VERSION(2, 12)
    QML_UNCREATABLE("PointerDevice is only available via read-only properties.")
};

struct QPointingDeviceUniqueIdForeign
{
    Q_GADGET
    QML_FOREIGN(QPointingDeviceUniqueId)
    QML_VALUE_TYPE(pointingDeviceUniqueId)
    QML_ADDED_IN_VERSION(2, 9)
};

#if !QT_CONFIG(quick_animatedimage)
struct QQuickAnimatedImageNotAvailable
{
    Q_GADGET
    QML_UNAVAILABLE
    QML_NAMED_ELEMENT(AnimatedImage)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Qt was built without support for QMovie.")
};
#endif

QT_END_NAMESPACE

#endif // QQUICKITEMSMODULE_P_H

