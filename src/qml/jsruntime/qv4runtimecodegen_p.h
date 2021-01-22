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
#ifndef QV4RUNTIMECODEGEN_P_H
#define QV4RUNTIMECODEGEN_P_H

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

#include <private/qv4codegen_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

class RuntimeCodegen : public Compiler::Codegen
{
public:
    RuntimeCodegen(ExecutionEngine *engine, Compiler::JSUnitGenerator *jsUnitGenerator, bool strict)
        : Codegen(jsUnitGenerator, strict)
        , engine(engine)
    {}

    void generateFromFunctionExpression(const QString &fileName,
                                        const QString &sourceCode,
                                        QQmlJS::AST::FunctionExpression *ast,
                                        Compiler::Module *module);

    void throwSyntaxError(const QQmlJS::SourceLocation &loc, const QString &detail) override;
    void throwReferenceError(const QQmlJS::SourceLocation &loc, const QString &detail) override;

private:
    ExecutionEngine *engine;
};

}

QT_END_NAMESPACE

#endif // QV4CODEGEN_P_H
