// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GREETER_H
#define GREETER_H

#include <QtCore/qobject.h>
#include <string>
#include <swift/bridging>

namespace HelloSwift {
class SwiftGreeter;
class Test;
}

template <typename SwiftType, typename QtType>
class SwiftWrapper
{
public:
    template <typename ...Args>
    SwiftWrapper(Args && ...args)
        : swiftImpl(new SwiftType(
            SwiftType::init(
                static_cast<QtType*>(this),
                std::forward<Args>(args)...)))
    {
    }

protected:
    std::unique_ptr<SwiftType> swiftImpl;
};

class Greeter : public QObject, public SwiftWrapper<HelloSwift::SwiftGreeter, Greeter>
{
    Q_OBJECT
public:
    Q_PRIVATE_PROPERTY(swiftImpl, std::string greeting READ getGreeting NOTIFY greetingChanged)
    Q_PRIVATE_SLOT(swiftImpl, void updateGreeting())
    Q_SIGNAL void greetingChanged();
} SWIFT_UNSAFE_REFERENCE;


#endif // GREETER_H
