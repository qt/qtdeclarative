// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickqmlgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include "utils_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQuickVectorImage)

class GeneratorStream : public QTextStream
{
public:
    GeneratorStream() = default;
    explicit GeneratorStream(QTextStream *stream)
        : QTextStream(&m_output, QIODevice::ReadWrite), m_stream(stream)
    {}
    ~GeneratorStream()
    {
        flush();
        if (m_stream && !m_output.isEmpty())
            *m_stream << m_output << Qt::endl;
    }

    GeneratorStream(GeneratorStream &other) = delete;
    GeneratorStream &operator=(const GeneratorStream &other) = delete;
    GeneratorStream(GeneratorStream &&other) noexcept
        : m_stream(std::exchange(other.m_stream, nullptr)), m_output(std::move(other.m_output))
    {}
    GeneratorStream &operator=(GeneratorStream &&other) noexcept
    {
        std::swap(m_stream, other.m_stream);
        std::swap(m_output, other.m_output);
        return *this;
    }

private:
    QTextStream *m_stream = nullptr;
    QByteArray m_output;
};

QQuickQmlGenerator::QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName)
    : QQuickGenerator(fileName, flags)
    , outputFileName(outFileName)
{
    m_stream = new QTextStream(&result);
}

QQuickQmlGenerator::~QQuickQmlGenerator()
{
    if (!outputFileName.isEmpty()) {
        QFile outFile(outputFileName);
        outFile.open(QIODevice::WriteOnly);
        outFile.write(result);
        outFile.close();
    }

    if (lcQuickVectorImage().isDebugEnabled()) {
        result.truncate(300);
        qCDebug(lcQuickVectorImage).noquote() << result;
    }
}

void QQuickQmlGenerator::setShapeTypeName(const QString &name)
{
    m_shapeTypeName = name.toLatin1();
}

QString QQuickQmlGenerator::shapeTypeName() const
{
    return QString::fromLatin1(m_shapeTypeName);
}

void QQuickQmlGenerator::setCommentString(const QString commentString)
{
    m_commentString = commentString;
}

QString QQuickQmlGenerator::commentString() const
{
    return m_commentString;
}

void QQuickQmlGenerator::generateNodeBase(const NodeInfo &info)
{
    m_indentLevel++;
    if (!info.nodeId.isEmpty())
        stream() << "objectName: \"" << info.nodeId << "\"";
    if (!info.isDefaultTransform) {
        auto sx = info.transform.m11();
        auto sy = info.transform.m22();
        auto x = info.transform.m31();
        auto y = info.transform.m32();
        if (info.transform.type() == QTransform::TxTranslate) {
            stream() << "transform: Translate { " << "x: " << x << "; y: " << y << " }";
        } else if (info.transform.type() == QTransform::TxScale && !x && !y) {
            stream() << "transform: Scale { xScale: " << sx << "; yScale: " << sy << " }";
        } else {
            const QMatrix4x4 m(info.transform);
            {
                stream() << "transform: [ Matrix4x4 { matrix: Qt.matrix4x4 (";
                m_indentLevel += 3;
                const auto *data = m.data();
                for (int i = 0; i < 4; i++) {
                    stream() << data[i] << ", " << data[i+4] << ", " << data[i+8] << ", " << data[i+12] << ", ";
                }
                stream() << ")  } ]";
                m_indentLevel -= 3;
            }
        }
    }
    if (!info.isDefaultOpacity) {
        stream() << "opacity: " << info.opacity;
    }
    m_indentLevel--;
}

bool QQuickQmlGenerator::generateDefsNode(const NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickQmlGenerator::generateImageNode(const ImageNodeInfo &info)
{
    QString fn = info.image.hasAlphaChannel() ? QStringLiteral("svg_asset_%1.png").arg(info.image.cacheKey())
                                              : QStringLiteral("svg_asset_%1.jpg").arg(info.image.cacheKey());
    // For now we just create a copy of the image in the current directory
    info.image.save(fn);
    qCDebug(lcQuickVectorImage) << "Saving copy of IMAGE" << fn;

    // TODO: this requires proper asset management.
    stream() << "Image {";
    m_indentLevel++;

    generateNodeBase(info);
    stream() << "x: " << info.rect.x();
    stream() << "y: " << info.rect.y();
    stream() << "width: " << info.rect.width();
    stream() << "height: " << info.rect.height();
    stream() << "source: \"" << fn <<"\"";

    m_indentLevel--;

    stream() << "}";
}

void QQuickQmlGenerator::generatePath(const PathNodeInfo &info)
{
    if (m_inShapeItem) {
        if (!info.isDefaultTransform)
            qWarning() << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info);
    } else {
        m_inShapeItem = true;
        stream() << shapeName() << " {";

        // Check ??
        generateNodeBase(info);

        m_indentLevel++;
        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            stream() << "preferredRendererType: Shape.CurveRenderer";
        optimizePaths(info);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_indentLevel--;
        stream() << "}";
        m_inShapeItem = false;
    }
}

void QQuickQmlGenerator::generateGradient(const QGradient *grad, const QRectF &boundingRect)
{
    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);
        stream() << "fillGradient: LinearGradient {";
        m_indentLevel++;

        QRectF gradRect(linGrad->start(), linGrad->finalStop());
        QRectF logRect = linGrad->coordinateMode() == QGradient::LogicalMode ? gradRect : QQuickVectorImageGenerator::Utils::mapToQtLogicalMode(gradRect, boundingRect);

        stream() << "x1: " << logRect.left();
        stream() << "y1: " << logRect.top();
        stream() << "x2: " << logRect.right();
        stream() << "y2: " << logRect.bottom();
        for (auto &stop : linGrad->stops()) {
            stream() << "GradientStop { position: " << stop.first << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        }
        m_indentLevel--;
        stream() << "}";
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        stream() << "fillGradient: RadialGradient {";
        m_indentLevel++;

        stream() << "centerX: " << radGrad->center().x();
        stream() << "centerY: " << radGrad->center().y();
        stream() << "centerRadius: " << radGrad->radius();
        stream() << "focalX:" << radGrad->focalPoint().x();
        stream() << "focalY:" << radGrad->focalPoint().y();
        for (auto &stop : radGrad->stops()) {
            stream() << "GradientStop { position: " << stop.first << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        }
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    QString penName = info.strokeColor;
    const bool noPen = penName.isEmpty() || penName == u"transparent";
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const bool noFill = !info.grad && info.fillColor == u"transparent";
    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    auto fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());
    stream() << "ShapePath {";
    m_indentLevel++;
    if (!info.nodeId.isEmpty()) {
        switch (pathSelector) {
        case QQuickVectorImageGenerator::FillPath:
            stream() << "objectName: \"svg_fill_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::StrokePath:
            stream() << "objectName: \"svg_stroke_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::FillAndStroke:
            stream() << "objectName: \"svg_path:" << info.nodeId << "\"";
            break;
        }
    }

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        stream() << "strokeColor: \"transparent\"";
    } else {
        stream() << "strokeColor: \"" << penName << "\"";
        stream() << "strokeWidth: " << info.strokeWidth;
    }
    if (info.capStyle == Qt::FlatCap)
        stream() << "capStyle: ShapePath.FlatCap"; //### TODO Add the rest of the styles, as well as join styles etc.

    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        stream() << "fillColor: \"transparent\"";
    } else if (auto *grad = info.grad) {
        generateGradient(grad, boundingRect);
    } else {
        stream() << "fillColor: \"" << info.fillColor << "\"";

    }
    if (fillRule == QQuickShapePath::WindingFill)
        stream() << "fillRule: ShapePath.WindingFill";
    else
        stream() << "fillRule: ShapePath.OddEvenFill";

    QString hintStr;
    if (quadPath)
        hintStr = QQuickVectorImageGenerator::Utils::pathHintString(*quadPath);
    if (!hintStr.isEmpty())
        stream() << hintStr;


    QString svgPathString = painterPath ? QQuickVectorImageGenerator::Utils::toSvgString(*painterPath) : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);
    stream() <<   "PathSvg { path: \"" << svgPathString << "\" }";

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateNode(const NodeInfo &info)
{
    stream() << "// Missing Implementation for SVG Node: " << info.typeName;
    stream() << "// Adding an empty Item and skipping";
    stream() << "Item {";
    generateNodeBase(info);
    stream() << "}";
}

void QQuickQmlGenerator::generateTextNode(const TextNodeInfo &info)
{
    static int counter = 0;
    stream() << "Item {";
    generateNodeBase(info);
    m_indentLevel++;

    if (!info.isTextArea)
        stream() << "Item { id: textAlignItem_" << counter << "; x: " << info.position.x() << "; y: " << info.position.y() << "}";

    stream() << "Text {";

    m_indentLevel++;

    if (info.isTextArea) {
        stream() << "x: " << info.position.x();
        stream() << "y: " << info.position.y();
        if (info.size.width() > 0)
            stream() << "width: " << info.size.width();
        if (info.size.height() > 0)
            stream() << "height: " << info.size.height();
        stream() << "wrapMode: Text.Wrap"; // ### WordWrap? verify with SVG standard
        stream() << "clip: true"; //### Not exactly correct: should clip on the text level, not the pixel level
    } else {
        QString hAlign = QStringLiteral("left");
        stream() << "anchors.baseline: textAlignItem_" << counter << ".top";
        switch (info.alignment) {
        case Qt::AlignHCenter:
            hAlign = QStringLiteral("horizontalCenter");
            break;
        case Qt::AlignRight:
            hAlign = QStringLiteral("right");
            break;
        default:
            qCDebug(lcQuickVectorImage) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            break;
        }
        stream() << "anchors." << hAlign << ": textAlignItem_" << counter << ".left";
    }
    counter++;

    stream() << "color: \"" << info.color << "\"";
    stream() << "textFormat:" << (info.needsRichText ? "Text.RichText" : "Text.StyledText");

    QString s = info.text;
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    stream() << "text: \"" << s << "\"";
    stream() << "font.family: \"" << info.font.family() << "\"";
    if (info.font.pixelSize() > 0)
        stream() << "font.pixelSize:" << info.font.pixelSize();
    else if (info.font.pointSize() > 0)
        stream() << "font.pixelSize:" << info.font.pointSizeF();
    if (info.font.underline())
        stream() << "font.underline: true";
    if (info.font.weight() != QFont::Normal)
        stream() << "font.weight: " << int(info.font.weight());
    if (info.font.italic())
        stream() << "font.italic: true";

    if (!info.strokeColor.isEmpty()) {
        stream() << "styleColor: \"" << info.strokeColor << "\"";
        stream() << "style: Text.Outline";
    }

    m_indentLevel--;
    stream() << "}";

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (info.stage == StructureNodeStage::Start) {
        stream() << "Item {";
        generateNodeBase(info);
        m_indentLevel++;
        stream() << "x: " << info.startPos.x();
        stream() << "y: " << info.startPos.y();
    } else {
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (info.stage == StructureNodeStage::Start) {
        if (!info.forceSeparatePaths && info.isPathContainer) {
            stream() << shapeName() <<" {";
            m_indentLevel++;
            if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
                stream() << "preferredRendererType: Shape.CurveRenderer";
            m_indentLevel--;

            m_inShapeItem = true;
        } else {
            stream() << "Item {";
        }

        if (!info.viewBox.isEmpty()) {
            m_indentLevel++;
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";
            m_indentLevel--;
        }

        generateNodeBase(info);
        m_indentLevel++;
    } else {
        m_indentLevel--;
        stream() << "}";
        m_inShapeItem = false;
    }
}

void QQuickQmlGenerator::generateRootNode(const StructureNodeInfo &info)
{
    m_indentLevel = 0;
    if (info.stage == StructureNodeStage::Start) {
        const QStringList comments = m_commentString.split(u'\n');
        if (comments.isEmpty())
            stream() << "// Generated from SVG";
        else
            for (const auto &comment : comments)
                stream() << "// " << comment;

        stream() << "import QtQuick";
        stream() << "import QtQuick.Shapes" << Qt::endl;

        stream() << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

        if (!info.viewBox.isEmpty()) {
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";;
        }

        generateNodeBase(info);
    } else {
        stream() << "}";
        m_inShapeItem = false;
    }
}

QString QQuickQmlGenerator::indent()
{
    return QString().fill(QLatin1Char(' '), m_indentLevel * 4);
}

GeneratorStream QQuickQmlGenerator::stream()
{
    GeneratorStream strm(m_stream);
    strm << indent();
    return strm;
}

const char *QQuickQmlGenerator::shapeName() const
{
    return m_shapeTypeName.isEmpty() ? "Shape" : m_shapeTypeName.constData();
}

QT_END_NAMESPACE
