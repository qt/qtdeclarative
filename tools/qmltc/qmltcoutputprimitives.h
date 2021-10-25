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

#ifndef QMLTCOUTPUTPRIMITIVES_H
#define QMLTCOUTPUTPRIMITIVES_H

#include <QtCore/qstack.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringbuilder.h>

QT_BEGIN_NAMESPACE

struct QmltcOutput
{
    QString header;
    QString cpp;
};

// TODO: this must adhere to C++ generated code templates (once introduced)
class QmltcOutputWrapper
{
    QmltcOutput &m_code;

    template<typename String>
    static void rawAppend(QString &out, const String &what, int extraIndent = 0)
    {
        constexpr char16_t newLine[] = u"\n";
        out += QString(extraIndent * 4, u' ') + what + newLine;
    }

public:
    QmltcOutputWrapper(QmltcOutput &code) : m_code(code) { }
    const QmltcOutput &code() const { return m_code; }

    QStack<QString> memberScopes; // member name scopes e.g. MyClass::MySubclass::
    int headerIndent = 0; // header indentation level
    int cppIndent = 0; // cpp indentation level

    // manages current scope of the generated code, which is necessary for
    // cpp file generation. Example:
    // class MyClass { MyClass(); };    - in header
    // MyClass::MyClass() {}            - in cpp
    // MemberNameScope makes sure "MyClass::" is recorded
    struct MemberNameScope
    {
        QmltcOutputWrapper *m_code;
        MemberNameScope(QmltcOutputWrapper *code, const QString &str) : m_code(code)
        {
            m_code->memberScopes.push(str);
        }
        ~MemberNameScope() { m_code->memberScopes.pop(); }
        Q_DISABLE_COPY_MOVE(MemberNameScope)
    };

    struct HeaderIndentationScope
    {
        QmltcOutputWrapper *m_code;
        HeaderIndentationScope(QmltcOutputWrapper *code) : m_code(code) { ++m_code->headerIndent; }
        ~HeaderIndentationScope() { --m_code->headerIndent; }
        Q_DISABLE_COPY_MOVE(HeaderIndentationScope)
    };

    struct CppIndentationScope
    {
        QmltcOutputWrapper *m_code;
        CppIndentationScope(QmltcOutputWrapper *code) : m_code(code) { ++m_code->cppIndent; }
        ~CppIndentationScope() { --m_code->cppIndent; }
        Q_DISABLE_COPY_MOVE(CppIndentationScope)
    };

    // appends string \a what with extra indentation \a extraIndent to current
    // header string
    template<typename String>
    void rawAppendToHeader(const String &what, int extraIndent = 0)
    {
        rawAppend(m_code.header, what, headerIndent + extraIndent);
    }

    // appends string \a what with extra indentation \a extraIndent to current
    // cpp string
    template<typename String>
    void rawAppendToCpp(const String &what, int extraIndent = 0)
    {
        rawAppend(m_code.cpp, what, cppIndent + extraIndent);
    }

    // special case of rawAppendToCpp that makes sure that string "foo()"
    // becomes "MyClass::foo()"
    template<typename String>
    void rawAppendSignatureToCpp(const String &what, int extraIndent = 0)
    {
        QString signatureScope;
        for (const auto &scope : memberScopes)
            signatureScope += scope + u"::";
        rawAppendToCpp(signatureScope + what, extraIndent);
    }
};

QT_END_NAMESPACE

#endif // QMLTCOUTPUTPRIMITIVES_H
