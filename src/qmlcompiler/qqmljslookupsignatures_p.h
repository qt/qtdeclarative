// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLOOKUPSIGNATURES_P_H
#define QQMLJSLOOKUPSIGNATURES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtQml/qqmlprivate.h>

#include <private/qqmljsscope_p.h>

#include <qtqmlcompilerexports.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_EXPORT QQmlJSLookupSignaturesRecorder
{
public:
    QQmlJSLookupSignaturesRecorder(const QString &currentFilePath,
                                   const QQmlJSTypeResolver *typeResolver);

    QQmlPrivate::AOTLookupValidation::LookupSignatures signatures() const { return m_signatures; }

    void recordPropertyLookup(const QQmlJSScope::ConstPtr &base, const QQmlJSMetaProperty &prop);
    void recordMethodLookup(const QQmlJSScope::ConstPtr &base, const QQmlJSMetaMethod &method);
    void recordEnumKeyLookup(const QQmlJSScope::ConstPtr &base, const QQmlJSMetaEnum &metaEnum,
                             const QString &keyName);

private:
    bool cantDesync(const QQmlJSScope::ConstPtr &base) const;
    bool safeBase(const QQmlJSScope::ConstPtr &base) const;
    bool isUnnamedCompositeType(const QQmlJSScope::ConstPtr &type) const;

    QQmlPrivate::AOTLookupValidation::Type type(const QQmlJSScope::ConstPtr &type);

    const QString m_currentFilePath;
    const QQmlJSTypeResolver *m_typeResolver = nullptr;
    QQmlPrivate::AOTLookupValidation::LookupSignatures m_signatures;
};

QT_END_NAMESPACE

#endif // QQMLJSLOOKUPSIGNATURES_P_H
