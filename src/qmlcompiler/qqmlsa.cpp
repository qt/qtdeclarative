/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qqmlsa_p.h"

#include "qqmljsscope_p.h"
#include "qqmljslogger_p.h"
#include "qqmljstyperesolver_p.h"
#include "qqmljsimportvisitor_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlSA {

class GenericPassPrivate {
public:
    const PassManager *manager;
};

GenericPass::~GenericPass() = default;

GenericPass::GenericPass(PassManager *manager)
{
    Q_ASSERT(manager);
    d = std::make_unique<GenericPassPrivate>();
    d->manager = manager;
}

void GenericPass::emitWarning(QAnyStringView message, QQmlJS::SourceLocation srcLocation)
{
    d->manager->m_visitor->logger()->log(message.toString(), Log_Plugin, srcLocation);
}

Element GenericPass::resolveType(QAnyStringView moduleName, QAnyStringView typeName)
{
    auto typeImporter = d->manager->m_visitor->importer();
    auto module = typeImporter->importModule(moduleName.toString());
    return module[typeName.toString()].scope;
}

void SimplePropertyPass::run(const QQmlJSMetaProperty &property,
                             const QList<QQmlJSMetaPropertyBinding> &bindings)
{
    if (!bindings.isEmpty())
        run(property, bindings.constFirst());
    else
        run(property, {});
}

bool SimplePropertyPass::shouldRun(const Element &element, const QQmlJSMetaProperty &property,
                                   const QList<QQmlJSMetaPropertyBinding> &bindings)
{
    if (!bindings.isEmpty())
        return shouldRun(element, property, bindings.constFirst());
    else
        return shouldRun(element, property, {});
}

/*!
 * \brief PassManager::registerElementPass registers ElementPass
          with the pass manager.
   \param pass The registered pass. Ownership is transferred to the pass manager.
 */
void PassManager::registerElementPass(std::unique_ptr<ElementPass> pass)
{
    m_elementPasses.push_back(std::move(pass));
}

void PassManager::registerPropertyPass(std::unique_ptr<PropertyPass> pass)
{
    m_propertyPasses.push_back(std::move(pass));
}

void PassManager::analyze(const Element &root)
{
    QList<Element> runStack;
    runStack.push_back(root);
    while (!runStack.isEmpty()) {
        auto element = runStack.takeLast();
        for (auto &elementPass : m_elementPasses)
            if (elementPass->shouldRun(element))
                elementPass->run(element);
        const auto ownPropertyBindings = element->ownPropertyBindings();
        for (auto it = ownPropertyBindings.keyBegin(); it != ownPropertyBindings.keyEnd(); ++it) {
            const auto bindings = element->propertyBindings(*it);
            for (auto &propertyPass : m_propertyPasses)
                if (propertyPass->shouldRun(element, element->property(*it), bindings))
                    propertyPass->run(element->property(*it), bindings);
        }

        for (auto it = element->childScopesBegin(); it != element->childScopesEnd(); ++it) {
            if ((*it)->scopeType() == QQmlJSScope::QMLScope)
                runStack.push_back(*it);
        }
    }
}

bool PassManager::hasImportedModule(QAnyStringView module) const
{
    return m_visitor->imports().contains(u"$module$." + module.toString());
}

void DebugElementPass::run(const Element &element) {
    emitWarning(u"Type: " + element->baseTypeName());
    if (auto bindings = element->propertyBindings(u"objectName"_s); !bindings.isEmpty()) {
        emitWarning(u"is named: " + bindings.first().stringValue());
    }
    if (auto defPropName = element->defaultPropertyName(); !defPropName.isEmpty()) {
        emitWarning(u"binding " + QString::number(element->propertyBindings(defPropName).size())
                    + u" elements to property "_s + defPropName);
    }
}

void DebugPropertyPass::run(const QQmlJSMetaProperty &property,
                            const QList<QQmlJSMetaPropertyBinding> &bindings)
{
    emitWarning(u">> Property name: " + property.propertyName() + u"(binding count: "
                + QString::number(bindings.count()) + u')');
}

bool PropertyPass::shouldRun(const Element &, const QQmlJSMetaProperty &, const QList<QQmlJSMetaPropertyBinding> &)
{
    return true;
}

bool ElementPass::shouldRun(const Element &)
{
    return true;
}

}

QT_END_NAMESPACE
