// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import Greeter

public class SwiftGreeter {
    private let qtImpl: Greeter

    public init(greeter: Greeter ) {
        self.qtImpl = greeter;
    }

    public var greeting: String {
        let greetings = ["Hello", "Howdy", "Hey", "Hola", "Heisan"]
        return greetings[Int.random(in: 0..<greetings.count)]
    }

    public func updateGreeting() {
        qtImpl.greetingChanged();
    }
}
