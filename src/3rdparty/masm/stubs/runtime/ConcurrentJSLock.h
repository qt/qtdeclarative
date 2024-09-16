// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

namespace JSC {

class NoLock {
public:
    void lock() { }
    void unlock() { }
    bool isHeld() { return false; }
};

typedef NoLock ConcurrentJSLock;

} // namespace JSC
