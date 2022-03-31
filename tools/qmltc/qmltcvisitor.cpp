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

#include "qmltcvisitor.h"

#include <QtCore/qfileinfo.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static QString uniqueNameFromPieces(const QStringList &pieces, QHash<QString, int> &repetitions)
{
    QString possibleName = pieces.join(u'_');
    const int count = repetitions[possibleName]++;
    if (count > 0)
        possibleName.append(u"_" + QString::number(count));
    return possibleName;
}

QmltcVisitor::QmltcVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                           const QString &implicitImportDirectory, const QStringList &qmldirFiles)
    : QQmlJSImportVisitor(
            QQmlJSScope::create(), importer, logger, implicitImportDirectory, qmldirFiles)
{
    m_qmlTypeNames.append(QFileInfo(logger->fileName()).baseName()); // put document root
}

void QmltcVisitor::findCppIncludes()
{
    // TODO: this pass is fairly slow: we have to do exhaustive search because
    // some C++ code could do forward declarations
    QSet<const QQmlJSScope *> visitedTypes; // we can still improve by walking all types only once
    const auto visitType = [&visitedTypes](const QQmlJSScope::ConstPtr &type) -> bool {
        if (visitedTypes.contains(type.data()))
            return true;
        visitedTypes.insert(type.data());
        return false;
    };

    const auto populateFromType = [&](const QQmlJSScope::ConstPtr &type) {
        if (!type) // TODO: it is a crutch
            return;
        if (visitType(type)) // optimization - don't call nonCompositeBaseType() needlessly
            return;
        auto t = QQmlJSScope::nonCompositeBaseType(type);
        if (t != type && visitType(t))
            return;

        QString includeFile = t->fileName();
        if (!includeFile.isEmpty())
            m_cppIncludes.insert(std::move(includeFile));
    };

    const auto constructPrivateInclude = [](QStringView publicInclude) -> QString {
        if (publicInclude.isEmpty())
            return QString();
        Q_ASSERT(publicInclude.endsWith(u".h"_qs) || publicInclude.endsWith(u".hpp"_qs));
        const qsizetype dotLocation = publicInclude.lastIndexOf(u'.');
        QStringView extension = publicInclude.sliced(dotLocation);
        QStringView includeWithoutExtension = publicInclude.first(dotLocation);
        // check if "public" include is in fact already private
        if (publicInclude.startsWith(u"private"))
            return includeWithoutExtension + u"_p" + extension;
        return u"private/" + includeWithoutExtension + u"_p" + extension;
    };

    // walk the whole type hierarchy
    for (const QQmlJSScope::ConstPtr &type : qAsConst(m_qmlTypes)) {
        // TODO: figure how to NOT walk all the types. theoretically, we can
        // stop at first non-composite type
        for (auto t = type; t; t = t->baseType()) {
            if (visitType(t))
                break;
            // look in type
            if (auto includeFile = t->fileName(); !includeFile.isEmpty())
                m_cppIncludes.insert(std::move(includeFile));

            // look in properties
            const auto properties = t->ownProperties();
            for (const QQmlJSMetaProperty &p : properties) {
                populateFromType(p.type());

                if (p.isPrivate()) {
                    const QString ownersInclude = t->fileName();
                    QString privateInclude = constructPrivateInclude(ownersInclude);
                    if (!privateInclude.isEmpty())
                        m_cppIncludes.insert(std::move(privateInclude));
                }
            }

            // look in methods
            const auto methods = t->ownMethods();
            for (const QQmlJSMetaMethod &m : methods) {
                populateFromType(m.returnType());
                // TODO: debug Q_ASSERT(m.returnType())
                const auto parameters = m.parameterTypes();
                for (const auto &param : parameters)
                    populateFromType(param);
            }
        }
    }
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectDefinition *object)
{
    if (!QQmlJSImportVisitor::visit(object))
        return false;

    // we're not interested in non-QML scopes
    if (m_currentScope->scopeType() != QQmlJSScope::QMLScope)
        return true;

    Q_ASSERT(m_currentScope->internalName().isEmpty());
    Q_ASSERT(!m_currentScope->baseTypeName().isEmpty());
    if (m_currentScope != m_exportedRootScope) // not document root
        m_qmlTypeNames.append(m_currentScope->baseTypeName());
    // give C++-relevant internal names to QMLScopes, we can use them later in compiler
    m_currentScope->setInternalName(uniqueNameFromPieces(m_qmlTypeNames, m_qmlTypeNameCounts));

    if (auto base = m_currentScope->baseType(); base && base->isComposite())
        m_qmlTypesWithQmlBases.append(m_currentScope);

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *object)
{
    m_qmlTypeNames.removeLast();
    QQmlJSImportVisitor::endVisit(object);
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    if (!QQmlJSImportVisitor::visit(uiob))
        return false;

    Q_ASSERT(m_currentScope->scopeType() == QQmlJSScope::QMLScope);
    Q_ASSERT(m_currentScope->internalName().isEmpty());
    Q_ASSERT(!m_currentScope->baseTypeName().isEmpty());
    if (m_currentScope != m_exportedRootScope) // not document root
        m_qmlTypeNames.append(m_currentScope->baseTypeName());
    // give C++-relevant internal names to QMLScopes, we can use them later in compiler
    m_currentScope->setInternalName(uniqueNameFromPieces(m_qmlTypeNames, m_qmlTypeNameCounts));

    if (auto base = m_currentScope->baseType(); base && base->isComposite())
        m_qmlTypesWithQmlBases.append(m_currentScope);

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    m_qmlTypeNames.removeLast();
    QQmlJSImportVisitor::endVisit(uiob);
}

bool QmltcVisitor::visit(QQmlJS::AST::UiPublicMember *publicMember)
{
    if (!QQmlJSImportVisitor::visit(publicMember))
        return false;

    // augment property: set its write/read/etc. methods
    if (publicMember->type == QQmlJS::AST::UiPublicMember::Property) {
        const auto name = publicMember->name.toString();
        QQmlJSMetaProperty prop = m_currentScope->ownProperty(name);
        const QString nameWithUppercase = name[0].toUpper() + name.sliced(1);
        prop.setRead(name);
        if (prop.isWritable())
            prop.setWrite(u"set" + nameWithUppercase);
        prop.setBindable(u"bindable" + nameWithUppercase);
        prop.setNotify(name + u"Changed");
        // also check that notify is already a method of m_currentScope
        {
            const auto methods = m_currentScope->ownMethods(prop.notify());
            if (methods.size() != 1) {
                const QString errorString =
                        methods.isEmpty() ? u"no signal"_qs : u"too many signals"_qs;
                m_logger->logCritical(
                        u"internal error: %1 found for property '%2'"_qs.arg(errorString, name),
                        Log_Compiler, publicMember->identifierToken);
                return false;
            } else if (methods[0].methodType() != QQmlJSMetaMethod::Signal) {
                m_logger->logCritical(
                        u"internal error: method %1 of property %2 must be a signal"_qs.arg(
                                prop.notify(), name),
                        Log_Compiler, publicMember->identifierToken);
                return false;
            }
        }
        m_currentScope->addOwnProperty(prop);
    }

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiProgram *program)
{
    QQmlJSImportVisitor::endVisit(program);

    findCppIncludes();
}

QT_END_NAMESPACE
