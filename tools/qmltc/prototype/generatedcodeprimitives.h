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

#ifndef GENERATEDCODEPRIMITIVES_H
#define GENERATEDCODEPRIMITIVES_H

#include <QtCore/qstring.h>
#include <QtCore/qstack.h>

// holds generated code for header and implementation files
struct GeneratedCode
{
    QString header;
    QString implementation;
};

// utility class that provides pretty-printing of the generated code into the
// GeneratedCode buffer
struct GeneratedCodeUtils
{
    GeneratedCode &m_code; // buffer

    QStack<QString> memberNamespaceStack; // member names scopes e.g. MyClass::MySubclass::
    int headerIndent = 0; // header indentation level
    int implIndent = 0; // implementation indentation level

    GeneratedCodeUtils(GeneratedCode &code) : m_code(code) { }

    // manages current scope of the generated code, which is necessary for
    // implementation file generation. Example:
    // class MyClass { MyClass(); };    - in header
    // MyClass::MyClass() {}            - in implementation file
    // MemberNamespaceScope exists to be able to record and use "MyClass::"
    struct MemberNamespaceScope
    {
        GeneratedCodeUtils &m_code;
        MemberNamespaceScope(GeneratedCodeUtils &code, const QString &str) : m_code(code)
        {
            m_code.memberNamespaceStack.push(str);
        }
        ~MemberNamespaceScope() { m_code.memberNamespaceStack.pop(); }
    };

    // manages current indentation scope: upon creation, increases current
    // scope, which is decreased back upon deletion. this is used by append*
    // functions that work with GeneratedCode::header to correctly indent the
    // input
    struct HeaderIndentationScope
    {
        GeneratedCodeUtils &m_code;
        HeaderIndentationScope(GeneratedCodeUtils &code) : m_code(code) { ++m_code.headerIndent; }
        ~HeaderIndentationScope() { --m_code.headerIndent; }
    };

    // manages current indentation scope: upon creation, increases current
    // scope, which is decreased back upon deletion. this is used by append*
    // functions that work with GeneratedCode::implementation to correctly
    // indent the input
    struct ImplIndentationScope
    {
        GeneratedCodeUtils &m_code;
        ImplIndentationScope(GeneratedCodeUtils &code) : m_code(code) { ++m_code.implIndent; }
        ~ImplIndentationScope() { --m_code.implIndent; }
    };

    // appends string \a what with extra indentation \a extraIndent to current
    // GeneratedCode::header string
    template<typename String>
    void appendToHeader(const String &what, int extraIndent = 0)
    {
        constexpr char16_t newLine[] = u"\n";
        m_code.header += QString((headerIndent + extraIndent) * 4, u' ') + what + newLine;
    }

    // appends string \a what with extra indentation \a extraIndent to current
    // GeneratedCode::implementation string
    template<typename String>
    void appendToImpl(const String &what, int extraIndent = 0)
    {
        constexpr char16_t newLine[] = u"\n";
        m_code.implementation += QString((implIndent + extraIndent) * 4, u' ') + what + newLine;
    }

    // appends string \a what with extra indentation \a extraIndent to current
    // GeneratedCode::implementation string. this is a special case function
    // that expects \a what to be a function signature as \a what is prepended
    // with member scope related text. for example, string "foo()" is converted
    // to string "MyClass::foo()" before append
    template<typename String>
    void appendSignatureToImpl(const String &what, int extraIndent = 0)
    {
        constexpr char16_t newLine[] = u"\n";
        QString signatureScope;
        for (const auto &subScope : memberNamespaceStack)
            signatureScope += subScope + u"::";
        m_code.implementation +=
                signatureScope + QString((implIndent + extraIndent) * 4, u' ') + what + newLine;
    }
};

#endif // GENERATEDCODEPRIMITIVES_H
