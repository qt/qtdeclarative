/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGSHADERSOURCEBUILDER_P_H
#define QSGSHADERSOURCEBUILDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

#include <QtGui/qsurfaceformat.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QOpenGLShaderProgram;

class Q_QUICK_PRIVATE_EXPORT QSGShaderSourceBuilder
{
public:
    QSGShaderSourceBuilder();

    static void initializeProgramFromFiles(QOpenGLShaderProgram *program,
                                           const QString &vertexShader,
                                           const QString &fragmentShader);

    QByteArray source() const;
    void clear();

    void appendSource(const QByteArray &source);
    void appendSourceFile(const QString &fileName);
    void addDefinition(const QByteArray &definition);
    void removeVersion();

protected:
    virtual QString resolveShaderPath(const QString &path) const;

private:
    QSurfaceFormat::OpenGLContextProfile contextProfile() const;

    QByteArray m_source;
};

QT_END_NAMESPACE

#endif // QSGSHADERSOURCEBUILDER_P_H
