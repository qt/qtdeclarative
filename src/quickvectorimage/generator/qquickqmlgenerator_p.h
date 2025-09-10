// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKQMLGENERATOR_P_H
#define QQUICKQMLGENERATOR_P_H

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

#include "qquickgenerator_p.h"

#include <QtCore/qtextstream.h>
#include <QtCore/qbuffer.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickQmlGenerator : public QQuickGenerator
{
public:
    QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName);
    ~QQuickQmlGenerator();

    void setShapeTypeName(const QString &name);
    QString shapeTypeName() const;

    void setCommentString(const QString commentString);
    QString commentString() const;

    void setRetainFilePaths(bool retainFilePaths)
    {
        m_retainFilePaths = retainFilePaths;
    }

    bool retainFilePaths() const
    {
        return m_retainFilePaths;
    }

    void setAssetFileDirectory(const QString &assetFileDirectory)
    {
        m_assetFileDirectory = assetFileDirectory;
    }

    QString assetFileDirectory() const
    {
        return m_assetFileDirectory;
    }

    void setAssetFilePrefix(const QString &assetFilePrefix)
    {
        m_assetFilePrefix = assetFilePrefix;
    }

    QString assetFilePrefix() const
    {
        return m_assetFilePrefix;
    }

protected:
    void generateNodeBase(const NodeInfo &info) override;
    bool generateDefsNode(const NodeInfo &info) override;
    void generateImageNode(const ImageNodeInfo &info) override;
    void generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect) override;
    void generateNode(const NodeInfo &info) override;
    void generateTextNode(const TextNodeInfo &info) override;
    void generateUseNode(const UseNodeInfo &info) override;
    bool generateStructureNode(const StructureNodeInfo &info) override;
    bool generateRootNode(const StructureNodeInfo &info) override;
    void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect) override;

private:
    void generateGradient(const QGradient *grad);
    void generateTransform(const QTransform &xf);
    void generatePathContainer(const StructureNodeInfo &info);

    QStringView indent();
    enum StreamFlags { NoFlags = 0x0, SameLine = 0x1 };
    QTextStream &stream(int flags = NoFlags);
    const char *shapeName() const;

private:
    int m_indentLevel = 0;
    QBuffer m_result;
    QTextStream m_stream;
    QString outputFileName;
    bool m_inShapeItem = false;
    QByteArray m_shapeTypeName;
    QString m_commentString;
    bool m_retainFilePaths = false;
    QString m_assetFileDirectory;
    QString m_assetFilePrefix;
};

QT_END_NAMESPACE

#endif // QQUICKQMLGENERATOR_P_H
