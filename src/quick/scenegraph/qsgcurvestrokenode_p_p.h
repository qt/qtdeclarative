// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVESTROKENODE_P_P_H
#define QSGCURVESTROKENODE_P_P_H

#include <QtQuick/qtquickexports.h>
#include <QtQuick/qsgmaterial.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QSGCurveStrokeNode;
class QSGCurveStrokeMaterial;

class Q_QUICK_EXPORT QSGCurveStrokeMaterialShader : public QSGMaterialShader
{
public:
    enum class Variant { // flags
        Default = 0,
        Expanding = 0x01,
        Derivatives = 0x02
    };

    QSGCurveStrokeMaterialShader(int variant, int viewCount)
    {
        static constexpr auto baseName = u":/qt-project.org/scenegraph/shaders_ng/shapestroke"_sv;
        setShaderFileName(VertexStage, baseName +
                                  (variant & int(Variant::Expanding) ? u"_expanding"_sv : u""_sv)
                                  + u".vert.qsb"_sv, viewCount);
        setShaderFileName(FragmentStage, baseName +
                                  (variant & int(Variant::Derivatives) ? u"_derivatives"_sv : u""_sv)
                                  + u".frag.qsb"_sv, viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};


class Q_QUICK_EXPORT QSGCurveStrokeMaterial : public QSGMaterial
{
public:
    QSGCurveStrokeMaterial(QSGCurveStrokeNode *node, bool strokeExpanding = false)
        : m_node(node), m_strokeExpanding(strokeExpanding)
    {
        setFlag(Blending, true);
    }

    int compare(const QSGMaterial *other) const override;

    QSGCurveStrokeNode *node() const
    {
        return m_node;
    }

protected:
    QSGMaterialType *type() const override
    {
        static QSGMaterialType legacyType;
        static QSGMaterialType strokeExpandingType;
        return m_strokeExpanding ? &strokeExpandingType : &legacyType;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QSGCurveStrokeNode *m_node;
    bool m_strokeExpanding = false;
};

QT_END_NAMESPACE

#endif // QSGCURVESTROKENODE_P_P_H
