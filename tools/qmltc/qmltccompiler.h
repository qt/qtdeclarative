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

#ifndef QMLTCCOMPILER_H
#define QMLTCCOMPILER_H

#include "qmltctyperesolver.h"
#include "qmltcvisitor.h"
#include "qmltcoutputir.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>

#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo
{
    QString outputCppFile;
    QString outputHFile;
    QString outputNamespace;
    QString resourcePath;
};

class QmltcCompiler
{
public:
    QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QmltcVisitor *visitor,
                  QQmlJSLogger *logger);
    void compile(const QmltcCompilerInfo &info);

private:
    QString m_url; // QML input file url
    QmltcTypeResolver *m_typeResolver = nullptr;
    QmltcVisitor *m_visitor = nullptr;
    QQmlJSLogger *m_logger = nullptr;
    QmltcCompilerInfo m_info {}; // miscellaneous input/output information

    void compileUrlMethod(QmltcMethod &urlMethod, const QString &urlMethodName);
    void compileType(QmltcType &current, const QQmlJSScope::ConstPtr &type);
    void compileEnum(QmltcType &current, const QQmlJSMetaEnum &e);
    void compileMethod(QmltcType &current, const QQmlJSMetaMethod &m);
    void compileProperty(QmltcType &current, const QQmlJSMetaProperty &p,
                         const QQmlJSScope::ConstPtr &owner);

    /*!
        \internal

        Helper structure that holds the information necessary for most bindings,
        such as accessor name, which is used to reference the properties. For
        example:
        > (accessor.name)->(propertyName) results in "this->myProperty"

        This data is also used in more advanced scenarios by attached and
        grouped properties
    */
    struct BindingAccessorData
    {
        QQmlJSScope::ConstPtr scope; // usually the current type
        QString name = u"this"_qs;
        QString propertyName = QString();
        bool isValueType = false;
    };
    void compileBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                        const QQmlJSScope::ConstPtr &type, const BindingAccessorData &accessor);

    bool hasErrors() const { return m_logger->hasErrors() || m_logger->hasWarnings(); }
    void recordError(const QQmlJS::SourceLocation &location, const QString &message,
                     QQmlJSLoggerCategory category = Log_Compiler)
    {
        // pretty much any compiler error is a critical error (we cannot
        // generate code - compilation fails)
        m_logger->logCritical(message, category, location);
    }
    void recordError(const QV4::CompiledData::Location &location, const QString &message,
                     QQmlJSLoggerCategory category = Log_Compiler)
    {
        recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message,
                    category);
    }
};

QT_END_NAMESPACE

#endif // QMLTCCOMPILER_H
