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
#include "qquicknodeinfo_p.h"
#include "qquickanimatedproperty_p.h"

#include <QtCore/qtextstream.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickQmlGenerator : public QQuickGenerator
{
public:
    QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName);
    ~QQuickQmlGenerator();

    bool save();

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

    void setUrlPrefix(const QString &prefix)
    {
        m_urlPrefix = prefix;
    }

    QString urlPrefix() const
    {
        return m_urlPrefix;
    }

    void addExtraImport(const QString &import) override { m_extraImports.append(import); }

    QStringList extraImports() const
    {
        return m_extraImports;
    }

    bool isRuntimeGenerator() const
    {
        return !m_urlPrefix.isEmpty();
    }

    QByteArray result() const
    {
        return m_result.data();
    }

protected:
    QString generateNodeBase(const NodeInfo &info, const QString &idSuffix = QString{}) override;
    void generateNodeEnd(const NodeInfo &info);
    bool generateDefsNode(const StructureNodeInfo &info) override;
    void generateDefsInstantiationNode(const StructureNodeInfo &info) override;
    void generateImageNode(const ImageNodeInfo &info) override;
    void generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect) override;
    void generateNode(const NodeInfo &info) override;
    void generateTextNode(const TextNodeInfo &info) override;
    void generateUseNode(const UseNodeInfo &info) override;
    void generateFilterNode(const FilterNodeInfo &info) override;
    bool generateStructureNode(const StructureNodeInfo &info) override;
    bool generateRootNode(const StructureNodeInfo &info) override;
    bool generateMaskNode(const MaskNodeInfo &info) override;
    bool generateMarkerNode(const MarkerNodeInfo &info) override;
    bool generatePatternNode(const PatternNodeInfo &info) override;
    void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect) override;

private:
    enum class AnimationType {
        Auto = 0,
        ColorOpacity = 1
    };

    QString generateNodeId(const NodeInfo &info);
    void generateGradient(const QGradient *grad,
                          const QString &propertyName,
                          const QRectF &coordinateConversion = QRectF(0.0, 0.0, 1.0, 1.0));
    void generateTransform(const QTransform &xf);
    void generatePathContainer(const StructureNodeInfo &info);
    void generateAnimateTransform(const QString &targetName, const NodeInfo &info);
    void generateAnimateMotionPath(const QString &targetName,
                                   const QQuickAnimatedProperty &property);
    void generateAnimationBindings();
    void generateItemAnimations(const QString &idString, const NodeInfo &nodeInfo);
    void generateEasing(const QQuickAnimatedProperty::PropertyAnimation &animation, int time,
                        int streamFlags = 0);
    void generateAnimatedPropertySetter(const QString &targetName,
                                        const QString &propertyName,
                                        const QVariant &value,
                                        const QQuickAnimatedProperty::PropertyAnimation &animation,
                                        int time,
                                        int frameTime,
                                        AnimationType animationType = AnimationType::Auto);
    void generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                   const QString &targetName,
                                   const QString &propertyName,
                                   AnimationType animationType = AnimationType::Auto);

    void generateShaderUse(const NodeInfo &info);
    void generateMarkers(const PathNodeInfo &info);
    qsizetype generateFilterStep(const FilterNodeInfo &info, qsizetype stepIndex);

    bool usingTimelineAnimation() const
    {
        return m_flags.testFlag(QQuickVectorImageGenerator::TimelineAnimation);
    }
    void generateTimelineFields(const StructureNodeInfo &info);
    void generateTimelinePropertySetter(const QString &targetName,
                                        const QString &propertyName,
                                        const QQuickAnimatedProperty::PropertyAnimation &animation,
                                        std::function<QVariant(const QVariant &)> const& extractValue,
                                        int valueIndex = 0);
    void generateTransformTimeline(const QString &targetName, const NodeInfo &info);
    void generatePropertyTimeline(const QQuickAnimatedProperty &property,
                                  const QString &targetName,
                                  const QString &propertyName,
                                  AnimationType animationType = AnimationType::Auto);

    QStringView indent();
    enum StreamFlags { NoFlags = 0x0, SameLine = 0x1 };
    QTextStream &stream(int flags = NoFlags);
    const char *shapeName() const;

protected:
    QBuffer m_result;

    void startDefsSuffixBlock();
    void endDefsSuffixBlock();

private:
    int m_indentLevel = 0;
    int m_textNodeCounter = 0;
    QStack<int> m_oldIndentLevels;
    QTextStream m_stream;
    QString outputFileName;
    int m_inShapeItemLevel = 0;
    QByteArray m_shapeTypeName;
    QString m_commentString;
    bool m_retainFilePaths = false;
    QString m_assetFileDirectory;
    QString m_assetFilePrefix;
    QString m_urlPrefix;
    QString m_topLevelIdString;
    QStringList m_extraImports;
    QMap<std::array<qreal, 4>, QString> m_easings;
    quint32 m_nodeCounter = 0;
    QString m_defsSuffix;
    QString m_indentString;
    QSet<QString> m_contentRelativeMasks;
};

QT_END_NAMESPACE

#endif // QQUICKQMLGENERATOR_P_H
