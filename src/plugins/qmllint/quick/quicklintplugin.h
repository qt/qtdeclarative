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

#ifndef QUICKLINTPLUGIN_H
#define QUICKLINTPLUGIN_H

#include <QtPlugin>
#include <QtQmlCompiler/private/qqmlsa_p.h>

class QmlLintQuickPlugin : public QObject, public QQmlSA::LintPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLintPluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(QQmlSA::LintPlugin)

public:
    void registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement) override;
};

class LayoutChildrenValidatorPass : public QQmlSA::ElementPass
{
public:
    LayoutChildrenValidatorPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    QQmlSA::Element m_layout;
};

#endif // QUICKLINTPLUGIN_H
