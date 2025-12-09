// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgvisitorimpl_p.h"
#include "qquickgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsvgvisitor_p.h>

#include <QString>
#include <QPainter>
#include <QTextDocument>
#include <QTextLayout>
#include <QMatrix4x4>
#include <QQuickItem>

#include <private/qquickshape_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickitem_p.h>

#include <private/qquickimagebase_p_p.h>
#include <private/qquickimage_p.h>
#include <private/qsgcurveprocessor_p.h>

#include <private/qquadpath_p.h>

#include <QtCore/private/qstringiterator_p.h>

#include "utils_p.h"
#include <QtCore/qloggingcategory.h>

#include <QtSvg/private/qsvgstyle_p.h>
#include <QtSvg/private/qsvgfilter_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcVectorImageAnimations, "qt.quick.vectorimage.animations")

using namespace Qt::StringLiterals;

class QSvgStyleResolver
{
public:
    QSvgStyleResolver()
    {
        m_dummyImage = QImage(1, 1, QImage::Format_RGB32);
        m_dummyPainter.begin(&m_dummyImage);
        QPen defaultPen(Qt::NoBrush, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
        defaultPen.setMiterLimit(4);
        m_dummyPainter.setPen(defaultPen);
        m_dummyPainter.setBrush(Qt::black);
    }

    ~QSvgStyleResolver()
    {
        m_dummyPainter.end();
    }

    QPainter& painter() { return m_dummyPainter; }
    QSvgExtraStates& states() { return m_svgState; }

    QColor currentFillColor() const
    {
        if (m_dummyPainter.brush().style() == Qt::NoBrush ||
            m_dummyPainter.brush().color() == QColorConstants::Transparent) {
            return QColor(QColorConstants::Transparent);
        }

        QColor fillColor;
        fillColor = m_dummyPainter.brush().color();
        fillColor.setAlphaF(m_svgState.fillOpacity);

        return fillColor;
    }

    qreal currentFillOpacity() const
    {
        return m_svgState.fillOpacity;
    }

    const QGradient *currentStrokeGradient() const
    {
        QBrush brush = m_dummyPainter.pen().brush();
        if (brush.style() == Qt::LinearGradientPattern
                || brush.style() == Qt::RadialGradientPattern
                || brush.style() == Qt::ConicalGradientPattern) {
            return brush.gradient();
        }
        return nullptr;
    }

    const QGradient *currentFillGradient() const
    {
        if (m_dummyPainter.brush().style() == Qt::LinearGradientPattern || m_dummyPainter.brush().style() == Qt::RadialGradientPattern || m_dummyPainter.brush().style() == Qt::ConicalGradientPattern )
            return m_dummyPainter.brush().gradient();
        return nullptr;
    }

    QTransform currentFillTransform() const
    {
        return m_dummyPainter.brush().transform();
    }

    QColor currentStrokeColor() const
    {
        if (m_dummyPainter.pen().brush().style() == Qt::NoBrush ||
            m_dummyPainter.pen().brush().color() == QColorConstants::Transparent) {
            return QColor(QColorConstants::Transparent);
        }

        QColor strokeColor;
        strokeColor = m_dummyPainter.pen().brush().color();
        strokeColor.setAlphaF(m_svgState.strokeOpacity);

        return strokeColor;
    }

    static QGradient applyOpacityToGradient(const QGradient &gradient, float opacity)
    {
        QGradient grad = gradient;
        QGradientStops stops;
        for (auto &stop : grad.stops()) {
            stop.second.setAlphaF(stop.second.alphaF() * opacity);
            stops.append(stop);
        }

        grad.setStops(stops);

        return grad;
    }

    float currentStrokeWidth() const
    {
        float penWidth = m_dummyPainter.pen().widthF();
        return penWidth ? penWidth : 1;
    }

    QPen currentStroke() const
    {
        return m_dummyPainter.pen();
    }

protected:
    QPainter m_dummyPainter;
    QImage m_dummyImage;
    QSvgExtraStates m_svgState;
};

Q_GLOBAL_STATIC(QSvgStyleResolver, styleResolver)

namespace {
inline bool isPathContainer(const QSvgStructureNode *node)
{
    bool foundPath = false;
    for (const auto *child : node->renderers()) {
        switch (child->type()) {
            // nodes that shouldn't go inside Shape{}
        case QSvgNode::Switch:
        case QSvgNode::Doc:
        case QSvgNode::Group:
        case QSvgNode::AnimateColor:
        case QSvgNode::AnimateTransform:
        case QSvgNode::Use:
        case QSvgNode::Video:
        case QSvgNode::Image:
        case QSvgNode::Textarea:
        case QSvgNode::Text:
        case QSvgNode::Tspan:
        case QSvgNode::Mask:
            //qCDebug(lcQuickVectorGraphics) << "NOT path container because" << node->typeName() ;
            return false;

            // nodes that could go inside Shape{}
        case QSvgNode::Defs:
        case QSvgNode::Symbol:
            break;

            // nodes that are done as pure ShapePath{}
        case QSvgNode::Rect:
        case QSvgNode::Circle:
        case QSvgNode::Ellipse:
        case QSvgNode::Line:
        case QSvgNode::Path:
        case QSvgNode::Polygon:
        case QSvgNode::Polyline:
        {
            if (child->hasFilter())
                return false;

            if (!child->style().opacity.isDefault())
                return false;

            if (!child->style().transform.isDefault()) {
                //qCDebug(lcQuickVectorGraphics) << "NOT path container because local transform";
                return false;
            }
            const QList<QSvgAbstractAnimation *> animations = child->document()->animator()->animationsForNode(child);
            if (!animations.isEmpty()) {
                //qCDebug(lcQuickVectorGraphics) << "NOT path container because local transform animation";
                return false;
            }
            foundPath = true;
            break;
        }
        default:
            qCDebug(lcQuickVectorImage) << "Unhandled type in switch" << child->type();
            break;
        }
    }
    //qCDebug(lcQuickVectorGraphics) << "Container" << node->nodeId() << node->typeName()  << "is" << foundPath;
    return foundPath;
}

static QString capStyleName(Qt::PenCapStyle style)
{
    QString styleName;

    switch (style) {
    case Qt::SquareCap:
        styleName = QStringLiteral("squarecap");
        break;
    case Qt::FlatCap:
        styleName = QStringLiteral("flatcap");
        break;
    case Qt::RoundCap:
        styleName = QStringLiteral("roundcap");
        break;
    default:
        break;
    }

    return styleName;
}

static QString joinStyleName(Qt::PenJoinStyle style)
{
    QString styleName;

    switch (style) {
    case Qt::MiterJoin:
        styleName = QStringLiteral("miterjoin");
        break;
    case Qt::BevelJoin:
        styleName = QStringLiteral("beveljoin");
        break;
    case Qt::RoundJoin:
        styleName = QStringLiteral("roundjoin");
        break;
    case Qt::SvgMiterJoin:
        styleName = QStringLiteral("svgmiterjoin");
        break;
    default:
        break;
    }

    return styleName;
}

static QString dashArrayString(QList<qreal> dashArray)
{
    if (dashArray.isEmpty())
        return QString();

    QString dashArrayString;
    QTextStream stream(&dashArrayString);

    for (int i = 0; i < dashArray.length() - 1; i++) {
        qreal value = dashArray[i];
        stream << value << ", ";
    }

    stream << dashArray.last();

    return dashArrayString;
}
};

QSvgVisitorImpl::QSvgVisitorImpl(const QString svgFileName,
                                 QQuickGenerator *generator,
                                 bool assumeTrustedSource)
    : m_svgFileName(svgFileName)
    , m_generator(generator)
    , m_assumeTrustedSource(assumeTrustedSource)
{
}

bool QSvgVisitorImpl::traverse()
{
    if (!m_generator) {
        qCDebug(lcQuickVectorImage) << "No valid QQuickGenerator is set. Genration will stop";
        return false;
    }

    QtSvg::Options options;
    if (m_assumeTrustedSource)
        options.setFlag(QtSvg::AssumeTrustedSource);

    auto *doc = QSvgDocument::load(m_svgFileName, options);
    if (!doc) {
        qCDebug(lcQuickVectorImage) << "Not a valid Svg File : " << m_svgFileName;
        return false;
    }

    QSvgVisitor::traverse(doc);
    return true;
}

void QSvgVisitorImpl::visitNode(const QSvgNode *node)
{
    if (m_defsLevel > 0)
        return;

    handleBaseNodeSetup(node);

    NodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    m_generator->generateNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitImageNode(const QSvgImage *node)
{
    if (m_defsLevel > 0)
        return;

    // TODO: this requires proper asset management.
    handleBaseNodeSetup(node);

    ImageNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);
    info.image = node->image();
    info.rect = node->rect();
    info.externalFileReference = node->filename();

    m_generator->generateImageNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitRectNode(const QSvgRect *node)
{
    if (m_defsLevel > 0)
        return;

    QRectF rect = node->rect();
    QPointF rads = node->radius();
    // This is using Qt::RelativeSize semantics: percentage of half rect size
    qreal x1 = rect.left();
    qreal x2 = rect.right();
    qreal y1 = rect.top();
    qreal y2 = rect.bottom();

    qreal rx =  rads.x() * rect.width() / 200;
    qreal ry = rads.y() * rect.height() / 200;
    QPainterPath p;

    p.moveTo(x1 + rx, y1);
    p.lineTo(x2 - rx, y1);
    // qCDebug(lcQuickVectorGraphics) << "Line1" << x2 - rx << y1;
    p.arcTo(x2 - rx * 2, y1, rx * 2, ry * 2, 90, -90); // ARC to x2, y1 + ry
    // qCDebug(lcQuickVectorGraphics) << "p1" << p;

    p.lineTo(x2, y2 - ry);
    p.arcTo(x2 - rx * 2, y2 - ry * 2, rx * 2, ry * 2, 0, -90); // ARC to x2 - rx, y2

    p.lineTo(x1 + rx, y2);
    p.arcTo(x1, y2 - ry * 2, rx * 2, ry * 2, 270, -90); // ARC to x1, y2 - ry

    p.lineTo(x1, y1 + ry);
    p.arcTo(x1, y1, rx * 2, ry * 2, 180, -90); // ARC to x1 + rx, y1

    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitEllipseNode(const QSvgEllipse *node)
{
    if (m_defsLevel > 0)
        return;

    QRectF rect = node->rect();

    QPainterPath p;
    p.addEllipse(rect);

    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPathNode(const QSvgPath *node)
{
    if (m_defsLevel > 0)
        return;

    handlePathNode(node, node->path());
}

void QSvgVisitorImpl::visitLineNode(const QSvgLine *node)
{
    if (m_defsLevel > 0)
        return;

    QPainterPath p;
    p.moveTo(node->line().p1());
    p.lineTo(node->line().p2());
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolygonNode(const QSvgPolygon *node)
{
    if (m_defsLevel > 0)
        return;

    QPainterPath p = QQuickVectorImageGenerator::Utils::polygonToPath(node->polygon(), true);
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolylineNode(const QSvgPolyline *node)
{
    if (m_defsLevel > 0)
        return;

    QPainterPath p = QQuickVectorImageGenerator::Utils::polygonToPath(node->polygon(), false);
    handlePathNode(node, p);
}

QString QSvgVisitorImpl::gradientCssDescription(const QGradient *gradient)
{
    QString cssDescription;
    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *linearGradient = static_cast<const QLinearGradient *>(gradient);

        cssDescription += " -qt-foreground: qlineargradient("_L1;
        cssDescription += "x1:"_L1 + QString::number(linearGradient->start().x()) + u',';
        cssDescription += "y1:"_L1 + QString::number(linearGradient->start().y()) + u',';
        cssDescription += "x2:"_L1 + QString::number(linearGradient->finalStop().x()) + u',';
        cssDescription += "y2:"_L1 + QString::number(linearGradient->finalStop().y()) + u',';
    } else if (gradient->type() == QGradient::RadialGradient) {
        const QRadialGradient *radialGradient = static_cast<const QRadialGradient *>(gradient);

        cssDescription += " -qt-foreground: qradialgradient("_L1;
        cssDescription += "cx:"_L1 + QString::number(radialGradient->center().x()) + u',';
        cssDescription += "cy:"_L1 + QString::number(radialGradient->center().y()) + u',';
        cssDescription += "fx:"_L1 + QString::number(radialGradient->focalPoint().x()) + u',';
        cssDescription += "fy:"_L1 + QString::number(radialGradient->focalPoint().y()) + u',';
        cssDescription += "radius:"_L1 + QString::number(radialGradient->radius()) + u',';
    } else {
        const QConicalGradient *conicalGradient = static_cast<const QConicalGradient *>(gradient);

        cssDescription += " -qt-foreground: qconicalgradient("_L1;
        cssDescription += "cx:"_L1 + QString::number(conicalGradient->center().x()) + u',';
        cssDescription += "cy:"_L1 + QString::number(conicalGradient->center().y()) + u',';
        cssDescription += "angle:"_L1 + QString::number(conicalGradient->angle()) + u',';
    }

    const QStringList coordinateModes = { "logical"_L1, "stretchtodevice"_L1, "objectbounding"_L1, "object"_L1 };
    cssDescription += "coordinatemode:"_L1;
    cssDescription += coordinateModes.at(int(gradient->coordinateMode()));
    cssDescription += u',';

    const QStringList spreads = { "pad"_L1, "reflect"_L1, "repeat"_L1 };
    cssDescription += "spread:"_L1;
    cssDescription += spreads.at(int(gradient->spread()));

    for (const QGradientStop &stop : gradient->stops()) {
        cssDescription += ",stop:"_L1;
        cssDescription += QString::number(stop.first);
        cssDescription += u' ';
        cssDescription += stop.second.name(QColor::HexArgb);
    }

    cssDescription += ");"_L1;

    return cssDescription;
}

QString QSvgVisitorImpl::colorCssDescription(QColor color)
{
    QString cssDescription;
    cssDescription += QStringLiteral("rgba(");
    cssDescription += QString::number(color.red()) + QStringLiteral(",");
    cssDescription += QString::number(color.green()) + QStringLiteral(",");
    cssDescription += QString::number(color.blue()) + QStringLiteral(",");
    cssDescription += QString::number(color.alphaF()) + QStringLiteral(")");

    return cssDescription;
}

namespace {

    // Simple class for representing the SVG font as a font engine
    // We use the Proxy font engine type, which is currently unused and does not map to
    // any specific font engine
    // (The QSvgFont object must outlive the engine.)
    class QSvgFontEngine : public QFontEngine
    {
    public:
        QSvgFontEngine(const QSvgFont *font, qreal size);

        QFontEngine *cloneWithSize(qreal size) const override;

        glyph_t glyphIndex(uint ucs4) const override;
        int stringToCMap(const QChar *str,
                         int len,
                         QGlyphLayout *glyphs,
                         int *nglyphs,
                         ShaperFlags flags) const override;

        void addGlyphsToPath(glyph_t *glyphs,
                             QFixedPoint *positions,
                             int nGlyphs,
                             QPainterPath *path,
                             QTextItem::RenderFlags flags) override;

        glyph_metrics_t boundingBox(glyph_t glyph) override;

        void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;
        QFixed ascent() const override;
        QFixed capHeight() const override;
        QFixed descent() const override;
        QFixed leading() const override;
        qreal maxCharWidth() const override;
        qreal minLeftBearing() const override;
        qreal minRightBearing() const override;

        QFixed emSquareSize() const override;

    private:
        const QSvgFont *m_font;
    };

    QSvgFontEngine::QSvgFontEngine(const QSvgFont *font, qreal size)
        : QFontEngine(Proxy)
        , m_font(font)
    {
        fontDef.pixelSize = size;
        fontDef.families = QStringList(m_font->m_familyName);
    }

    QFixed QSvgFontEngine::emSquareSize() const
    {
        return QFixed::fromReal(m_font->m_unitsPerEm);
    }

    glyph_t QSvgFontEngine::glyphIndex(uint ucs4) const
    {
        const ushort c(ucs4);
        if (ucs4 < USHRT_MAX && m_font->findFirstGlyphFor(QStringView(&c, 1)))
            return glyph_t(ucs4);

        return 0;
    }

    int QSvgFontEngine::stringToCMap(const QChar *str,
                                     int len,
                                     QGlyphLayout *glyphs,
                                     int *nglyphs,
                                     ShaperFlags flags) const
    {
        Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
        if (*nglyphs < len) {
            *nglyphs = len;
            return -1;
        }

        int ucs4Length = 0;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            char32_t ucs4 = it.next();
            glyph_t index = glyphIndex(ucs4);
            glyphs->glyphs[ucs4Length++] = index;
        }

        *nglyphs = ucs4Length;
        glyphs->numGlyphs = ucs4Length;

        if (!(flags & GlyphIndicesOnly))
            recalcAdvances(glyphs, flags);

        return *nglyphs;
    }

    void QSvgFontEngine::addGlyphsToPath(glyph_t *glyphs,
                                         QFixedPoint *positions,
                                         int nGlyphs,
                                         QPainterPath *path,
                                         QTextItem::RenderFlags flags)
    {
        Q_UNUSED(flags);
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        for (int i = 0; i < nGlyphs; ++i) {
            glyph_t index = glyphs[i];
            if (index > 0) {
                QPointF position = positions[i].toPointF();
                const ushort c(index);
                const QSvgGlyph *foundGlyph = m_font->findFirstGlyphFor(QStringView(&c, 1));

                if (!foundGlyph)
                    continue;

                QPainterPath glyphPath = foundGlyph->m_path;

                QTransform xform;
                xform.translate(position.x(), position.y());
                xform.scale(scale, -scale);
                glyphPath = xform.map(glyphPath);
                path->addPath(glyphPath);
            }
        }
    }

    glyph_metrics_t QSvgFontEngine::boundingBox(glyph_t glyph)
    {
        glyph_metrics_t ret;
        ret.x = 0; // left bearing
        ret.y = -ascent();
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        const ushort c(glyph);
        const QSvgGlyph *svgGlyph = m_font->findFirstGlyphFor(QStringView(&c, 1));
        ret.width = QFixed::fromReal(svgGlyph ? svgGlyph->m_horizAdvX * scale : 0.);
        ret.height = ascent() + descent();
        return ret;
    }

    QFontEngine *QSvgFontEngine::cloneWithSize(qreal size) const
    {
        QSvgFontEngine *otherEngine = new QSvgFontEngine(m_font, size);
        return otherEngine;
    }

    void QSvgFontEngine::recalcAdvances(QGlyphLayout *glyphLayout, ShaperFlags) const
    {
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        for (int i = 0; i < glyphLayout->numGlyphs; i++) {
            const ushort c(glyphLayout->glyphs[i]);
            const QSvgGlyph *svgGl = m_font->findFirstGlyphFor(QStringView(&c, 1));
            glyphLayout->advances[i] = QFixed::fromReal(svgGl ? svgGl->m_horizAdvX * scale : 0.);
        }
    }

    QFixed QSvgFontEngine::ascent() const
    {
        return QFixed::fromReal(fontDef.pixelSize);
    }

    QFixed QSvgFontEngine::capHeight() const
    {
        return ascent();
    }
    QFixed QSvgFontEngine::descent() const
    {
        return QFixed{};
    }

    QFixed QSvgFontEngine::leading() const
    {
        return QFixed{};
    }

    qreal QSvgFontEngine::maxCharWidth() const
    {
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        return m_font->m_horizAdvX * scale;
    }

    qreal QSvgFontEngine::minLeftBearing() const
    {
        return 0.0;
    }

    qreal QSvgFontEngine::minRightBearing() const
    {
        return 0.0;
    }
}

static QVariant calculateInterpolatedValue(const QSvgAbstractAnimatedProperty *property, int index, int)
{
    if (index == 0)
        const_cast<QSvgAbstractAnimatedProperty *>(property)->interpolate(1, 0.0);
    else
        const_cast<QSvgAbstractAnimatedProperty *>(property)->interpolate(index, 1.0);

    return property->interpolatedValue();
}

void QSvgVisitorImpl::visitTextNode(const QSvgText *node)
{
    if (m_defsLevel > 0)
        return;

    handleBaseNodeSetup(node);
    const bool isTextArea = node->type() == QSvgNode::Textarea;

    QString text;
    const QSvgFont *svgFont = styleResolver->states().svgFont;
    bool needsRichText = false;
    bool preserveWhiteSpace = node->whitespaceMode() == QSvgText::Preserve;
    const QGradient *mainGradient = styleResolver->currentFillGradient();

    QFontEngine *fontEngine = nullptr;
    if (svgFont != nullptr) {
        fontEngine = new QSvgFontEngine(svgFont, styleResolver->painter().font().pointSize());
        fontEngine->ref.ref();
    }

#if QT_CONFIG(texthtmlparser)
    bool needsPathNode = mainGradient != nullptr
                           || svgFont != nullptr
                           || styleResolver->currentStrokeGradient() != nullptr;
#endif
    for (const auto *tspan : node->tspans()) {
        if (!tspan) {
            text += QStringLiteral("<br>");
            continue;
        }

        // Note: We cannot get the font directly from the style, since this does
        // not apply the weight, since this is relative and depends on current state.
        handleBaseNodeSetup(tspan);
        QFont font = styleResolver->painter().font();

        QString styleTagContent;

        if ((font.resolveMask() & QFont::FamilyResolved)
            || (font.resolveMask() & QFont::FamiliesResolved)) {
            styleTagContent += QStringLiteral("font-family: %1;").arg(font.family());
        }

        if (font.resolveMask() & QFont::WeightResolved
            && font.weight() != QFont::Normal
            && font.weight() != QFont::Bold) {
            styleTagContent += QStringLiteral("font-weight: %1;").arg(int(font.weight()));
        }

        if (font.resolveMask() & QFont::SizeResolved) {
            // Pixel size stored as point size in SVG parser
            styleTagContent += QStringLiteral("font-size: %1px;").arg(int(font.pointSizeF()));
        }

        if (font.resolveMask() & QFont::CapitalizationResolved
            && font.capitalization() == QFont::SmallCaps) {
            styleTagContent += QStringLiteral("font-variant: small-caps;");
        }

        if (styleResolver->currentFillGradient() != nullptr
            && styleResolver->currentFillGradient() != mainGradient) {
            const QGradient grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());
            styleTagContent += gradientCssDescription(&grad) + u';';
#if QT_CONFIG(texthtmlparser)
            needsPathNode = true;
#endif
        }

        const QColor currentStrokeColor = styleResolver->currentStrokeColor();
        if (currentStrokeColor.alpha() > 0) {
            QString strokeColor = colorCssDescription(currentStrokeColor);
            styleTagContent += QStringLiteral("-qt-stroke-color:%1;").arg(strokeColor);
            styleTagContent += QStringLiteral("-qt-stroke-width:%1px;").arg(styleResolver->currentStrokeWidth());
            styleTagContent += QStringLiteral("-qt-stroke-dasharray:%1;").arg(dashArrayString(styleResolver->currentStroke().dashPattern()));
            styleTagContent += QStringLiteral("-qt-stroke-dashoffset:%1;").arg(styleResolver->currentStroke().dashOffset());
            styleTagContent += QStringLiteral("-qt-stroke-lineCap:%1;").arg(capStyleName(styleResolver->currentStroke().capStyle()));
            styleTagContent += QStringLiteral("-qt-stroke-lineJoin:%1;").arg(joinStyleName(styleResolver->currentStroke().joinStyle()));
            if (styleResolver->currentStroke().joinStyle() == Qt::MiterJoin || styleResolver->currentStroke().joinStyle() == Qt::SvgMiterJoin)
                styleTagContent += QStringLiteral("-qt-stroke-miterlimit:%1;").arg(styleResolver->currentStroke().miterLimit());
#if QT_CONFIG(texthtmlparser)
            needsPathNode = true;
#endif
        }

        if (tspan->whitespaceMode() == QSvgText::Preserve && !preserveWhiteSpace)
            styleTagContent += QStringLiteral("white-space: pre-wrap;");

        QString content = tspan->text().toHtmlEscaped();
        content.replace(QLatin1Char('\t'), QLatin1Char(' '));
        content.replace(QLatin1Char('\n'), QLatin1Char(' '));

        bool fontTag = false;
        if (!tspan->style().fill.isDefault()) {
            auto &b = tspan->style().fill->qbrush();
            qCDebug(lcQuickVectorImage) << "tspan FILL:" << b;
            if (b.style() != Qt::NoBrush)
            {
                if (qFuzzyCompare(b.color().alphaF() + 1.0, 2.0))
                {
                    QString spanColor = b.color().name();
                    fontTag = !spanColor.isEmpty();
                    if (fontTag)
                        text += QStringLiteral("<font color=\"%1\">").arg(spanColor);
                } else {
                    QString spanColor = colorCssDescription(b.color());
                    styleTagContent += QStringLiteral("color:%1").arg(spanColor);
                }
            }
        }

        needsRichText = needsRichText || !styleTagContent.isEmpty();
        if (!styleTagContent.isEmpty())
            text += QStringLiteral("<span style=\"%1\">").arg(styleTagContent.toHtmlEscaped());

        if (font.resolveMask() & QFont::WeightResolved && font.bold())
            text += QStringLiteral("<b>");

        if (font.resolveMask() & QFont::StyleResolved && font.italic())
            text += QStringLiteral("<i>");

        if (font.resolveMask() & QFont::CapitalizationResolved) {
            switch (font.capitalization()) {
            case QFont::AllLowercase:
                content = content.toLower();
                break;
            case QFont::AllUppercase:
                content = content.toUpper();
                break;
            case QFont::Capitalize:
                // ### We need to iterate over the string and do the title case conversion,
                // since this is not part of QString.
                qCWarning(lcQuickVectorImage) << "Title case not implemented for tspan";
                break;
            default:
                break;
            }
        }
        text += content;
        if (fontTag)
            text += QStringLiteral("</font>");

        if (font.resolveMask() & QFont::StyleResolved && font.italic())
            text += QStringLiteral("</i>");

        if (font.resolveMask() & QFont::WeightResolved && font.bold())
            text += QStringLiteral("</b>");

        if (!styleTagContent.isEmpty())
            text += QStringLiteral("</span>");

        handleBaseNodeEnd(tspan);
    }

    if (preserveWhiteSpace && (needsRichText || styleResolver->currentFillGradient() != nullptr))
        text = QStringLiteral("<span style=\"white-space: pre-wrap\">") + text + QStringLiteral("</span>");

    QFont font = styleResolver->painter().font();
    if (font.pixelSize() <= 0 && font.pointSize() > 0)
        font.setPixelSize(font.pointSize()); // Pixel size stored as point size by SVG parser

    font.setHintingPreference(QFont::PreferNoHinting);

#if QT_CONFIG(texthtmlparser)
    if (needsPathNode) {
        QTextDocument document;
        document.setHtml(text);
        if (isTextArea && node->size().width() > 0)
            document.setTextWidth(node->size().width());
        document.setDefaultFont(font);
        document.pageCount(); // Force layout

        QTextBlock block = document.firstBlock();
        while (block.isValid()) {
            QTextLayout *lout = block.layout();

            if (lout != nullptr) {
                QRectF boundingRect = lout->boundingRect();

                // If this block has requested the current SVG font, we override it
                // (note that this limits the text to one svg font, but this is also the case
                // in the QPainter at the moment, and needs a more centralized solution in Qt Svg
                // first)
                QFont blockFont = block.charFormat().font();
                if (svgFont != nullptr
                    && blockFont.family() == svgFont->m_familyName) {
                    QRawFont rawFont;
                    QRawFontPrivate *rawFontD = QRawFontPrivate::get(rawFont);
                    rawFontD->setFontEngine(fontEngine->cloneWithSize(blockFont.pixelSize()));

                    lout->setRawFont(rawFont);
                }

                auto addPathForFormat = [&](QPainterPath p, QTextCharFormat fmt, int pathIndex) {
                    PathNodeInfo info;
                    fillCommonNodeInfo(node, info, QStringLiteral("_path%1").arg(pathIndex));
                    fillPathAnimationInfo(node, info);
                    auto fillStyle = node->style().fill;
                    if (fillStyle)
                        info.fillRule = fillStyle->fillRule();

                    if (fmt.hasProperty(QTextCharFormat::ForegroundBrush)) {
                        info.fillColor.setDefaultValue(fmt.foreground().color());
                        if (fmt.foreground().gradient() != nullptr && fmt.foreground().gradient()->type() != QGradient::NoGradient)
                            info.grad = *fmt.foreground().gradient();
                    } else {
                        info.fillColor.setDefaultValue(styleResolver->currentFillColor());
                    }

                    info.path.setDefaultValue(QVariant::fromValue(p));

                    const QGradient *strokeGradient = styleResolver->currentStrokeGradient();
                    QPen pen;
                    if (fmt.hasProperty(QTextCharFormat::TextOutline)) {
                        pen = fmt.textOutline();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color.setDefaultValue(pen.color());
                        }
                    } else {
                        pen = styleResolver->currentStroke();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color.setDefaultValue(styleResolver->currentStrokeColor());
                        }
                    }

                    if (info.grad.type() == QGradient::NoGradient && styleResolver->currentFillGradient() != nullptr)
                        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());

                    info.fillTransform = styleResolver->currentFillTransform();

                    m_generator->generatePath(info, boundingRect);

                    if (strokeGradient != nullptr) {
                        PathNodeInfo strokeInfo;
                        fillCommonNodeInfo(node, strokeInfo, QStringLiteral("_stroke%1").arg(pathIndex));
                        fillPathAnimationInfo(node, strokeInfo);

                        strokeInfo.grad = *strokeGradient;

                        QPainterPathStroker stroker(pen);
                        strokeInfo.path.setDefaultValue(QVariant::fromValue(stroker.createStroke(p)));
                        m_generator->generatePath(strokeInfo, boundingRect);
                    }
                };

                qreal baselineOffset = -QFontMetricsF(font).ascent();
                if (lout->lineCount() > 0 && lout->lineAt(0).isValid())
                    baselineOffset = -lout->lineAt(0).ascent();

                const QPointF baselineTranslation(0.0, baselineOffset);
                auto glyphsToPath = [&](QList<QGlyphRun> glyphRuns, qreal width) {
                    QList<QPainterPath> paths;
                    for (const QGlyphRun &glyphRun : glyphRuns) {
                        QRawFont font = glyphRun.rawFont();
                        QList<quint32> glyphIndexes = glyphRun.glyphIndexes();
                        QList<QPointF> positions = glyphRun.positions();

                        for (qsizetype j = 0; j < glyphIndexes.size(); ++j) {
                            quint32 glyphIndex = glyphIndexes.at(j);
                            const QPointF &pos = positions.at(j);

                            QPainterPath p = font.pathForGlyph(glyphIndex);
                            p.translate(pos + node->position() + baselineTranslation);
                            if (styleResolver->states().textAnchor == Qt::AlignHCenter)
                                p.translate(QPointF(-0.5 * width, 0));
                            else if (styleResolver->states().textAnchor == Qt::AlignRight)
                                p.translate(QPointF(-width, 0));
                            paths.append(p);
                        }
                    }

                    return paths;
                };

                QList<QTextLayout::FormatRange> formats = block.textFormats();
                for (int i = 0; i < formats.size(); ++i) {
                    QTextLayout::FormatRange range = formats.at(i);

                    QList<QGlyphRun> glyphRuns = lout->glyphRuns(range.start, range.length);
                    QList<QPainterPath> paths = glyphsToPath(glyphRuns, lout->minimumWidth());
                    for (int j = 0; j < paths.size(); ++j) {
                        const QPainterPath &path = paths.at(j);
                        addPathForFormat(path, range.format, j);
                    }
                }
            }

            block = block.next();
        }
    } else
#endif
    {
        TextNodeInfo info;
        fillCommonNodeInfo(node, info);
        fillAnimationInfo(node, info);

        {
            QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("fill"));
            if (!animations.isEmpty())
                applyAnimationsToProperty(animations, &info.fillColor, calculateInterpolatedValue);
        }

        {
            QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("fill-opacity"));
            if (!animations.isEmpty())
                applyAnimationsToProperty(animations, &info.fillOpacity, calculateInterpolatedValue);
        }

        {
            QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("stroke"));
            if (!animations.isEmpty())
                applyAnimationsToProperty(animations, &info.strokeColor, calculateInterpolatedValue);
        }

        {
            QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("stroke-opacity"));
            if (!animations.isEmpty())
                applyAnimationsToProperty(animations, &info.strokeOpacity, calculateInterpolatedValue);
        }

        info.position = node->position();
        info.size = node->size();
        info.font = font;
        info.text = text;
        info.isTextArea = isTextArea;
        info.needsRichText = needsRichText;
        info.fillColor.setDefaultValue(styleResolver->currentFillColor());
        info.alignment = styleResolver->states().textAnchor;
        info.strokeColor.setDefaultValue(styleResolver->currentStrokeColor());

        m_generator->generateTextNode(info);
    }

    handleBaseNodeEnd(node);

    if (fontEngine != nullptr) {
        fontEngine->ref.deref();
        Q_ASSERT(fontEngine->ref.loadRelaxed() == 0);
        delete fontEngine;
    }
}

void QSvgVisitorImpl::visitUseNode(const QSvgUse *node)
{
    if (m_defsLevel > 0)
        return;

    QSvgNode *link = node->link();
    if (!link)
        return;

    handleBaseNodeSetup(node);
    UseNodeInfo info;
    QPointF startPos = node->start();

    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    if (!info.bounds.isNull())
        info.bounds.translate(-startPos);
    info.stage = StructureNodeStage::Start;

    if (!startPos.isNull()) {
        QTransform xform;
        if (!info.isDefaultTransform)
            xform = info.transform.defaultValue().value<QTransform>();
        xform.translate(startPos.x(), startPos.y());
        info.transform.setDefaultValue(QVariant::fromValue(xform));
        info.isDefaultTransform = false;
    }

    m_generator->generateUseNode(info);

    QString oldLinkSuffix = m_linkSuffix;
    m_linkSuffix += QStringLiteral("_use") + info.id;

    m_useLevel++;
    QSvgVisitor::traverse(link);
    m_useLevel--;
    m_linkSuffix = oldLinkSuffix;

    info.stage = StructureNodeStage::End;
    m_generator->generateUseNode(info);
    handleBaseNodeEnd(node);
}

bool QSvgVisitorImpl::visitSwitchNodeStart(const QSvgSwitch *node)
{
    if (m_defsLevel > 0)
        return false;

    QSvgNode *link = node->childToRender();
    if (!link)
        return false;

    QString oldLinkSuffix = m_linkSuffix;
    m_linkSuffix += QStringLiteral("_switch") + QString::number(quintptr(node), 16);
    QSvgVisitor::traverse(link);
    m_linkSuffix = oldLinkSuffix;

    return false;
}

void QSvgVisitorImpl::visitSwitchNodeEnd(const QSvgSwitch *node)
{
    Q_UNUSED(node);
}

bool QSvgVisitorImpl::visitDefsNodeStart(const QSvgDefs *node)
{
    Q_UNUSED(node)

    m_defsLevel++;
    return true;
}

void QSvgVisitorImpl::visitDefsNodeEnd(const QSvgDefs *node)
{
    Q_UNUSED(node)

    m_defsLevel--;
}

bool QSvgVisitorImpl::visitSymbolNodeStart(const QSvgSymbol *node)
{
    if (m_useLevel == 0)
        return false;

    handleBaseNodeSetup(node);

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    QTransform oldTransform = info.transform.defaultValue().value<QTransform>();
    info.clipBox = oldTransform.mapRect(node->clipRect());

    QTransform xform = node->aspectRatioTransform();
    if (!xform.isIdentity()) {
        info.isDefaultTransform = false;
        xform = xform * oldTransform;
        info.transform.setDefaultValue(QVariant::fromValue(xform));
    }
    info.stage = StructureNodeStage::Start;

    return m_generator->generateStructureNode(info);
}

void QSvgVisitorImpl::visitSymbolNodeEnd(const QSvgSymbol *node)
{
    Q_ASSERT(m_useLevel > 0);
    handleBaseNodeSetup(node);

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    info.clipBox = node->clipRect();
    info.stage = StructureNodeStage::End;

    m_generator->generateStructureNode(info);
}

bool QSvgVisitorImpl::visitMaskNodeStart(const QSvgMask *node)
{
    handleBaseNodeSetup(node);

    MaskNodeInfo info;

    QSvgRectF r = node->rect();
    info.isMaskRectRelativeCoordinates = r.unitX() == QtSvg::UnitTypes::objectBoundingBox;
    info.maskRect = r;

    if (node->contentUnits() == QtSvg::UnitTypes::objectBoundingBox)
        qCWarning(lcQuickVectorImage) << "Only user space content units supported for masks";

    fillCommonNodeInfo(node, info);

    return m_generator->generateMaskNode(info);
}

void QSvgVisitorImpl::visitMaskNodeEnd(const QSvgMask *node)
{
    MaskNodeInfo info;
    info.stage = StructureNodeStage::End;

    QSvgRectF r = node->rect();
    info.isMaskRectRelativeCoordinates = r.unitX() == QtSvg::UnitTypes::objectBoundingBox;
    info.maskRect = r;
    fillCommonNodeInfo(node, info);

    m_generator->generateMaskNode(info);

    handleBaseNodeEnd(node);
}

bool QSvgVisitorImpl::visitFilterNodeStart(const QSvgFilterContainer *node)
{
    Q_UNUSED(node)

    if (!m_filterPrimitives.isEmpty()) {
        qCWarning(lcQuickVectorImage) << "Filter defined inside a filter";
        return false;
    }

    return true;
}

void QSvgVisitorImpl::visitFilterNodeEnd(const QSvgFilterContainer *node)
{
    if (m_filterPrimitives.isEmpty())
        return;

    handleBaseNodeSetup(node);

    FilterNodeInfo info;
    fillCommonNodeInfo(node, info);

    info.filterRect = node->rect();
    if (node->filterUnits() == QtSvg::UnitTypes::objectBoundingBox)
        info.csFilterRect = FilterNodeInfo::CoordinateSystem::Relative;

    bool generatedAlpha = false;
    for (const QSvgFeFilterPrimitive *filterPrimitive : std::as_const(m_filterPrimitives)) {
        if (filterPrimitive->requiresSourceAlpha() && !generatedAlpha) {
            FilterNodeInfo::FilterStep alphaStep;
            alphaStep.filterType = FilterNodeInfo::Type::ColorMatrix;
            alphaStep.csFilterParameter = FilterNodeInfo::CoordinateSystem::MatchFilterRect;

            // Isolate alpha
            qreal values[] = { 0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 1.0, 0.0,
                              0.0, 0.0, 0.0, 0.0, 0.0 };
            QGenericMatrix<5, 5, qreal> matrix(values);
            alphaStep.filterParameter = QVariant::fromValue(matrix);
            alphaStep.outputName = info.id + QStringLiteral("_source_alpha");
            generatedAlpha = true;

            info.steps.append(alphaStep);
        }

        fillFilterPrimitiveInfo(node, filterPrimitive, info);
    }

    m_generator->generateFilterNode(info);
    m_filterPrimitives.clear();
}

void QSvgVisitorImpl::fillFilterPrimitiveInfo(const QSvgFilterContainer *node,
                                              const QSvgFeFilterPrimitive *filterPrimitive,
                                              FilterNodeInfo &info)
{
    FilterNodeInfo::FilterStep step;
    step.filterPrimitiveRect = filterPrimitive->rect();

    step.outputName = info.id + QStringLiteral("_")
        + (filterPrimitive->result().isEmpty()
        ? QStringLiteral("output_") + QString::number(info.steps.size())
        : filterPrimitive->result());

    auto findInput = [&info, &filterPrimitive](const QString &input, QString *outName) {
        const QString alphaSource = info.id + QStringLiteral("_source_alpha");
        if (input == QStringLiteral("SourceGraphic")) {
            return FilterNodeInfo::FilterInput::SourceColor;
        } else if (input == QStringLiteral("SourceAlpha")) {
            *outName = alphaSource;
            return FilterNodeInfo::FilterInput::SourceAlpha;
        } else if (info.steps.isEmpty()) {
            return FilterNodeInfo::FilterInput::SourceColor;
        }

        if (!input.isEmpty()) {
            *outName = info.id + QStringLiteral("_") + input;
        } else {
            bool insideMergeNode = filterPrimitive->type() == QSvgNode::FeMergenode;
            for (int i = info.steps.size() - 1; i >= 0; --i) {
                const auto &prevStep = info.steps.at(i);
                if (insideMergeNode && prevStep.filterType == FilterNodeInfo::Type::Merge) {
                    insideMergeNode = false;
                    continue;
                }

                if (!prevStep.outputName.isEmpty() && prevStep.outputName != alphaSource) {
                    *outName = prevStep.outputName;
                    break;
                }
            }
        }

        return FilterNodeInfo::FilterInput::Name;
    };

    step.input1 = findInput(filterPrimitive->input(), &step.namedInput1);

    if (node->primitiveUnits() == QtSvg::UnitTypes::objectBoundingBox)
        step.csFilterParameter = FilterNodeInfo::CoordinateSystem::Relative;

    // We special-case the default filter primitive rect as is done in Qt Svg.
    // The default is to match the filter's rect and this is represented by making
    // the types of the filter primitive's rect QtSvg::UnitTypes::unknown. Since
    // this is not generally handled in Qt Svg, we also just special case it here.
    if (node->primitiveUnits() == QtSvg::UnitTypes::userSpaceOnUse
        && filterPrimitive->rect().unitW() == QtSvg::UnitTypes::unknown) {
        step.csFilterParameter = FilterNodeInfo::CoordinateSystem::MatchFilterRect;
    }

    switch (filterPrimitive->type()) {
    case QSvgNode::FeMerge:
        step.filterType = FilterNodeInfo::Type::Merge;
        break;
    case QSvgNode::FeMergenode:
        step.filterType = FilterNodeInfo::Type::MergeNode;
        break;
    case QSvgNode::FeBlend:
    {
        const QSvgFeBlend *blend = static_cast<const QSvgFeBlend *>(filterPrimitive);
        switch (blend->mode()) {
        case QSvgFeBlend::Mode::Normal:
            step.filterType = FilterNodeInfo::Type::BlendNormal;
            break;
        case QSvgFeBlend::Mode::Multiply:
            step.filterType = FilterNodeInfo::Type::BlendMultiply;
            break;
        case QSvgFeBlend::Mode::Screen:
            step.filterType = FilterNodeInfo::Type::BlendScreen;
            break;
        case QSvgFeBlend::Mode::Darken:
            step.filterType = FilterNodeInfo::Type::BlendDarken;
            break;
        case QSvgFeBlend::Mode::Lighten:
            step.filterType = FilterNodeInfo::Type::BlendLighten;
            break;
        }

        step.input2 = findInput(blend->input2(), &step.namedInput2);
        break;
    }
    case QSvgNode::FeComposite:
    {
        const QSvgFeComposite *composite =  static_cast<const QSvgFeComposite *>(filterPrimitive);
        switch (composite->compositionOperator()) {
        case QSvgFeComposite::Operator::Over:
            step.filterType = FilterNodeInfo::Type::CompositeOver;
            break;
        case QSvgFeComposite::Operator::In:
            step.filterType = FilterNodeInfo::Type::CompositeIn;
            break;
        case QSvgFeComposite::Operator::Out:
            step.filterType = FilterNodeInfo::Type::CompositeOut;
            break;
        case QSvgFeComposite::Operator::Atop:
            step.filterType = FilterNodeInfo::Type::CompositeAtop;
            break;
        case QSvgFeComposite::Operator::Xor:
            step.filterType = FilterNodeInfo::Type::CompositeXor;
            break;
        case QSvgFeComposite::Operator::Lighter:
            step.filterType = FilterNodeInfo::Type::CompositeLighter;
            break;
        case QSvgFeComposite::Operator::Arithmetic:
            step.filterType = FilterNodeInfo::Type::CompositeArithmetic;
            break;
        };

        step.input2 = findInput(composite->input2(), &step.namedInput2);
        step.filterParameter = composite->k();
        break;
    }
    case QSvgNode::FeOffset:
    {
        const QSvgFeOffset *offset = static_cast<const QSvgFeOffset *>(filterPrimitive);
        step.filterType = FilterNodeInfo::Type::Offset;
        step.filterParameter = QVariant::fromValue(QVector2D(offset->dx(), offset->dy()));
        break;

    }
    case QSvgNode::FeColormatrix:
    {
        const QSvgFeColorMatrix *colorMatrix =
            static_cast<const QSvgFeColorMatrix *>(filterPrimitive);
        step.filterType = FilterNodeInfo::Type::ColorMatrix;
        step.filterParameter = QVariant::fromValue(colorMatrix->matrix());
        break;
    }

    case QSvgNode::FeGaussianblur:
    {
        const QSvgFeGaussianBlur *gaussianBlur =
            static_cast<const QSvgFeGaussianBlur *>(filterPrimitive);

        if (gaussianBlur->edgeMode() == QSvgFeGaussianBlur::EdgeMode::Wrap)
            info.wrapMode = QSGTexture::Repeat;
        step.filterType = FilterNodeInfo::Type::GaussianBlur;
        if (!qFuzzyCompare(gaussianBlur->stdDeviationX(), gaussianBlur->stdDeviationY()))
            qCWarning(lcQuickVectorImage) << "Separate X and Y deviations not supported for gaussian blur";
        step.filterParameter = std::max(gaussianBlur->stdDeviationX(),
                                        gaussianBlur->stdDeviationY());
        break;
    }

    case QSvgNode::FeFlood:
    {
        const QSvgFeFlood *flood =
            static_cast<const QSvgFeFlood *>(filterPrimitive);

        step.filterType = FilterNodeInfo::Type::Flood;
        step.filterParameter = flood->color();
        break;
    }
    default:
        // Create a dummy filter node to make sure bindings still work for unsupported filters
        step.filterType = FilterNodeInfo::Type::None;
        break;
    }

    info.steps.append(step);
}

bool QSvgVisitorImpl::visitFeFilterPrimitiveNodeStart(const QSvgFeFilterPrimitive *node)
{
    m_filterPrimitives.append(node);
    return true;
}

void QSvgVisitorImpl::visitFeFilterPrimitiveNodeEnd(const QSvgFeFilterPrimitive *node)
{
    Q_UNUSED(node);
}

bool QSvgVisitorImpl::visitStructureNodeStart(const QSvgStructureNode *node)
{
    if (m_defsLevel > 0)
        return false;

    constexpr bool forceSeparatePaths = false;
    handleBaseNodeSetup(node);

    StructureNodeInfo info;

    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);
    info.forceSeparatePaths = forceSeparatePaths;
    info.isPathContainer = isPathContainer(node);
    info.stage = StructureNodeStage::Start;

    return m_generator->generateStructureNode(info);
}

void QSvgVisitorImpl::visitStructureNodeEnd(const QSvgStructureNode *node)
{
    handleBaseNodeEnd(node);
    //    qCDebug(lcQuickVectorGraphics) << "REVERT" << node->nodeId() << node->type() << (m_styleResolver->painter().pen().style() != Qt::NoPen) << m_styleResolver->painter().pen().color().name()
    //             << (m_styleResolver->painter().pen().brush().style() != Qt::NoBrush) << m_styleResolver->painter().pen().brush().color().name();

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    info.isPathContainer = isPathContainer(node);
    info.stage = StructureNodeStage::End;

    m_generator->generateStructureNode(info);
}

QString QSvgVisitorImpl::nextNodeId() const
{
    return QStringLiteral("_qt_node%1").arg(m_nodeIdCounter++);
}

bool QSvgVisitorImpl::visitDocumentNodeStart(const QSvgDocument *node)
{
    Q_ASSERT(m_defsLevel == 0);
    handleBaseNodeSetup(node);

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    const QSvgDocument *doc = static_cast<const QSvgDocument *>(node);
    info.size = doc->size();
    info.viewBox = doc->viewBox();
    info.isPathContainer = isPathContainer(node);
    info.forceSeparatePaths = false;
    info.stage = StructureNodeStage::Start;

    return m_generator->generateRootNode(info);
}

void QSvgVisitorImpl::visitDocumentNodeEnd(const QSvgDocument *node)
{
    handleBaseNodeEnd(node);
    qCDebug(lcQuickVectorImage) << "REVERT" << node->nodeId() << node->type() << (styleResolver->painter().pen().style() != Qt::NoPen)
                                   << styleResolver->painter().pen().color().name() << (styleResolver->painter().pen().brush().style() != Qt::NoBrush)
                                   << styleResolver->painter().pen().brush().color().name();

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    info.stage = StructureNodeStage::End;

    m_generator->generateRootNode(info);
}

static QString scrub(const QString &raw)
{
    QString res(raw.left(80));

    if (!res.isEmpty()) {
        constexpr QLatin1StringView legalSymbols("_-.:"); // Only valid SVG id characters
        qsizetype i = 0;
        do {
            if (res.at(i).isLetterOrNumber() || legalSymbols.contains(res.at(i)))
                i++;
            else
                res.remove(i, 1);
        } while (i < res.size());
    }

    return res;
}

void QSvgVisitorImpl::fillCommonNodeInfo(const QSvgNode *node, NodeInfo &info, const QString &idSuffix)
{
    const QString nodeId = scrub(node->nodeId());
    const QString key = nodeId.isEmpty()
                            ? QString::number(quintptr(node), 16)
                            : nodeId;
    info.id = m_idForNodeId.value(key);
    if (info.id.isEmpty()) {
        info.id = nextNodeId();
        m_idForNodeId.insert(key, info.id);
    }

    // Internal disambiguation when multiple items come from the same node
    info.id += idSuffix;

    if (!m_linkSuffix.isEmpty())
        info.id += m_linkSuffix;

    info.nodeId = nodeId;
    info.typeName = node->typeName();
    info.isDefaultTransform = node->style().transform.isDefault();

    QTransform xf = !info.isDefaultTransform ? node->style().transform->qtransform() : QTransform();
    info.transform.setDefaultValue(QVariant::fromValue(xf));
    info.isDefaultOpacity = node->style().opacity.isDefault();
    info.opacity.setDefaultValue(!info.isDefaultOpacity ? node->style().opacity->opacity() : 1.0);
    info.isVisible = node->isVisible();
    info.isDisplayed = node->displayMode() != QSvgNode::DisplayMode::NoneMode;

    if (node->hasFilter() || node->hasMask() || node->type() == QSvgNode::Type::Mask) {
        QImage dummy(1, 1, QImage::Format_RGB32);
        QPainter p(&dummy);
        QSvgExtraStates states;
        p.setPen(QPen(Qt::NoPen));
        info.bounds = node->internalBounds(&p, states);
    }

    if (node->hasMask()) {
        info.maskId = m_idForNodeId.value(node->maskId());
        if (info.maskId.isEmpty()) {
            info.maskId = nextNodeId();
            m_idForNodeId.insert(node->maskId(), info.maskId);
        }
    }

    if (node->hasFilter()) {
        info.filterId = m_idForNodeId.value(node->filterId());
        if (info.filterId.isEmpty()) {
            info.filterId = nextNodeId();
            m_idForNodeId.insert(node->filterId(), info.filterId);
        }
    }
}

QList<QSvgVisitorImpl::AnimationPair> QSvgVisitorImpl::collectAnimations(const QSvgNode *node,
                                                                         const QString &propertyName)
{
    QList<AnimationPair> ret;
    const QList<QSvgAbstractAnimation *> animations = node->document()->animator()->animationsForNode(node);
    for (const QSvgAbstractAnimation *animation : animations) {
        const QList<QSvgAbstractAnimatedProperty *> properties = animation->properties();
        for (const QSvgAbstractAnimatedProperty *property : properties) {
            if (property->propertyName() == propertyName)
                ret.append(std::make_pair(animation, property));
        }
    }

    return ret;
}

void QSvgVisitorImpl::applyAnimationsToProperty(const QList<AnimationPair> &animations,
                                                QQuickAnimatedProperty *outProperty,
                                                std::function<QVariant(const QSvgAbstractAnimatedProperty *, int index, int animationIndex)> calculateValue)
{
    qCDebug(lcVectorImageAnimations) << "Applying animations to property with default value"
                                     << outProperty->defaultValue();
    for (auto it = animations.constBegin(); it != animations.constEnd(); ++it) {
        qCDebug(lcVectorImageAnimations) << "    -> Add animation";
        const QSvgAbstractAnimation *animation = it->first;
        const QSvgAbstractAnimatedProperty *property = it->second;

        const int start = animation->start();
        const int repeatCount = animation->iterationCount();
        const int duration = animation->duration();

        bool freeze = false;
        bool replace = true;
        if (animation->animationType() == QSvgAbstractAnimation::SMIL) {
            const QSvgAnimateNode *animateNode = static_cast<const QSvgAnimateNode *>(animation);
            freeze = animateNode->fill() == QSvgAnimateNode::Freeze;
            replace = animateNode->additiveType() == QSvgAnimateNode::Replace;
        }

        qCDebug(lcVectorImageAnimations) << "        -> Start:" << start
                                         << ", repeatCount:" << repeatCount
                                         << ", freeze:" << freeze
                                         << ", replace:" << replace;

        QList<qreal> propertyKeyFrames = property->keyFrames();
        QList<QQuickAnimatedProperty::PropertyAnimation> outAnimations;

        // For transform animations, we register the type of the transform in the animation
        // (this assumes that each animation is only for a single part of the transform)
        if (property->type() == QSvgAbstractAnimatedProperty::Transform) {
            const auto *transformProperty = static_cast<const QSvgAnimatedPropertyTransform *>(property);
            const auto &components = transformProperty->components();
            Q_ASSERT(q20::cmp_greater_equal(components.size(),transformProperty->transformCount()));
            for (uint i = 0; i < transformProperty->transformCount(); ++i) {
                QQuickAnimatedProperty::PropertyAnimation outAnimation;
                outAnimation.repeatCount = repeatCount;
                outAnimation.startOffset = start;
                if (freeze)
                    outAnimation.flags |= QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;
                if (replace)
                    outAnimation.flags |= QQuickAnimatedProperty::PropertyAnimation::ReplacePreviousAnimations;
                switch (components.at(i).type) {
                case QSvgAnimatedPropertyTransform::TransformComponent::Translate:
                    outAnimation.subtype = QTransform::TxTranslate;
                    break;
                case QSvgAnimatedPropertyTransform::TransformComponent::Scale:
                    outAnimation.subtype = QTransform::TxScale;
                    break;
                case QSvgAnimatedPropertyTransform::TransformComponent::Rotate:
                    outAnimation.subtype = QTransform::TxRotate;
                    break;
                case QSvgAnimatedPropertyTransform::TransformComponent::Skew:
                    outAnimation.subtype = QTransform::TxShear;
                    break;
                default:
                    qCWarning(lcQuickVectorImage()) << "Unhandled transform type:" << components.at(i).type;
                    break;
                }

                qDebug(lcVectorImageAnimations) << "        -> Property type:"
                                                << property->type()
                                                << " name:"
                                                << property->propertyName()
                                                << " animation subtype:"
                                                << outAnimation.subtype;

                outAnimations.append(outAnimation);
            }
        } else {
            QQuickAnimatedProperty::PropertyAnimation outAnimation;
            outAnimation.repeatCount = repeatCount;
            outAnimation.startOffset = start;
            if (freeze)
                outAnimation.flags |= QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;

            qDebug(lcVectorImageAnimations) << "        -> Property type:"
                                            << property->type()
                                            << " name:"
                                            << property->propertyName();

            outAnimations.append(outAnimation);
        }

        outProperty->beginAnimationGroup();
        for (int i = 0; i < outAnimations.size(); ++i) {
            QQuickAnimatedProperty::PropertyAnimation outAnimation = outAnimations.at(i);

            for (int j = 0; j < propertyKeyFrames.size(); ++j) {
                const int time = qRound(propertyKeyFrames.at(j) * duration);

                const QVariant value = calculateValue(property, j, i);
                outAnimation.frames[time] = value;
                qCDebug(lcVectorImageAnimations) << "        -> Frame " << time << " is " << value;
            }

            outProperty->addAnimation(outAnimation);
        }
    }
}

void QSvgVisitorImpl::fillColorAnimationInfo(const QSvgNode *node, PathNodeInfo &info)
{
    // Collect all animations affecting fill
    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("fill"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.fillColor, calculateInterpolatedValue);
    }

    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("fill-opacity"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.fillOpacity, calculateInterpolatedValue);
    }

    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("stroke"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.strokeStyle.color, calculateInterpolatedValue);
    }

    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("stroke-opacity"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.strokeStyle.opacity, calculateInterpolatedValue);
    }
}

void QSvgVisitorImpl::fillTransformAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    qCDebug(lcVectorImageAnimations) << "Applying transform animations to property with default value"
                                     << info.transform.defaultValue();

    auto calculateValue = [](const QSvgAbstractAnimatedProperty *property, int index, int animationIndex) {
        if (property->type() != QSvgAbstractAnimatedProperty::Transform)
            return QVariant{};

        const auto *transformProperty = static_cast<const QSvgAnimatedPropertyTransform *>(property);
        const auto &components = transformProperty->components();

        const int componentIndex = index * transformProperty->transformCount() + animationIndex;

        QVariantList parameters;

        const QSvgAnimatedPropertyTransform::TransformComponent &component = components.at(componentIndex);
        switch (component.type) {
        case QSvgAnimatedPropertyTransform::TransformComponent::Translate:
            parameters.append(QVariant::fromValue(QPointF(component.values.value(0),
                                                          component.values.value(1))));
            break;
        case QSvgAnimatedPropertyTransform::TransformComponent::Rotate:
            parameters.append(QVariant::fromValue(QPointF(component.values.value(1),
                                                          component.values.value(2))));
            parameters.append(QVariant::fromValue(component.values.value(0)));
            break;
        case QSvgAnimatedPropertyTransform::TransformComponent::Scale:
            parameters.append(QVariant::fromValue(QPointF(component.values.value(0),
                                                          component.values.value(1))));
            break;
        case QSvgAnimatedPropertyTransform::TransformComponent::Skew:
            parameters.append(QVariant::fromValue(QPointF(component.values.value(0),
                                                          component.values.value(1))));
            break;
        default:
            qCWarning(lcVectorImageAnimations) << "Unhandled transform type:" << component.type;
        };

        return QVariant::fromValue(parameters);
    };


    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("transform"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.transform, calculateValue);
    }
}

void QSvgVisitorImpl::fillMotionPathAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("offset-distance"));
    if (animations.isEmpty())
        return;

    if (animations.size() > 1) {
        qCWarning(lcQuickVectorImage)
            << "Not supported: More than one offset path animation on same node";
    }

    if (node->style().offset == nullptr) {
        qCWarning(lcQuickVectorImage) << "Motion path animation: No offset path";
        return;
    }

    const AnimationPair &animationPair = animations.first();

    const QSvgAbstractAnimation *animation = animationPair.first;
    const QSvgAbstractAnimatedProperty *property = animationPair.second;

    const int start = animation->start();
    const int repeatCount = animation->iterationCount();
    const int duration = animation->duration();

    qCDebug(lcVectorImageAnimations) << "Motion path animation:"
                                     << "start == " << start
                                     << ", repeatCount == " << repeatCount
                                     << "; duration == " << duration;


    QQuickAnimatedProperty::PropertyAnimation outAnimation;
    outAnimation.repeatCount = repeatCount;
    outAnimation.startOffset = start;

    QPainterPath originalPath = node->style().offset->path();

    qreal baseRotation;
    bool adaptAngle;
    switch (node->style().offset->rotateType()) {
    case QtSvg::OffsetRotateType::Auto:
        adaptAngle = true;
        baseRotation = 0.0;
        break;
    case QtSvg::OffsetRotateType::Angle:
        adaptAngle = false;
        baseRotation = node->style().offset->rotateAngle();
        break;
    case QtSvg::OffsetRotateType::AutoAngle:
        adaptAngle = true;
        baseRotation = node->style().offset->rotateAngle();
        break;
    case QtSvg::OffsetRotateType::Reverse:
        adaptAngle = true;
        baseRotation = 180.0;
        break;
    case QtSvg::OffsetRotateType::ReverseAngle:
        adaptAngle = true;
        baseRotation = node->style().offset->rotateAngle() + 180.0f;
        break;
    default:
        Q_UNREACHABLE();
    }

    // Default value holds additional parameters
    info.motionPath.setDefaultValue(QVariant::fromValue(QVariantPair(adaptAngle, baseRotation)));

    const QList<qreal> propertyKeyFrames = property->keyFrames();

    qreal previousT = 0.0;
    for (int j = 0; j < propertyKeyFrames.size(); ++j) {
        const int time = qRound(propertyKeyFrames.at(j) * duration);

        qreal t = calculateInterpolatedValue(property, j, 0).toReal();

        if (time > 0) {
            QPainterPath path = originalPath.trimmed(previousT, t);

            outAnimation.frames[time] = QVariant::fromValue(path);
            qCDebug(lcVectorImageAnimations) << "        -> Frame " << time << " is " << path;
        }

        previousT = t;
    }

    info.motionPath.addAnimation(outAnimation);
}

void QSvgVisitorImpl::fillPathAnimationInfo(const QSvgNode *node, PathNodeInfo &info)
{
    fillColorAnimationInfo(node, info);
    fillAnimationInfo(node, info);
}

void QSvgVisitorImpl::fillAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    {
        QList<AnimationPair> animations = collectAnimations(node, QStringLiteral("opacity"));
        if (!animations.isEmpty())
            applyAnimationsToProperty(animations, &info.opacity, calculateInterpolatedValue);
    }

    fillTransformAnimationInfo(node, info);
    fillMotionPathAnimationInfo(node, info);
}

void QSvgVisitorImpl::handleBaseNodeSetup(const QSvgNode *node)
{
    qCDebug(lcQuickVectorImage) << "Before SETUP" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor() << styleResolver->currentStrokeWidth()
                                   << node->nodeId() << " type: " << node->typeName()  << " " << node->type();

    node->applyStyle(&styleResolver->painter(), styleResolver->states());

    qCDebug(lcQuickVectorImage) << "After SETUP" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor()
                                   << styleResolver->currentStrokeWidth() << node->nodeId();
}

void QSvgVisitorImpl::handleBaseNode(const QSvgNode *node)
{
    NodeInfo info;
    fillCommonNodeInfo(node, info);

    m_generator->generateNodeBase(info);
}

void QSvgVisitorImpl::handleBaseNodeEnd(const QSvgNode *node)
{
    node->revertStyle(&styleResolver->painter(), styleResolver->states());

    qCDebug(lcQuickVectorImage) << "After END" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor() << styleResolver->currentStrokeWidth()
                                   << node->nodeId();
}

void QSvgVisitorImpl::handlePathNode(const QSvgNode *node, const QPainterPath &path)
{
    handleBaseNodeSetup(node);

    PathNodeInfo info;
    fillCommonNodeInfo(node, info);
    auto fillStyle = node->style().fill;
    if (fillStyle)
        info.fillRule = fillStyle->fillRule();

    const QGradient *strokeGradient = styleResolver->currentStrokeGradient();

    info.path.setDefaultValue(QVariant::fromValue(path));
    info.fillColor.setDefaultValue(styleResolver->currentFillColor());
    if (strokeGradient == nullptr) {
        info.strokeStyle = StrokeStyle::fromPen(styleResolver->currentStroke());
        info.strokeStyle.color.setDefaultValue(styleResolver->currentStrokeColor());
    }
    if (styleResolver->currentFillGradient() != nullptr)
        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());
    info.fillTransform = styleResolver->currentFillTransform();

    fillPathAnimationInfo(node, info);

    m_generator->generatePath(info);

    if (strokeGradient != nullptr) {
        PathNodeInfo strokeInfo;
        fillCommonNodeInfo(node, strokeInfo, QStringLiteral("_stroke"));

        strokeInfo.grad = *strokeGradient;

        QPainterPathStroker stroker(styleResolver->currentStroke());
        strokeInfo.path.setDefaultValue(QVariant::fromValue(stroker.createStroke(path)));
        m_generator->generatePath(strokeInfo);
    }

    handleBaseNodeEnd(node);
}

QT_END_NAMESPACE
