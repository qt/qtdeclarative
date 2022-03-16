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

#ifndef QQMLSA_P_H
#define QQMLSA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <private/qqmljsscope_p.h>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QQmlJSTypeResolver;
class QQmlJSImportVisitor;

namespace QQmlSA {

// ### FIXME: Replace with a proper PIMPL'd type
using Element = QQmlJSScope::ConstPtr;

class GenericPassPrivate;
class PassManager;

class Q_QMLCOMPILER_EXPORT GenericPass
{
public:
    Q_DISABLE_COPY_MOVE(GenericPass)
    GenericPass(PassManager *manager);
    virtual ~GenericPass();

    void emitWarning(QAnyStringView message,
                     QQmlJS::SourceLocation srcLocation = QQmlJS::SourceLocation());
    Element resolveType(QAnyStringView moduleName, QAnyStringView typeName); // #### TODO: revisions
private:
    std::unique_ptr<GenericPassPrivate> d; // PIMPL might be overkill
};

class Q_QMLCOMPILER_EXPORT ElementPass : public GenericPass
{
public:
    ElementPass(PassManager *manager) : GenericPass(manager) { }

    virtual bool shouldRun(const Element &element);
    virtual void run(const Element &element) = 0;
};

class Q_QMLCOMPILER_EXPORT PropertyPass : public GenericPass
{
public:
    PropertyPass(PassManager *manager) : GenericPass(manager) { }

    virtual bool shouldRun(const Element &element, const QQmlJSMetaProperty &property,
                           const QList<QQmlJSMetaPropertyBinding> &bindings);
    virtual void run(const QQmlJSMetaProperty &property,
                     const QList<QQmlJSMetaPropertyBinding> &bindings) = 0;
};

class SimplePropertyPass : public PropertyPass
{
protected:
    SimplePropertyPass(PassManager *manager) : PropertyPass(manager) { }

    virtual void run(const QQmlJSMetaProperty &property,
                     const QQmlJSMetaPropertyBinding &binding) = 0;
    virtual bool shouldRun(const Element &element, const QQmlJSMetaProperty &property,
                           const QQmlJSMetaPropertyBinding &binding) = 0;

private:
    void run(const QQmlJSMetaProperty &property,
             const QList<QQmlJSMetaPropertyBinding> &bindings) override;
    bool shouldRun(const Element &element, const QQmlJSMetaProperty &property,
                   const QList<QQmlJSMetaPropertyBinding> &bindings) override;
};

class Q_QMLCOMPILER_EXPORT LintPlugin
{
public:
    LintPlugin() = default;
    virtual ~LintPlugin() = default;

    Q_DISABLE_COPY_MOVE(LintPlugin)

    virtual void registerPasses(PassManager *manager, const Element &rootElement) = 0;
};

// ### FIXME: Make this (at least partially) private again as soon as possible
class Q_QMLCOMPILER_EXPORT PassManager
{
public:
    Q_DISABLE_COPY_MOVE(PassManager)

    friend class GenericPass;
    PassManager(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
        : m_visitor(visitor), m_typeResolver(resolver)
    {
        Q_UNUSED(m_typeResolver);
    }
    void registerElementPass(std::unique_ptr<ElementPass> pass);
    void registerPropertyPass(std::unique_ptr<PropertyPass> pass);
    void analyze(const Element &root);

private:
    std::vector<std::unique_ptr<ElementPass>> m_elementPasses;
    std::vector<std::unique_ptr<PropertyPass>> m_propertyPasses;
    QQmlJSImportVisitor *m_visitor;
    QQmlJSTypeResolver *m_typeResolver;
};

class Q_QMLCOMPILER_EXPORT DebugElementPass : public ElementPass
{
    void run(const Element &element) override;
};

class Q_QMLCOMPILER_EXPORT DebugPropertyPass : public PropertyPass
{
    virtual void run(const QQmlJSMetaProperty &property,
                     const QList<QQmlJSMetaPropertyBinding> &bindings) override;
};
}

#define QmlLintPluginInterface_iid "org.qt-project.Qt.Qml.SA.LintPlugin/1.0"

Q_DECLARE_INTERFACE(QQmlSA::LintPlugin, QmlLintPluginInterface_iid)

QT_END_NAMESPACE

#endif
