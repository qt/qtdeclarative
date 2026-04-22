// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKITEMGENERATOR_P_H
#define QQUICKITEMGENERATOR_P_H

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

#include "qquickqmlgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <QStack>

QT_BEGIN_NAMESPACE

class QQuickMatrix4x4;
class QQuickAnimatedProperty;
class QQmlContext;
class QQmlIncubator;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickItemGenerator : public QQuickQmlGenerator
{
public:
    QQuickItemGenerator(const QString fileName,
                        QQuickVectorImageGenerator::GeneratorFlags flags);
    ~QQuickItemGenerator();
};

QT_END_NAMESPACE

#endif // QQUICKITEMGENERATOR_P_H
