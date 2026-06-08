// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

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

struct CodeWriter
{
    static void writeGlobalHeader(OutputWrapper &code, const QString &sourcePath,
                                  const QString &hPath, const QString &cppPath,
                                  const QString &outNamespace,
                                  const QSet<QString> &requiredCppIncludes);
    static void writeGlobalFooter(OutputWrapper &code, const QString &sourcePath,
                                  const QString &outNamespace);
    static void write(OutputWrapper &code, const Program &program);
    static void write(OutputWrapper &code, const Type &type, const QString &exportMacro);
    static void write(OutputWrapper &code, const Enum &enumeration);
    static void write(OutputWrapper &code, const Method &method);
    static void write(OutputWrapper &code, const Ctor &ctor);
    static void write(OutputWrapper &code, const Dtor &dtor);
    static void write(OutputWrapper &code, const Variable &var);
    static void write(OutputWrapper &code, const Property &prop);
    static void write(OutputWrapper &code, const PropertyInitializer &propertyInitializer, const Type& wrappedType);
    static void write(OutputWrapper &code, const RequiredPropertiesBundle &requiredPropertiesBundle);

private:
    static void writeUrl(OutputWrapper &code, const Method &urlMethod); // special
};

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCCODEWRITER_P_H
