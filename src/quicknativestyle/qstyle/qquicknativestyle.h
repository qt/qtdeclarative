/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKNATIVESTYLE_H
#define QQUICKNATIVESTYLE_H

#include "qquickstyle.h"

QT_BEGIN_NAMESPACE

namespace QQC2 {

class QQuickNativeStyle
{
public:
    static void setStyle(QStyle *style)
    {
        if (s_style)
            delete s_style;
        s_style = style;
    }

    inline static QStyle *style()
    {
        return s_style;
    }

private:
    static QStyle *s_style;
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QQUICKNATIVESTYLE_H
