// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef STATE_H
#define STATE_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace model {
namespace Window {
Q_NAMESPACE
QML_NAMED_ELEMENT(WindowState)
enum State {
    OPEN,
    TILT,
    UNCALIBRATED,
    CLOSE
};
Q_ENUM_NS(State)
}

class WindowInstance : public QObject
{
    Q_PROPERTY(int count READ count CONSTANT FINAL)
    Q_OBJECT
    QML_ELEMENT
public:
    int count() { return 11; }
};

}

#endif // STATE_H
