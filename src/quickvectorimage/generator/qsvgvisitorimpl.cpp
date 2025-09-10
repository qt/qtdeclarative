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

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcQuickVectorImage)

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
        case QSvgNode::Animation:
        case QSvgNode::Use:
        case QSvgNode::Video:
        case QSvgNode::Image:
        case QSvgNode::Textarea:
        case QSvgNode::Text:
        case QSvgNode::Tspan:
            //qCDebug(lcQuickVectorGraphics) << "NOT path container because" << node->typeName() ;
            return false;

            // nodes that could go inside Shape{}
        case QSvgNode::Defs:
            break;

            // nodes that are done as pure ShapePath{}
        case QSvgNode::Rect:
        case QSvgNode::Circle:
        case QSvgNode::Ellipse:
        case QSvgNode::Line:
        case QSvgNode::Path:
        case QSvgNode::Polygon:
        case QSvgNode::Polyline:
            if (!child->style().transform.isDefault()) {
                //qCDebug(lcQuickVectorGraphics) << "NOT path container because local transform";
                return false;
            }
            foundPath = true;
            break;
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

QSvgVisitorImpl::QSvgVisitorImpl(const QString svgFileName, QQuickGenerator *generator)
    : m_svgFileName(svgFileName)
    , m_generator(generator)
{
}

bool QSvgVisitorImpl::traverse()
{
    if (!m_generator) {
        qCDebug(lcQuickVectorImage) << "No valid QQuickGenerator is set. Genration will stop";
        return false;
    }

    auto *doc = QSvgTinyDocument::load(m_svgFileName);
    if (!doc) {
        qCDebug(lcQuickVectorImage) << "Not a valid Svg File : " << m_svgFileName;
        return false;
    }

    QSvgVisitor::traverse(doc);
    return true;
}

void QSvgVisitorImpl::visitNode(const QSvgNode *node)
{
    handleBaseNodeSetup(node);

    NodeInfo info;
    fillCommonNodeInfo(node, info);

    m_generator->generateNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitImageNode(const QSvgImage *node)
{
    // TODO: this requires proper asset management.
    handleBaseNodeSetup(node);

    ImageNodeInfo info;
    fillCommonNodeInfo(node, info);
    info.image = node->image();
    info.rect = node->rect();
    info.externalFileReference = node->filename();

    m_generator->generateImageNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitRectNode(const QSvgRect *node)
{
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
    QRectF rect = node->rect();

    QPainterPath p;
    p.addEllipse(rect);

    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPathNode(const QSvgPath *node)
{
    handlePathNode(node, node->path());
}

void QSvgVisitorImpl::visitLineNode(const QSvgLine *node)
{
    QPainterPath p;
    p.moveTo(node->line().p1());
    p.lineTo(node->line().p2());
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolygonNode(const QSvgPolygon *node)
{
    QPainterPath p = QQuickVectorImageGenerator::Utils::polygonToPath(node->polygon(), true);
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolylineNode(const QSvgPolyline *node)
{
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
        if (ucs4 < USHRT_MAX && m_font->m_glyphs.contains(QChar(ushort(ucs4))))
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
                QPainterPath glyphPath = m_font->m_glyphs.value(QChar(ushort(index))).m_path;

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
        const QSvgGlyph &svgGlyph = m_font->m_glyphs.value(QChar(ushort(glyph)));
        ret.width = QFixed::fromReal(svgGlyph.m_horizAdvX * scale);
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
            glyph_t glyph = glyphLayout->glyphs[i];
            const QSvgGlyph &svgGlyph = m_font->m_glyphs.value(QChar(ushort(glyph)));
            glyphLayout->advances[i] = QFixed::fromReal(svgGlyph.m_horizAdvX * scale);
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

void QSvgVisitorImpl::visitTextNode(const QSvgText *node)
{
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
            text += QStringLiteral("<span style=\"%1\">").arg(styleTagContent);

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

                auto addPathForFormat = [&](QPainterPath p, QTextCharFormat fmt) {
                    PathNodeInfo info;
                    fillCommonNodeInfo(node, info);
                    auto fillStyle = node->style().fill;
                    if (fillStyle)
                        info.fillRule = fillStyle->fillRule();

                    if (fmt.hasProperty(QTextCharFormat::ForegroundBrush)) {
                        info.fillColor = fmt.foreground().color();
                        if (fmt.foreground().gradient() != nullptr && fmt.foreground().gradient()->type() != QGradient::NoGradient)
                            info.grad = *fmt.foreground().gradient();
                    } else {
                        info.fillColor = styleResolver->currentFillColor();
                    }

                    info.painterPath = p;

                    const QGradient *strokeGradient = styleResolver->currentStrokeGradient();
                    QPen pen;
                    if (fmt.hasProperty(QTextCharFormat::TextOutline)) {
                        pen = fmt.textOutline();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color = pen.color();
                        }
                    } else {
                        pen = styleResolver->currentStroke();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color = styleResolver->currentStrokeColor();
                        }
                    }

                    if (info.grad.type() == QGradient::NoGradient && styleResolver->currentFillGradient() != nullptr)
                        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());

                    info.fillTransform = styleResolver->currentFillTransform();

                    m_generator->generatePath(info, boundingRect);

                    if (strokeGradient != nullptr) {
                        PathNodeInfo strokeInfo;
                        fillCommonNodeInfo(node, strokeInfo);

                        strokeInfo.grad = *strokeGradient;

                        QPainterPathStroker stroker(pen);
                        strokeInfo.painterPath = stroker.createStroke(p);
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

                    // If we hit a "multi" anchor, it means we have additional formats to apply
                    // for both this and the subsequent range, so we merge them.
                    if (!range.format.anchorNames().isEmpty()
                        && range.format.anchorNames().first().startsWith(QStringLiteral("multi"))
                        && i < formats.size() - 1) {
                        QTextLayout::FormatRange nextRange = formats.at(++i);
                        range.length += nextRange.length;
                        range.format.merge(nextRange.format);
                    }
                    QList<QGlyphRun> glyphRuns = lout->glyphRuns(range.start, range.length);
                    QList<QPainterPath> paths = glyphsToPath(glyphRuns, lout->minimumWidth());
                    for (const QPainterPath &path : paths)
                        addPathForFormat(path, range.format);
                }
            }

            block = block.next();
        }
    } else
#endif
    {
        TextNodeInfo info;
        fillCommonNodeInfo(node, info);

        info.position = node->position();
        info.size = node->size();
        info.font = font;
        info.text = text;
        info.isTextArea = isTextArea;
        info.needsRichText = needsRichText;
        info.fillColor = styleResolver->currentFillColor();
        info.alignment = styleResolver->states().textAnchor;
        info.strokeColor = styleResolver->currentStrokeColor();

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
    QSvgNode *link = node->link();
    if (!link)
        return;

    handleBaseNodeSetup(node);
    UseNodeInfo info;
    fillCommonNodeInfo(node, info);

    info.stage = StructureNodeStage::Start;
    info.startPos = node->start();

    m_generator->generateUseNode(info);

    QSvgVisitor::traverse(link);

    info.stage = StructureNodeStage::End;
    m_generator->generateUseNode(info);
    handleBaseNodeEnd(node);
}

bool QSvgVisitorImpl::visitSwitchNodeStart(const QSvgSwitch *node)
{
    QSvgNode *link = node->childToRender();
    if (!link)
        return false;

    QSvgVisitor::traverse(link);

    return false;
}

void QSvgVisitorImpl::visitSwitchNodeEnd(const QSvgSwitch *node)
{
    Q_UNUSED(node);
}

bool QSvgVisitorImpl::visitDefsNodeStart(const QSvgDefs *node)
{
    Q_UNUSED(node)

    return m_generator->generateDefsNode(NodeInfo{});
}

bool QSvgVisitorImpl::visitStructureNodeStart(const QSvgStructureNode *node)
{
    constexpr bool forceSeparatePaths = false;
    handleBaseNodeSetup(node);

    StructureNodeInfo info;

    fillCommonNodeInfo(node, info);
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
    info.stage = StructureNodeStage::End;

    m_generator->generateStructureNode(info);
}

bool QSvgVisitorImpl::visitDocumentNodeStart(const QSvgTinyDocument *node)
{
    handleBaseNodeSetup(node);

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);

    const QSvgTinyDocument *doc = static_cast<const QSvgTinyDocument *>(node);
    info.size = doc->size();
    info.viewBox = doc->viewBox();
    info.isPathContainer = isPathContainer(node);
    info.forceSeparatePaths = false;
    info.stage = StructureNodeStage::Start;

    return m_generator->generateRootNode(info);
}

void QSvgVisitorImpl::visitDocumentNodeEnd(const QSvgTinyDocument *node)
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

void QSvgVisitorImpl::fillCommonNodeInfo(const QSvgNode *node, NodeInfo &info)
{
    info.nodeId = node->nodeId();
    info.typeName = node->typeName();
    info.isDefaultTransform = node->style().transform.isDefault();
    info.transform = !info.isDefaultTransform ? node->style().transform->qtransform() : QTransform();
    info.isDefaultOpacity = node->style().opacity.isDefault();
    info.opacity = !info.isDefaultOpacity ? node->style().opacity->opacity() : 1.0;
    info.isVisible = node->isVisible();
    info.isDisplayed = node->displayMode() != QSvgNode::DisplayMode::NoneMode;
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

    info.painterPath = path;
    info.fillColor = styleResolver->currentFillColor();
    if (strokeGradient == nullptr) {
        info.strokeStyle = StrokeStyle::fromPen(styleResolver->currentStroke());
        info.strokeStyle.color = styleResolver->currentStrokeColor();
    }
    if (styleResolver->currentFillGradient() != nullptr)
        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());
    info.fillTransform = styleResolver->currentFillTransform();

    m_generator->generatePath(info);

    if (strokeGradient != nullptr) {
        PathNodeInfo strokeInfo;
        fillCommonNodeInfo(node, strokeInfo);

        strokeInfo.grad = *strokeGradient;

        QPainterPathStroker stroker(styleResolver->currentStroke());
        strokeInfo.painterPath = stroker.createStroke(path);
        m_generator->generatePath(strokeInfo);
    }

    handleBaseNodeEnd(node);
}

QT_END_NAMESPACE
