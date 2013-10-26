/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgshadersourcebuilder_p.h"

#include <QtGui/qopenglshaderprogram.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

QSGShaderSourceBuilder::QSGShaderSourceBuilder()
{
}

void QSGShaderSourceBuilder::initializeProgramFromFiles(QOpenGLShaderProgram *program,
                                                        const QString &vertexShader,
                                                        const QString &fragmentShader)
{
    Q_ASSERT(program);
    program->removeAllShaders();

    QSGShaderSourceBuilder builder;

    builder.appendSourceFile(vertexShader);
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, builder.source());
    builder.clear();

    builder.appendSourceFile(fragmentShader);
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, builder.source());
}

QByteArray QSGShaderSourceBuilder::source() const
{
    return m_source;
}

void QSGShaderSourceBuilder::clear()
{
    m_source.clear();
}

void QSGShaderSourceBuilder::appendSource(const QByteArray &source)
{
    m_source += source;
}

void QSGShaderSourceBuilder::appendSourceFile(const QString &fileName)
{
    const QString resolvedFileName = resolveShaderPath(fileName);
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to find shader" << resolvedFileName;
        return;
    }
    m_source += f.readAll();
}

QString QSGShaderSourceBuilder::resolveShaderPath(const QString &path) const
{
    // For now, just return the path unaltered.
    // TODO: Resolve to more specific filename based upon OpenGL profile and
    // version, platform, GPU type etc
    return path;
}

QT_END_NAMESPACE
