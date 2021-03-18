/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef TYPEDESCRIPTIONREADER_H
#define TYPEDESCRIPTIONREADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "scopetree.h"

#include <QtQml/private/qqmljsastfwd_p.h>

// for Q_DECLARE_TR_FUNCTIONS
#include <QtCore/qcoreapplication.h>

struct ModuleApiInfo
{
    QString uri;
    ComponentVersion version;
    QString cppName;
};

class TypeDescriptionReader
{
    Q_DECLARE_TR_FUNCTIONS(TypeDescriptionReader)
public:
    TypeDescriptionReader() = default;
    explicit TypeDescriptionReader(QString fileName, QString data)
        : m_fileName(std::move(fileName)), m_source(std::move(data)) {}

    bool operator()(
            QHash<QString, ScopeTree::ConstPtr> *objects,
            QList<ModuleApiInfo> *moduleApis,
            QStringList *dependencies);

    QString errorMessage() const { return m_errorMessage; }
    QString warningMessage() const { return m_warningMessage; }

private:
    void readDocument(QQmlJS::AST::UiProgram *ast);
    void readModule(QQmlJS::AST::UiObjectDefinition *ast);
    void readDependencies(QQmlJS::AST::UiScriptBinding *ast);
    void readComponent(QQmlJS::AST::UiObjectDefinition *ast);
    void readModuleApi(QQmlJS::AST::UiObjectDefinition *ast);
    void readSignalOrMethod(QQmlJS::AST::UiObjectDefinition *ast, bool isMethod,
                            const ScopeTree::Ptr &scope);
    void readProperty(QQmlJS::AST::UiObjectDefinition *ast, const ScopeTree::Ptr &scope);
    void readEnum(QQmlJS::AST::UiObjectDefinition *ast, const ScopeTree::Ptr &scope);
    void readParameter(QQmlJS::AST::UiObjectDefinition *ast, MetaMethod *metaMethod);

    QString readStringBinding(QQmlJS::AST::UiScriptBinding *ast);
    bool readBoolBinding(QQmlJS::AST::UiScriptBinding *ast);
    double readNumericBinding(QQmlJS::AST::UiScriptBinding *ast);
    ComponentVersion readNumericVersionBinding(QQmlJS::AST::UiScriptBinding *ast);
    int readIntBinding(QQmlJS::AST::UiScriptBinding *ast);
    void readExports(QQmlJS::AST::UiScriptBinding *ast, const ScopeTree::Ptr &scope);
    void readMetaObjectRevisions(QQmlJS::AST::UiScriptBinding *ast, const ScopeTree::Ptr &scope);
    void readEnumValues(QQmlJS::AST::UiScriptBinding *ast, MetaEnum *metaEnum);

    void addError(const QQmlJS::SourceLocation &loc, const QString &message);
    void addWarning(const QQmlJS::SourceLocation &loc, const QString &message);

    QString m_fileName;
    QString m_source;
    QString m_errorMessage;
    QString m_warningMessage;
    QHash<QString, ScopeTree::ConstPtr> *m_objects = nullptr;
    QList<ModuleApiInfo> *m_moduleApis = nullptr;
    QStringList *m_dependencies = nullptr;
};

#endif // TYPEDESCRIPTIONREADER_H
