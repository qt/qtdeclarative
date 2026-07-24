// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTRANSFORMSOURCE_P_H
#define QQUICKTRANSFORMSOURCE_P_H

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

#include <QtQuickVectorImageGenerator/qtquickvectorimagegeneratorexports.h>

#include <QtGui/qmatrix4x4.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickTransformSource : public QQuickItem
{
    Q_OBJECT
public:
    using QQuickItem::QQuickItem;
    ~QQuickTransformSource() override;

    virtual QMatrix4x4 transformMatrix() = 0;

Q_SIGNALS:
    void transformMatrixChanged();
};

QT_END_NAMESPACE

#endif // QQUICKTRANSFORMSOURCE_P_H
