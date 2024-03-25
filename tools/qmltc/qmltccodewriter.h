// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCCODEWRITER_H
#define QMLTCCODEWRITER_H

#include "qmltcoutputprimitives.h"
#include "qmltcoutputir.h"

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#endif // QMLTCCODEWRITER_H
