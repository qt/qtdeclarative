// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GADGETWITHENUM_H
#define GADGETWITHENUM_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class GadgetWithEnum : public QObject {
    Q_GADGET
    QML_ELEMENT

public:
    enum State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };
    Q_ENUM(State)
};

#endif // GADGETWITHENUM_H
