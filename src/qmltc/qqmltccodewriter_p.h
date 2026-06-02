// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTCCODEWRITER_P_H
#define QQMLTCCODEWRITER_P_H

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

#include <private/qqmltcoutputprimitives_p.h>
#include <private/qqmltcoutputir_p.h>

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QQmltc {

struct QmltcCodeWriter
{
    static void writeGlobalHeader(QmltcOutputWrapper &code, const QString &sourcePath,
                                  const QString &hPath, const QString &cppPath,
                                  const QString &outNamespace,
                                  const QSet<QString> &requiredCppIncludes);
    static void writeGlobalFooter(QmltcOutputWrapper &code, const QString &sourcePath,
                                  const QString &outNamespace);
    static void write(QmltcOutputWrapper &code, const QmltcProgram &program);
    static void write(QmltcOutputWrapper &code, const QmltcType &type, const QString &exportMacro);
    static void write(QmltcOutputWrapper &code, const QmltcEnum &enumeration);
    static void write(QmltcOutputWrapper &code, const QmltcMethod &method);
    static void write(QmltcOutputWrapper &code, const QmltcCtor &ctor);
    static void write(QmltcOutputWrapper &code, const QmltcDtor &dtor);
    static void write(QmltcOutputWrapper &code, const QmltcVariable &var);
    static void write(QmltcOutputWrapper &code, const QmltcProperty &prop);
    static void write(QmltcOutputWrapper &code, const QmltcPropertyInitializer &propertyInitializer, const QmltcType& wrappedType);
    static void write(QmltcOutputWrapper &code, const QmltcRequiredPropertiesBundle &requiredPropertiesBundle);

private:
    static void writeUrl(QmltcOutputWrapper &code, const QmltcMethod &urlMethod); // special
};

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCCODEWRITER_P_H
