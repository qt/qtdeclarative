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

#include "qmltccompiler.h"

QT_BEGIN_NAMESPACE

QmltcCompiler::QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QQmlJSLogger *logger)
    : m_url(url), m_typeResolver(resolver), m_logger(logger)
{
    Q_UNUSED(m_url);
    Q_UNUSED(m_typeResolver);
    Q_UNUSED(m_logger);
}

void QmltcCompiler::compile(const QmltcCompilerInfo &info)
{
    m_info = info;
}

QT_END_NAMESPACE
