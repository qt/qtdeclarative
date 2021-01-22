/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QQMLSCRIPTDATA_P_H
#define QQMLSCRIPTDATA_P_H

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

#include <private/qqmlrefcount_p.h>
#include <private/qqmlcleanup_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qv4value_p.h>
#include <private/qv4persistent_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QQmlContextData;

class Q_AUTOTEST_EXPORT QQmlScriptData : public QQmlRefCount
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptData();

public:
    QUrl url;
    QString urlString;
    QQmlRefPointer<QQmlTypeNameCache> typeNameCache;
    QVector<QQmlRefPointer<QQmlScriptBlob>> scripts;

    QV4::ReturnedValue scriptValueForContext(QQmlContextData *parentCtxt);

    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit() const { return m_precompiledScript; }

private:
    friend class QQmlScriptBlob;

    QQmlContextData *qmlContextDataForContext(QQmlContextData *parentQmlContextData);

    bool m_loaded;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_precompiledScript;
    QV4::PersistentValue m_value;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTDATA_P_H
