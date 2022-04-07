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

#include "quicklintplugin.h"

LayoutChildrenValidatorPass::LayoutChildrenValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
    m_layout = resolveType("QtQuick.Layouts", "Layout");
}

bool LayoutChildrenValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_layout.isNull() && element->parentScope()
            && element->parentScope()->inherits(m_layout);
}

void LayoutChildrenValidatorPass::run(const QQmlSA::Element &element)
{
    auto bindings = element->propertyBindings(u"anchors"_qs);
    if (!bindings.empty())
        emitWarning(u"Detected anchors on an item that is managed by a layout. This is undefined "
                    u"behavior; use Layout.alignment instead.",
                    element->sourceLocation());
}

void QmlLintQuickPlugin::registerPasses(QQmlSA::PassManager *manager,
                                        const QQmlSA::Element &rootElement)
{
    Q_UNUSED(rootElement);
    manager->registerElementPass(std::make_unique<LayoutChildrenValidatorPass>(manager));
}
