/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef QQMLIRLOADER_P_H
#define QQMLIRLOADER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsmemorypool_p.h>

QT_BEGIN_NAMESPACE

namespace QmlIR {
struct Document;
struct Object;
}

struct Q_QML_PRIVATE_EXPORT QQmlIRLoader {
    QQmlIRLoader(const QV4::CompiledData::Unit *unit, QmlIR::Document *output);

    void load();

private:
    QmlIR::Object *loadObject(const QV4::CompiledData::Object *serializedObject);

    template <typename _Tp> _Tp *New() { return pool->New<_Tp>(); }

    const QV4::CompiledData::Unit *unit;
    QmlIR::Document *output;
    QQmlJS::MemoryPool *pool;
};

QT_END_NAMESPACE

#endif // QQMLIRLOADER_P_H
