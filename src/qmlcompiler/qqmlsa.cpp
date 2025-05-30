// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmlsa_p.h"

#include "qqmljsscope_p.h"
#include "qqmljslogger_p.h"
#include "qqmljstyperesolver_p.h"
#include "qqmljsimportvisitor_p.h"
#include "qqmljsutils_p.h"

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

void GenericPass::emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                              QQmlJS::SourceLocation srcLocation)
{
    d->manager->m_visitor->logger()->log(diagnostic.toString(), id, srcLocation);
}


void GenericPass::emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                              QQmlJS::SourceLocation srcLocation, const FixSuggestion &fix)
{
    d->manager->m_visitor->logger()->log(diagnostic.toString(), id, srcLocation, true, true, fix);
}

Element GenericPass::resolveTypeInFileScope(QAnyStringView typeName)
{
    return d->manager->m_visitor->imports().type(typeName.toString()).scope;
}

Element GenericPass::resolveType(QAnyStringView moduleName, QAnyStringView typeName)
{
    auto typeImporter = d->manager->m_visitor->importer();
    auto module = typeImporter->importModule(moduleName.toString());
    return module.type(typeName.toString()).scope;
}

Element GenericPass::resolveLiteralType(const QQmlJSMetaPropertyBinding &binding)
{
    return binding.literalType(d->manager->m_typeResolver);
}

Element GenericPass::resolveIdToElement(QAnyStringView id, const Element &context)
{
    return d->manager->m_visitor->addressableScopes().scope(id.toString(), context);
}

QString GenericPass::resolveElementToId(const Element &element, const Element &context)
{
    return d->manager->m_visitor->addressableScopes().id(element, context);
}

QString GenericPass::sourceCode(QQmlJS::SourceLocation location)
{
    return d->manager->m_visitor->logger()->code().mid(location.offset, location.length);
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

enum LookupMode { Register, Lookup };
static QString lookupName(const QQmlSA::Element &element, LookupMode mode = Lookup)
{
    QString name;
    if (element.isNull() || element->internalName().isEmpty()) {
        // Bail out with an invalid name, this type is so screwed up we can't do anything reasonable
        // with it We should have warned about it in another plac
        if (element.isNull() || element->baseType().isNull())
            return u"$INVALID$"_s;
        name = element->baseType()->internalName();
    } else {
        name = element->internalName();
    }

    const QString filePath =
            (mode == Register || !element->baseType() ? element : element->baseType())->filePath();

    if (element->isComposite() && !filePath.endsWith(u".h"))
        name += u'@' + filePath;
    return name;
}

bool PassManager::registerPropertyPass(std::shared_ptr<PropertyPass> pass,
                                       QAnyStringView moduleName, QAnyStringView typeName,
                                       QAnyStringView propertyName, bool allowInheritance)
{
    QString name;
    if (!moduleName.isEmpty() && !typeName.isEmpty()) {
        auto typeImporter = m_visitor->importer();
        auto module = typeImporter->importModule(moduleName.toString());
        auto element = module.type(typeName.toString()).scope;

        if (element.isNull())
            return false;

        name = lookupName(element, Register);
    }
    const PassManager::PropertyPassInfo passInfo {
        propertyName.isEmpty() ? QStringList {} : QStringList { propertyName.toString() },
        std::move(pass), allowInheritance
    };
    m_propertyPasses.insert({ name, passInfo });

    return true;
}

void PassManager::addBindingSourceLocations(const Element &element, const Element &scope,
                                            const QString prefix, bool isAttached)
{
    const Element &currentScope = scope.isNull() ? element : scope;
    const auto ownBindings = currentScope->ownPropertyBindings();
    for (const auto &binding : ownBindings.values()) {
        switch (binding.bindingType()) {
        case QQmlJSMetaPropertyBinding::GroupProperty:
            addBindingSourceLocations(element, binding.groupType(),
                                      prefix + binding.propertyName() + u'.');
            break;
        case QQmlJSMetaPropertyBinding::AttachedProperty:
            addBindingSourceLocations(element, binding.attachingType(),
                                      prefix + binding.propertyName() + u'.', true);
            break;
        default:
            m_bindingsByLocation.insert({ binding.sourceLocation().offset,
                                          BindingInfo { prefix + binding.propertyName(), binding,
                                                        currentScope, isAttached } });

            if (binding.bindingType() != QQmlJSMetaPropertyBinding::Script)
                analyzeBinding(element, QQmlSA::Element(), binding.sourceLocation());
        }
    }
}

void PassManager::analyze(const Element &root)
{
    QList<Element> runStack;
    runStack.push_back(root);
    while (!runStack.isEmpty()) {
        auto element = runStack.takeLast();
        addBindingSourceLocations(element);
        for (auto &elementPass : m_elementPasses)
            if (elementPass->shouldRun(element))
                elementPass->run(element);
        const auto ownPropertyBindings = element->ownPropertyBindings();

        for (auto it = element->childScopesBegin(); it != element->childScopesEnd(); ++it) {
            if ((*it)->scopeType() == QQmlJSScope::QMLScope)
                runStack.push_back(*it);
        }
    }
}

void PassManager::analyzeWrite(const Element &element, QString propertyName, const Element &value,
                               const Element &writeScope, QQmlJS::SourceLocation location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onWrite(element, propertyName, value, writeScope, location);
}

void PassManager::analyzeRead(const Element &element, QString propertyName,
                              const Element &readScope, QQmlJS::SourceLocation location)
{
    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onRead(element, propertyName, readScope, location);
}

void PassManager::analyzeBinding(const Element &element, const QQmlSA::Element &value,
                                 QQmlJS::SourceLocation location)
{
    const auto info = m_bindingsByLocation.find(location.offset);

    // If there's no matching binding that means we're in a nested Ret somewhere inside an
    // expression
    if (info == m_bindingsByLocation.end())
        return;

    const QQmlSA::Element &bindingScope = info->second.bindingScope;
    const QQmlJSMetaPropertyBinding &binding = info->second.binding;
    const QString &propertyName = info->second.fullPropertyName;

    for (PropertyPass *pass : findPropertyUsePasses(element, propertyName))
        pass->onBinding(element, propertyName, binding, bindingScope, value);

    if (!info->second.isAttached || bindingScope->baseType().isNull())
        return;

    for (PropertyPass *pass : findPropertyUsePasses(bindingScope->baseType(), propertyName))
        pass->onBinding(element, propertyName, binding, bindingScope, value);
}

bool PassManager::hasImportedModule(QAnyStringView module) const
{
    return m_visitor->imports().hasType(u"$module$." + module.toString());
}

bool PassManager::isCategoryEnabled(LoggerWarningId category) const
{
    return !m_visitor->logger()->isCategoryIgnored(category);
}

void PassManager::setCategoryEnabled(LoggerWarningId category, bool enabled)
{
    m_visitor->logger()->setCategoryIgnored(category, !enabled);
}

QSet<PropertyPass *> PassManager::findPropertyUsePasses(const QQmlSA::Element &element,
                                                        const QString &propertyName)
{
    QStringList typeNames { lookupName(element) };

    QQmlJSUtils::searchBaseAndExtensionTypes(
            element, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                Q_UNUSED(mode);
                typeNames.append(lookupName(scope));
                return false;
            });

    QSet<PropertyPass *> passes;

    for (const QString &typeName : typeNames) {
        for (auto &pass :
             { m_propertyPasses.equal_range(u""_s), m_propertyPasses.equal_range(typeName) }) {
            if (pass.first == pass.second)
                continue;

            for (auto it = pass.first; it != pass.second; it++) {
                if (typeName != typeNames.constFirst() && !it->second.allowInheritance)
                    continue;
                if (it->second.properties.isEmpty()
                    || it->second.properties.contains(propertyName)) {
                    passes.insert(it->second.pass.get());
                }
            }
        }
    }
    return passes;
}

void DebugElementPass::run(const Element &element) {
    emitWarning(u"Type: " + element->baseTypeName(), qmlPlugin);
    if (auto bindings = element->propertyBindings(u"objectName"_s); !bindings.isEmpty()) {
        emitWarning(u"is named: " + bindings.first().stringValue(), qmlPlugin);
    }
    if (auto defPropName = element->defaultPropertyName(); !defPropName.isEmpty()) {
        emitWarning(u"binding " + QString::number(element->propertyBindings(defPropName).size())
                            + u" elements to property "_s + defPropName,
                    qmlPlugin);
    }
}

bool ElementPass::shouldRun(const Element &)
{
    return true;
}

PropertyPass::PropertyPass(PassManager *manager) : GenericPass(manager) { }

void PropertyPass::onBinding(const Element &element, const QString &propertyName,
                             const QQmlJSMetaPropertyBinding &binding, const Element &bindingScope,
                             const Element &value)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(binding);
    Q_UNUSED(bindingScope);
    Q_UNUSED(value);
}

void PropertyPass::onRead(const Element &element, const QString &propertyName,
                          const Element &readScope, QQmlJS::SourceLocation location)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(readScope);
    Q_UNUSED(location);
}

void PropertyPass::onWrite(const Element &element, const QString &propertyName,
                           const Element &value, const Element &writeScope,
                           QQmlJS::SourceLocation location)
{
    Q_UNUSED(element);
    Q_UNUSED(propertyName);
    Q_UNUSED(writeScope);
    Q_UNUSED(value);
    Q_UNUSED(location);
}

DebugPropertyPass::DebugPropertyPass(QQmlSA::PassManager *manager) : QQmlSA::PropertyPass(manager)
{
}

void DebugPropertyPass::onRead(const QQmlSA::Element &element, const QString &propertyName,
                               const QQmlSA::Element &readScope, QQmlJS::SourceLocation location)
{
    emitWarning(u"onRead "_s
                        + (element->internalName().isEmpty() ? element->baseTypeName()
                                                             : element->internalName())
                        + u' ' + propertyName + u' ' + readScope->internalName() + u' '
                        + QString::number(location.startLine) + u':'
                        + QString::number(location.startColumn),
                qmlPlugin, location);
}

void DebugPropertyPass::onBinding(const QQmlSA::Element &element, const QString &propertyName,
                                  const QQmlJSMetaPropertyBinding &binding,
                                  const QQmlSA::Element &bindingScope, const QQmlSA::Element &value)
{
    const auto location = binding.sourceLocation();
    emitWarning(u"onBinding element: '"_s
                        + (element->internalName().isEmpty() ? element->baseTypeName()
                                                             : element->internalName())
                        + u"' property: '"_s + propertyName + u"' value: '"_s
                        + (value.isNull()
                                   ? u"NULL"_s
                                   : (value->internalName().isNull() ? value->baseTypeName()
                                                                     : value->internalName()))
                        + u"' binding_scope: '"_s
                        + (bindingScope->internalName().isEmpty() ? bindingScope->baseTypeName()
                                                                  : bindingScope->internalName())
                        + u"' "_s + QString::number(location.startLine) + u':'
                        + QString::number(location.startColumn),
                qmlPlugin, location);
}

void DebugPropertyPass::onWrite(const QQmlSA::Element &element, const QString &propertyName,
                                const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                                QQmlJS::SourceLocation location)
{
    emitWarning(u"onWrite "_s + element->baseTypeName() + u' ' + propertyName + u' '
                        + value->internalName() + u' ' + writeScope->internalName() + u' '
                        + QString::number(location.startLine) + u':'
                        + QString::number(location.startColumn),
                qmlPlugin, location);
}
}

QT_END_NAMESPACE
