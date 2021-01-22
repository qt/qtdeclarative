/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick module of the Qt Toolkit.
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

#ifndef QQUICKSVGPARSER_P_H
#define QQUICKSVGPARSER_P_H

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
#include <QtCore/qstring.h>
#include <QtGui/qpainterpath.h>

QT_BEGIN_NAMESPACE

namespace QQuickSvgParser
{
    bool parsePathDataFast(const QString &dataStr, QPainterPath &path);
    Q_QUICK_PRIVATE_EXPORT void pathArc(QPainterPath &path, qreal rx, qreal ry, qreal x_axis_rotation,
                                        int large_arc_flag, int sweep_flag, qreal x, qreal y, qreal curx,
                                        qreal cury);
}

QT_END_NAMESPACE

#endif // QQUICKSVGPARSER_P_H
