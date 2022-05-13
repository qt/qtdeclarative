// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQuickV8PARTICLEDATA_H
#define QQuickV8PARTICLEDATA_H

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

#include <private/qv4persistent_p.h>
#include <private/qv4value_p.h>

QT_BEGIN_NAMESPACE

class QQuickParticleData;
class QQuickParticleSystem;
class QQuickV4ParticleData {
public:
    QQuickV4ParticleData(QV4::ExecutionEngine*, QQuickParticleData*, QQuickParticleSystem *system);
    ~QQuickV4ParticleData();
    QV4::ReturnedValue v4Value() const;
private:
    QV4::PersistentValue m_v4Value;
};


QT_END_NAMESPACE


#endif
