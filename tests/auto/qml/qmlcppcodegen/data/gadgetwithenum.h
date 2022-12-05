// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GADGETWITHENUM_H
#define GADGETWITHENUM_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

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

namespace GadgetWithEnumWrapper {
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(GadgetWithEnum)
    QML_NAMED_ELEMENT(NamespaceWithEnum)
};

#endif // GADGETWITHENUM_H
