/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QV4COMPILATIONUNITMAPPER_H
#define QV4COMPILATIONUNITMAPPER_H

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

#include <private/qv4global_p.h>
#include <QFile>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace CompiledData {
struct Unit;
}

class CompilationUnitMapper
{
public:
    ~CompilationUnitMapper();

    CompiledData::Unit *get(
            const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString);
    static void invalidate(const QString &cacheFilePath);

private:
    CompiledData::Unit *open(
            const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString);
    void close();

#if defined(Q_OS_UNIX)
    size_t length = 0;
#endif
    void *dataPtr = nullptr;
};

}

QT_END_NAMESPACE

#endif // QV4COMPILATIONUNITMAPPER_H
