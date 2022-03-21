/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "lintplugin.h"

using namespace Qt::StringLiterals;

class ElementTest : public QQmlSA::ElementPass
{
public:
    ElementTest(QQmlSA::PassManager *manager) : QQmlSA::ElementPass(manager)
    {
        m_rectangle = resolveType(u"QtQuick", u"Rectangle");
    }

    bool shouldRun(const QQmlSA::Element &element) override
    {
        return element->baseType() == m_rectangle;
    }

    void run(const QQmlSA::Element &element) override
    {
        auto property = element->property(u"radius"_s);
        if (!property.isValid() || element->property(u"radius"_s).typeName() != u"double") {
            emitWarning(u"Failed to verify radius property", element->sourceLocation());
            return;
        }

        auto bindings = element->propertyBindings(u"radius"_s);
        if (bindings.isEmpty() || bindings.constFirst().numberValue() != 5) {
            emitWarning(u"Failed to verify radius property binding", element->sourceLocation());
            return;
        }

        emitWarning(u"ElementTest OK", element->sourceLocation());
    }

private:
    QQmlSA::Element m_rectangle;
};

class PropertyTest : public QQmlSA::PropertyPass
{
public:
    PropertyTest(QQmlSA::PassManager *manager) : QQmlSA::PropertyPass(manager)
    {
        m_image = resolveType(u"QtQuick", u"Image");
    }

    bool shouldRun(const QQmlSA::Element &element, const QQmlJSMetaProperty &property,
                   const QList<QQmlJSMetaPropertyBinding> &bindings) override
    {
        return !bindings.isEmpty() && element->baseType() == m_image
                && property.propertyName() == u"x";
    }

    void run(const QQmlJSMetaProperty &property,
             const QList<QQmlJSMetaPropertyBinding> &bindings) override
    {
        if (property.typeName() != u"double") {
            emitWarning(u"Failed to verify x property");
            return;
        }

        bool foundInterceptor = false, foundValue = false;
        for (const QQmlJSMetaPropertyBinding &binding : bindings) {
            switch (binding.bindingType()) {
            case QQmlJSMetaPropertyBinding::Interceptor:
                foundInterceptor = true;
                break;
            case QQmlJSMetaPropertyBinding::NumberLiteral:
                foundValue = true;
                if (binding.numberValue() != 5) {
                    emitWarning(u"Binding has wrong value");
                    return;
                }
                break;
            default:
                emitWarning(u"Found unexpected binding on x property");
                return;
            }
        }

        if (!foundInterceptor || !foundValue) {
            emitWarning(u"Didn't see all expected bindings");
            return;
        }

        emitWarning(u"PropertyTest OK");
    }

private:
    QQmlSA::Element m_image;
};

class HasImportedModuleTest : public QQmlSA::ElementPass
{
public:
    HasImportedModuleTest(QQmlSA::PassManager *manager, QString message)
        : QQmlSA::ElementPass(manager), m_message(message)
    {
    }

    bool shouldRun(const QQmlSA::Element &element) override
    {
        Q_UNUSED(element)
        return true;
    }

    void run(const QQmlSA::Element &element) override
    {
        Q_UNUSED(element)
        emitWarning(m_message);
    }

private:
    QString m_message;
};

void LintPlugin::registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement)
{
    if (!rootElement->filePath().endsWith(u"_pluginTest.qml"))
        return;

    manager->registerElementPass(std::make_unique<ElementTest>(manager));
    manager->registerPropertyPass(std::make_unique<PropertyTest>(manager));
    if (manager->hasImportedModule("QtQuick.Controls")) {
        if (manager->hasImportedModule("QtQuick")) {
            if (manager->hasImportedModule("QtQuick.Window")) {
                manager->registerElementPass(std::make_unique<HasImportedModuleTest>(
                        manager, "QtQuick.Controls, QtQuick and QtQuick.Window present"));
            }
        } else {
            manager->registerElementPass(std::make_unique<HasImportedModuleTest>(
                    manager, "QtQuick.Controls and NO QtQuick present"));
        }
    }
}
