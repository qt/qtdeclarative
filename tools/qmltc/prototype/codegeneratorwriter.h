/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CODEGENERATORWRITER_H
#define CODEGENERATORWRITER_H

#include "generatedcodeprimitives.h"
#include "qmlcompiler.h"

// writes compiled code into the GeneratedCode structure
struct CodeGeneratorWriter
{
    static void writeGlobalHeader(GeneratedCodeUtils &code, const QString &sourceName,
                                  const QString &hPath, const QString &cppPath,
                                  const QString &outNamespace,
                                  const QSet<QString> &requiredCppIncludes);
    static void writeGlobalFooter(GeneratedCodeUtils &code, const QString &sourceName,
                                  const QString &hPath, const QString &cppPath,
                                  const QString &outNamespace);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotObject &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotEnum &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotVariable &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotProperty &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotMethod &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSAotSpecialMethod &compiled);
    static void write(GeneratedCodeUtils &code, const QQmlJSProgram &compiled);

private:
    static void writeUrl(GeneratedCodeUtils &code, const QQmlJSAotMethod &urlMethod);
};

#endif // CODEGENERATORWRITER_H
