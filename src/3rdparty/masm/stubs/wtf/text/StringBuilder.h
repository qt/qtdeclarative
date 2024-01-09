// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <wtf/text/WTFString.h>

namespace WTF {

struct StringBuilder : public String
{
    String toString() const { return *this; }
};

}

using WTF::StringBuilder;
