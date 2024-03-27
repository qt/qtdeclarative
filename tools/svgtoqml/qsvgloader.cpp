// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qsvgloader_p.h"
#include <private/qsvgvisitor_p.h>

#include <QString>
#include <QPainter>
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

QT_BEGIN_NAMESPACE

class RaiiStream : public QTextStream
{
public:
    RaiiStream() = default;
    explicit RaiiStream(QTextStream *stream): QTextStream(&output, QIODevice::ReadWrite), m_stream(stream) {}
    ~RaiiStream() {
        flush();
        if (m_stream && !output.isEmpty())
            *m_stream << output << Qt::endl;
    }

    RaiiStream(RaiiStream &other) = delete;
    RaiiStream &operator=(const RaiiStream &other) = delete;
    RaiiStream(RaiiStream &&other) : m_stream(std::exchange(other.m_stream, nullptr)), output(std::move(other.output)) {}
    RaiiStream &operator=(RaiiStream &&other) {
        std::swap(m_stream, other.m_stream);
        std::swap(output, other.output);
        return *this;
    }

private:
    QTextStream *m_stream = nullptr;
    QByteArray output;
};

class SvgLoaderVisitor : public QSvgVisitor
{
public:
    SvgLoaderVisitor();
    ~SvgLoaderVisitor();
    void setShapeTypeName(const QString &name) { m_shapeTypeName = name.toLatin1(); }
    void setFlags(QSvgQmlWriter::GeneratorFlags flags) { m_flags = flags; }
    QQuickItem *loadQML(QTextStream *stream, const QSvgTinyDocument *doc, QQuickItem *svgItem);

protected:
    void visitNode(const QSvgNode *) override;

    bool visitStructureNodeStart(const QSvgStructureNode *node) override;
    void visitStructureNodeEnd(const QSvgStructureNode *node) override;
    bool visitDefsNodeStart(const QSvgDefs *node) override;

    void visitTextNode(const QSvgText *node) override;
    void visitPathNode(const QSvgPath *node) override;
    void visitRectNode(const QSvgRect *node) override;
    void visitEllipseNode(const QSvgEllipse *node) override;

    void visitLineNode(const QSvgLine *node) override;
    void visitPolygonNode(const QSvgPolygon *node) override;
    void visitPolylineNode(const QSvgPolyline *node) override;

    void visitImageNode(const QSvgImage *node) override;

private:
    QString indent() { return QString().fill(' ', m_indentLevel * 4);}
    RaiiStream stream() {
        RaiiStream strm(m_stream);
        strm << indent();
        return strm;
    }

    const char *shapeName() const { return m_shapeTypeName.isEmpty() ? "Shape" : m_shapeTypeName.constData(); }

    QString currentFillColor() const;
    const QGradient *currentFillGradient() const;
    QString currentStrokeColor() const;
    float currentStrokeWidth() const;

    void handleBaseNodeSetup(const QSvgNode *node);
    void handleBaseNode(const QSvgNode *node);
    void handleBaseNodeEnd(const QSvgNode *node);
    void handlePathNode(const QSvgNode *node, const QPainterPath &path, Qt::PenCapStyle = Qt::SquareCap);
    void outputShapePath(const QSvgNode *node, const QPainterPath &path, Qt::PenCapStyle capStyle);
    void outputGradient(const QGradient *grad, QQuickShapePath *shapePath, const QRectF &boundingRect);

    enum PathSelector { FillPath = 0x1, StrokePath = 0x2, FillAndStroke = 0x3 };
    void outputShapePath(const QSvgNode *node, const QPainterPath *path, const QQuadPath *quadPath, Qt::PenCapStyle capStyle,
                         PathSelector pathSelector, const QRectF &boundingRect);

    QQuickItem *currentItem() { return m_items.top(); }
    void addCurrentItem(QQuickItem *item, const QSvgNode *node = nullptr) {
        item->setParentItem(currentItem());
        m_items.push(item);
        if (node) {
            if (!node->nodeId().isEmpty())
                item->setObjectName(node->nodeId());
            else
                item->setObjectName(node->typeName());
        }
    }

    int m_indentLevel = 0;
    QTextStream *m_stream = nullptr;
    QPainter m_dummyPainter;
    QImage m_dummyImage;
    QSvgExtraStates m_svgState;
    bool m_inShapeItem = false;
    QQuickShape *m_parentShapeItem = nullptr;
    QByteArray m_shapeTypeName;

    QStack<QQuickItem *> m_items;

    QQuickItem *m_loadedItem = nullptr;
    bool m_generateQML = true;
    bool m_generateItems = false;
    QSvgQmlWriter::GeneratorFlags m_flags;
};

SvgLoaderVisitor::SvgLoaderVisitor()
{
    m_dummyImage = QImage(1, 1, QImage::Format_RGB32);
    m_dummyPainter.begin(&m_dummyImage);
    QPen noPen(Qt::NoPen);
    noPen.setBrush(Qt::NoBrush);
    m_dummyPainter.setPen(noPen);
    m_dummyPainter.setBrush(Qt::black);
}

SvgLoaderVisitor::~SvgLoaderVisitor()
{
    m_dummyPainter.end();
}

void SvgLoaderVisitor::handlePathNode(const QSvgNode *node, const QPainterPath &path, Qt::PenCapStyle capStyle)
{
    handleBaseNodeSetup(node);

    QPainterPath pathCopy = path;
    auto fillStyle = node->style().fill;
    if (fillStyle)
        pathCopy.setFillRule(fillStyle->fillRule());
    if (m_inShapeItem) {
        if (!node->style().transform.isDefault())
            qWarning() << "Skipped transform for node" << node->nodeId() << "type" << node->typeName() << "(this is not supposed to happen)";
        outputShapePath(node, pathCopy, capStyle);
    } else {
        if (m_generateItems) {
            auto *shapeItem = new QQuickShape;
            shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
            shapeItem->setContainsMode(QQuickShape::ContainsMode::FillContains); // TODO: configurable?
            addCurrentItem(shapeItem, node);
            m_parentShapeItem = shapeItem;
        }
        m_inShapeItem = true;
        stream() << shapeName() << " {";
        handleBaseNode(node);
        m_indentLevel++;
        if (m_flags & QSvgQmlWriter::CurveRenderer)
            stream() << "preferredRendererType: Shape.CurveRenderer";
        outputShapePath(node, pathCopy, capStyle);
        //qDebug() << *node->qpath();
        m_indentLevel--;
        stream() << "}";
        if (m_generateItems)
            m_items.pop();
        m_inShapeItem = false;
        m_parentShapeItem = nullptr;
    }
    handleBaseNodeEnd(node);
}

void SvgLoaderVisitor::visitPathNode(const QSvgPath *node)
{
    stream() << "// PATH visit " << node->nodeId() << " count: " << node->path().elementCount();

    handlePathNode(node, node->path());
}

void SvgLoaderVisitor::visitLineNode(const QSvgLine *node)
{
    // TODO: proper end caps (should be flat by default?)
    QPainterPath p;
    p.moveTo(node->line().p1());
    p.lineTo(node->line().p2());
    handlePathNode(node, p, Qt::FlatCap);
}

static QPainterPath polygonToPath(const QPolygonF &poly, bool closed)
{
    QPainterPath path;
    if (poly.isEmpty())
        return path;
    bool first = true;
    for (const auto &p : poly) {
        if (first)
            path.moveTo(p);
        else
            path.lineTo(p);
        first = false;
    }
    if (closed)
        path.closeSubpath();
    return path;
}

void SvgLoaderVisitor::visitPolygonNode(const QSvgPolygon *node)
{
    stream() << "// POLYGON visit " << node->nodeId() << " count: " << node->polygon().count();
    QPainterPath p = polygonToPath(node->polygon(), true);
    handlePathNode(node, p);
}

void SvgLoaderVisitor::visitPolylineNode(const QSvgPolyline *node)
{
    stream() << "// POLYLINE visit " << node->nodeId() << " count: " << node->polygon().count();
    QPainterPath p = polygonToPath(node->polygon(), false);
    handlePathNode(node, p, Qt::FlatCap);
}

void SvgLoaderVisitor::visitImageNode(const QSvgImage *node)
{
    // TODO: this requires proper asset management.
    stream() << "// IMAGE visit " << node->nodeId() << " type: " << node->typeName() << " " << node->type();

    handleBaseNodeSetup(node);
    const auto &img = node->image();
    QRectF rect = node->rect();

    if (m_generateItems) {
        auto *imageItem = new QQuickImage;
        addCurrentItem(imageItem, node);
        auto *imagePriv = static_cast<QQuickImageBasePrivate*>(QQuickItemPrivate::get(imageItem));
        imagePriv->pix.setImage(img);

        imageItem->setX(rect.x());
        imageItem->setY(rect.y());
        imageItem->setWidth(rect.width());
        imageItem->setHeight(rect.height());
    }

    QString fn = img.hasAlphaChannel() ? QStringLiteral("svg_asset_%1.png").arg(img.cacheKey()) : QStringLiteral("svg_asset_%1.jpg").arg(img.cacheKey());
    if (m_generateQML) {
        // For now we just create a copy of the image in the current directory
        img.save(fn);
        qDebug() << "Saving copy of IMAGE" << fn;
    }
    stream() << "Image {";
    handleBaseNode(node);
    m_indentLevel++;
    stream() << "x: " << rect.x();
    stream() << "y: " << rect.y();
    stream() << "width: " << rect.width();
    stream() << "height: " << rect.height();
    stream() << "source: \"" << fn <<"\"";

    m_indentLevel--;
    stream() << "}";
    handleBaseNodeEnd(node);
    if (m_generateItems)
        m_items.pop();
}

void SvgLoaderVisitor::visitTextNode(const QSvgText *node)
{
    // TODO: font/size
    // TODO: fallback to path for gradient fill
    stream() << "// TEXT visit " << node->nodeId() << " type: " << node->typeName() << " " << node->type();
    QPointF pos = node->position();
    static int counter = 0;

    const bool isTextArea = node->type() == QSvgNode::Textarea;
    QQuickItem *alignItem = nullptr;
    QQuickText *textItem = nullptr;
    if (!isTextArea) {
        stream() << "Item { id: textAlignItem_" << counter << "; x: " << pos.x() << "; y: " << pos.y() << "}";
        if (m_generateItems) {
            alignItem = new QQuickItem(currentItem());
            alignItem->setX(pos.x());
            alignItem->setY(pos.y());
        }
    }
    stream() << "Text {";

    if (m_generateItems) {
        textItem = new QQuickText;
        addCurrentItem(textItem, node);
    }
    handleBaseNodeSetup(node);
    m_indentLevel++;

    if (isTextArea) {
        stream() << "x: " << pos.x();
        stream() << "y: " << pos.y();
        stream() << "width: " << node->size().width();
        stream() << "height: " << node->size().height();
        stream() << "wrapMode: Text.Wrap"; // ### WordWrap? verify with SVG standard
        stream() << "clip: true"; //### Not exactly correct: should clip on the text level, not the pixel level
        if (textItem) {
            textItem->setX(pos.x());
            textItem->setY(pos.y());
            textItem->setWidth(node->size().width());
            textItem->setHeight(node->size().height());
            textItem->setWrapMode(QQuickText::Wrap);
            textItem->setClip(true);
        }
    } else {
        auto *anchors = m_generateItems ? QQuickItemPrivate::get(textItem)->anchors() : nullptr;
        auto *alignPrivate = m_generateItems ? QQuickItemPrivate::get(alignItem) : nullptr;
        if (m_generateItems)
            anchors->setBaseline(alignPrivate->top());
        QString hAlign = QStringLiteral("left");
        stream() << "anchors.baseline: textAlignItem_" << counter << ".top";
        switch (m_svgState.textAnchor) {
        case Qt::AlignHCenter:
            hAlign = "horizontalCenter";
            if (m_generateItems)
                anchors->setHorizontalCenter(alignPrivate->left());
            break;
        case Qt::AlignRight:
            hAlign = "right";
            if (m_generateItems)
                anchors->setRight(alignPrivate->left());
            break;
        default:
            qDebug() << "Unexpected text alignment" << m_svgState.textAnchor;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            if (m_generateItems)
                anchors->setLeft(alignPrivate->left());
            break;
        }
        stream() << "anchors." << hAlign << ": textAlignItem_" << counter << ".left";
    }
    counter++;
    QString text;
    for (const auto *tspan : node->tspans()) {
        if (!tspan) {
            text += "<br>";
            continue;
        }

        if (!tspan->style().font.isDefault()) // TODO: switch to rich text when we have more complex spans with fonts?
            qDebug() << "Not implemented Tspan with font:" << tspan->style().font->qfont();
        QString spanColor;
        if (!tspan->style().fill.isDefault()) {
            auto &b = tspan->style().fill->qbrush();
            qDebug() << "tspan FILL:" << b;
            if (b.style() != Qt::NoBrush)
                spanColor = b.color().name();
        }
        bool fontTag = !spanColor.isEmpty();
        if (fontTag)
            text += QStringLiteral("<font color=\"%1\">").arg(spanColor); // TODO: size="1-7" ???
        text += tspan->text().toHtmlEscaped();
        if (fontTag)
            text += QStringLiteral("</font>");
    }
    stream() << "color: \"" << currentFillColor() << "\"";
    stream() << "textFormat: Text.StyledText";

    QFont font = m_dummyPainter.font();

    stream() << "text: \"" << text << "\""; // TODO: how about adding template<T> TestVisitor::streamProperty(const QString &name, const T &value)
    stream() << "font.family: \"" << font.family() << "\"";
    if (font.pixelSize() > 0)
        stream() << "font.pixelSize:" << font.pixelSize();
    else if (font.pointSize() > 0)
        stream() << "font.pixelSize:" << font.pointSizeF();
    if (font.underline())
        stream() << "font.underline: true";
    if (font.weight() != QFont::Normal)
        stream() << "font.weight: " << int(font.weight());

    if (font.pixelSize() <= 0 && font.pointSize() > 0)
        font.setPixelSize(font.pointSize()); // ### TODO: this makes no sense ###

    if (m_generateItems) {
        textItem->setColor(QColor::fromString(currentFillColor()));
        textItem->setTextFormat(QQuickText::StyledText);
        textItem->setText(text);
        textItem->setFont(font);
    }

    m_indentLevel--;
    stream() << "}";
    if (m_generateItems)
        m_items.pop();
    handleBaseNodeEnd(node);
}

//#define EXTRA_DEBUG
#ifdef EXTRA_DEBUG
static int nodeSetupLevel = 0;
#endif

void SvgLoaderVisitor::handleBaseNodeSetup(const QSvgNode *node)
{
#ifdef EXTRA_DEBUG
    qDebug() << QByteArray().fill(' ', m_indentLevel * 2).constData() << "before SETUP" << node << "fill" << currentFillColor()
        << "stroke" << currentStrokeColor() << currentStrokeWidth() << node->nodeId() << " type: " << node->typeName()  << " " << node->type() << "level" << nodeSetupLevel;
#endif
    node->applyStyle(&m_dummyPainter, m_svgState);

#ifdef EXTRA_DEBUG
    nodeSetupLevel++;
    qDebug() << QByteArray().fill(' ', m_indentLevel * 2).constData() << "after SETUP" << node << "fill" << currentFillColor()
             << "stroke" << currentStrokeColor() << currentStrokeWidth() << node->nodeId();
#endif
}

void SvgLoaderVisitor::handleBaseNode(const QSvgNode *node)
{
    m_indentLevel++;
    if (!node->nodeId().isEmpty())
        stream() << "objectName: \"" << node->nodeId() << "\""; // or maybe "objectName: \"svg_node:" << node->nodeId()
    if (!node->style().transform.isDefault()) {
        QTransform tr = node->style().transform->qtransform();
        auto sx = tr.m11();
        auto sy = tr.m22();
        auto x = tr.m31();
        auto y = tr.m32();
        if (tr.type() == QTransform::TxTranslate) {
            stream() << "// Translate " << tr.m31() << ", " << tr.m32();
            stream() << "transform: Translate { " << "x: " << x << "; y: " << y << " }";
        } else if (tr.type() == QTransform::TxScale && !x && !y) {
            stream() << "// Scale " << tr.m11() << ", " << tr.m22();
            stream() << "transform: Scale { xScale: " << sx << "; yScale: " << sy << " }";
        } else {
            stream() << "// Complex xform " << tr.type();
            const QMatrix4x4 m(tr);
            auto xform = new QQuickMatrix4x4;
            xform->setMatrix(m);
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
        if (m_generateItems) {
            auto xformProp = currentItem()->transform();
            if (tr.type() == QTransform::TxTranslate) {
                auto *translate = new QQuickTranslate;
                translate->setX(x);
                translate->setY(y);
                xformProp.append(&xformProp, translate);
            } else if (tr.type() == QTransform::TxScale && !x && !y) {
                auto scale = new QQuickScale;
                scale->setParent(currentItem());
                scale->setXScale(sx);
                scale->setYScale(sy);
                xformProp.append(&xformProp, scale);
            } else {
                const QMatrix4x4 m(tr);
                auto xform = new QQuickMatrix4x4;
                xform->setMatrix(m);
                xformProp.append(&xformProp, xform);
            }
        }
    }
    if (!node->style().opacity.isDefault()) {
        stream() << "opacity: " << node->style().opacity->opacity();
        if (m_generateItems)
            currentItem()->setOpacity(node->style().opacity->opacity());
    }
    m_indentLevel--;
}

void SvgLoaderVisitor::handleBaseNodeEnd(const QSvgNode *node)
{
    node->revertStyle(&m_dummyPainter, m_svgState);
#ifdef EXTRA_DEBUG
    nodeSetupLevel--;
    qDebug() << QByteArray().fill(' ', m_indentLevel * 2).constData() << "AFTER" << node << "fill" << currentFillColor()
             << "stroke" << currentStrokeColor() << currentStrokeWidth() << node->nodeId() << "level" << nodeSetupLevel;
#endif
}

QString SvgLoaderVisitor::currentFillColor() const
{
    if (m_dummyPainter.brush().style() != Qt::NoBrush) {
        QColor c(m_dummyPainter.brush().color());
        c.setAlphaF(m_svgState.fillOpacity);
        //qDebug() << "FILL" << c << m_svgState.fillOpacity << c.name();
        return c.name(QColor::HexArgb);
    } else {
        return QStringLiteral("transparent");
    }
}

const QGradient *SvgLoaderVisitor::currentFillGradient() const
{

    if (m_dummyPainter.brush().style() == Qt::LinearGradientPattern || m_dummyPainter.brush().style() == Qt::RadialGradientPattern || m_dummyPainter.brush().style() == Qt::ConicalGradientPattern )
       return m_dummyPainter.brush().gradient();
    return nullptr;
}

QString SvgLoaderVisitor::currentStrokeColor() const
{
    if (m_dummyPainter.pen().style() != Qt::NoPen)
        return m_dummyPainter.pen().color().name();
    else if (m_dummyPainter.pen().brush().style() == Qt::SolidPattern)
        return m_dummyPainter.pen().brush().color().name();
    return {};
}

float SvgLoaderVisitor::currentStrokeWidth() const
{
    float penWidth = m_dummyPainter.pen().widthF();
    return penWidth ? penWidth : 1;
}

// Find the square that gives the same gradient in QGradient::LogicalMode as
// objModeRect does in QGradient::ObjectMode

// When the object's bounding box is not square, the stripes that are conceptually
// perpendicular to the gradient vector within object bounding box space shall render
// non-perpendicular relative to the gradient vector in user space due to application
// of the non-uniform scaling transformation from bounding box space to user space.
static QRectF mapToQtLogicalMode(const QRectF &objModeRect, const QRectF &boundingRect)
{

    QRect pixelRect(objModeRect.x() * boundingRect.width() + boundingRect.left(),
                    objModeRect.y() * boundingRect.height() + boundingRect.top(),
                    objModeRect.width() * boundingRect.width(),
                    objModeRect.height() * boundingRect.height());

    if (pixelRect.isEmpty()) // pure horizontal/vertical gradient
        return pixelRect;

    double w = boundingRect.width();
    double h = boundingRect.height();
    double objModeSlope = objModeRect.height() / objModeRect.width();
    double a = objModeSlope * w / h;

    // do calculation with origin == pixelRect.topLeft
    double x2 = pixelRect.width();
    double y2 = pixelRect.height();
    double x = (x2 + a * y2) / (1 + a * a);
    double y = y2 - (x - x2)/a;

    return QRectF(pixelRect.topLeft(), QSizeF(x,y));
}

static QString toSvgString(const QPainterPath &path)
{
    QString svgPathString;
    QTextStream strm(&svgPathString);

    for (int i = 0; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);
        if (element.isMoveTo()) {
            strm << "M " << element.x << " " << element.y << " ";
        } else if (element.isLineTo()) {
            strm << "L " << element.x << " " << element.y << " ";
        } else if (element.isCurveTo()) {
            QPointF c1(element.x, element.y);
            ++i;
            element = path.elementAt(i);

            QPointF c2(element.x, element.y);
            ++i;
            element = path.elementAt(i);
            QPointF ep(element.x, element.y);

            strm <<  "C "
                 <<  c1.x() << " "
                 <<  c1.y() << " "
                 <<  c2.x() << " "
                 <<  c2.y() << " "
                 <<  ep.x() << " "
                 <<  ep.y() << " ";
        }
    }

    return svgPathString;
}

static QString toSvgString(const QQuadPath &path)
{
    QString svgPathString;
    QTextStream strm(&svgPathString);
    path.iterateElements([&](const QQuadPath::Element &e, int) {
        if (e.isSubpathStart())
            strm << "M " << e.startPoint().x() << " " << e.startPoint().y() << " ";

        if (e.isLine()) {
            strm << "L " << e.endPoint().x() << " " << e.endPoint().y() << " ";
        } else {
            strm << "Q " << e.controlPoint().x() << " " << e.controlPoint().y() << " "
                 << e.endPoint().x() << " " << e.endPoint().y() << " ";
        }
    });

    return svgPathString;
}

void SvgLoaderVisitor::outputGradient(const QGradient *grad, QQuickShapePath *shapePath, const QRectF &boundingRect)
{
    auto setStops = [](QQuickShapeGradient *quickGrad, const QGradientStops &stops) {
        auto stopsProp = quickGrad->stops();
        for (auto &stop : stops) {
            auto *stopObj = new QQuickGradientStop(quickGrad);
            stopObj->setPosition(stop.first);
            stopObj->setColor(stop.second);
            stopsProp.append(&stopsProp, stopObj);
        }
    };

    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);
        stream() << "fillGradient: LinearGradient {";
        m_indentLevel++;

        QRectF gradRect(linGrad->start(), linGrad->finalStop());
        QRectF logRect = linGrad->coordinateMode() == QGradient::LogicalMode ? gradRect : mapToQtLogicalMode(gradRect, boundingRect);

        stream() << "x1: " << logRect.left();
        stream() << "y1: " << logRect.top();
        stream() << "x2: " << logRect.right();
        stream() << "y2: " << logRect.bottom();
        for (auto &stop : linGrad->stops()) {
            stream() << "GradientStop { position: " << stop.first << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        }
        m_indentLevel--;
        stream() << "}";

        if (shapePath) {
            auto *quickGrad = new QQuickShapeLinearGradient(shapePath);

            quickGrad->setX1(logRect.left());
            quickGrad->setY1(logRect.top());
            quickGrad->setX2(logRect.right());
            quickGrad->setY2(logRect.bottom());
            setStops(quickGrad, linGrad->stops());

            shapePath->setFillGradient(quickGrad);
        }
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        stream() << "fillGradient: RadialGradient {";
        m_indentLevel++;

        stream() << "centerX: " << radGrad->center().x();
        stream() << "centerY: " << radGrad->center().y();
        stream() << "centerRadius: " << radGrad->radius();
        stream() << "focalX: centerX; focalY: centerY";
        for (auto &stop : radGrad->stops()) {
            stream() << "GradientStop { position: " << stop.first << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        }
        m_indentLevel--;
        stream() << "}";

        if (shapePath) {
            auto *quickGrad = new QQuickShapeRadialGradient(shapePath);
            quickGrad->setCenterX(radGrad->center().x());
            quickGrad->setCenterY(radGrad->center().y());
            quickGrad->setCenterRadius(radGrad->radius());
            quickGrad->setFocalX(radGrad->center().x());
            quickGrad->setFocalY(radGrad->center().y());
            setStops(quickGrad, radGrad->stops());

            shapePath->setFillGradient(quickGrad);
        }
    }
}

void SvgLoaderVisitor::outputShapePath(const QSvgNode *node, const QPainterPath &path, Qt::PenCapStyle capStyle)
{
    const bool optimize = m_flags.testFlag(QSvgQmlWriter::OptimizePaths);
    QRectF boundingRect = path.boundingRect();
    if (optimize) {
        const bool outlineMode = m_flags.testFlag(QSvgQmlWriter::OutlineStrokeMode);
        QQuadPath strokePath = QQuadPath::fromPainterPath(path);
        bool fillPathNeededClose;
        QQuadPath fillPath = strokePath.subPathsClosed(&fillPathNeededClose);
        const bool intersectionsFound = QSGCurveProcessor::solveIntersections(fillPath, false);
        fillPath.addCurvatureData();
        QSGCurveProcessor::solveOverlaps(fillPath);

        const bool compatibleStrokeAndFill = !fillPathNeededClose && !intersectionsFound;

        if (compatibleStrokeAndFill || outlineMode) {
            outputShapePath(node, nullptr, &fillPath, capStyle, FillAndStroke, boundingRect);
        } else {
            outputShapePath(node, nullptr, &fillPath, capStyle, FillPath, boundingRect);
            outputShapePath(node, nullptr, &strokePath, capStyle, StrokePath, boundingRect);
        }
    } else {
        outputShapePath(node, &path, nullptr, capStyle, FillAndStroke, boundingRect);
    }
}

static QString pathHintString(const QQuadPath &qp)
{
    QString res;
    QTextStream str(&res);
    auto flags = qp.pathHints();
    if (!flags)
        return res;
    str << "pathHints:";
    bool first = true;

#define CHECK_PATH_HINT(flagName)              \
    if (flags.testFlag(QQuadPath::flagName)) { \
        if (!first)                            \
            str << " |";                       \
        first = false;                         \
        str << " ShapePath." #flagName;        \
    }

    CHECK_PATH_HINT(PathLinear)
    CHECK_PATH_HINT(PathQuadratic)
    CHECK_PATH_HINT(PathConvex)
    CHECK_PATH_HINT(PathFillOnRight)
    CHECK_PATH_HINT(PathSolid)
    CHECK_PATH_HINT(PathNonIntersecting)
    CHECK_PATH_HINT(PathNonOverlappingControlPointTriangles)

    return res;
}

void SvgLoaderVisitor::outputShapePath(const QSvgNode *node, const QPainterPath *painterPath, const QQuadPath *quadPath, Qt::PenCapStyle capStyle, PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    QString penName = currentStrokeColor();
    const bool noPen = penName.isEmpty() || penName == u"transparent";
    if (pathSelector == StrokePath && noPen)
        return;

    const bool noFill = !currentFillGradient() && currentFillColor() == u"transparent";
    if (pathSelector == FillPath && noFill)
        return;

    auto fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());
    stream() << "ShapePath {";
    m_indentLevel++;
    auto *shapePath = m_generateItems ? new QQuickShapePath : nullptr;
    if (!node->nodeId().isEmpty()) {
        switch (pathSelector) {
        case FillPath:
            stream() << "objectName: \"svg_fill_path:" << node->nodeId() << "\"";
            break;
        case StrokePath:
            stream() << "objectName: \"svg_stroke_path:" << node->nodeId() << "\"";
            break;
        case FillAndStroke:
            stream() << "objectName: \"svg_path:" << node->nodeId() << "\"";
            break;
        }
        if (shapePath)
            shapePath->setObjectName(QStringLiteral("svg_path:") + node->nodeId());
    }
    stream() << "// boundingRect: " << boundingRect.x() << ", " << boundingRect.y() << " " << boundingRect.width() << "x" << boundingRect.height();

    if (noPen || !(pathSelector & StrokePath)) {
        stream() << "strokeColor: \"transparent\"";
        if (shapePath)
            shapePath->setStrokeColor(Qt::transparent);
    } else {
        stream() << "strokeColor: \"" << penName << "\"";
        stream() << "strokeWidth: " << currentStrokeWidth();
        if (shapePath) {
            shapePath->setStrokeColor(QColor::fromString(penName));
            shapePath->setStrokeWidth(currentStrokeWidth());
        }
    }
    if (capStyle == Qt::FlatCap)
        stream() << "capStyle: ShapePath.FlatCap"; //### TODO Add the rest of the styles, as well as join styles etc.

    if (shapePath)
        shapePath->setCapStyle(QQuickShapePath::CapStyle(capStyle));

    if (!(pathSelector & FillPath)) {
        stream() << "fillColor: \"transparent\"";
        if (shapePath)
            shapePath->setFillColor(Qt::transparent);
    } else if (auto *grad = currentFillGradient()) {
        outputGradient(grad, shapePath, boundingRect);
    } else {
        stream() << "fillColor: \"" << currentFillColor() << "\"";
        if (shapePath)
            shapePath->setFillColor(QColor::fromString(currentFillColor()));
    }
    if (fillRule == QQuickShapePath::WindingFill)
        stream() << "fillRule: ShapePath.WindingFill";
    else
        stream() << "fillRule: ShapePath.OddEvenFill";
    if (shapePath)
        shapePath->setFillRule(fillRule);

    if (quadPath) {
        QString hintStr = pathHintString(*quadPath);
        if (!hintStr.isEmpty())
            stream() << hintStr;
    }

    QString svgPathString = painterPath ? toSvgString(*painterPath) : toSvgString(*quadPath);
    stream() <<   "PathSvg { path: \"" << svgPathString << "\" }";

    if (shapePath) {
        auto *pathSvg = new QQuickPathSvg;
        pathSvg->setPath(svgPathString);
        pathSvg->setParent(shapePath);

        auto pathElementProp = shapePath->pathElements();
        pathElementProp.append(&pathElementProp, pathSvg);

        shapePath->setParent(currentItem());
        auto shapeDataProp = m_parentShapeItem->data();
        shapeDataProp.append(&shapeDataProp, shapePath);
    }
    m_indentLevel--;
    stream() << "}";
}

static bool isPathContainer(const QSvgStructureNode *node)
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
            //qDebug() << "NOT path container because" << node->typeName() ;
            return false;

            // nodes that could go inside Shape{}
        case QSvgNode::Defs:
        case QSvgNode::Image:
        case QSvgNode::Textarea:
        case QSvgNode::Text:
        case QSvgNode::Tspan:
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
                //qDebug() << "NOT path container because local transform";
                return false;
            }
            foundPath = true;
            break;
        default:
            qDebug() << "Unhandled type in switch" << child->type();
            break;
        }
    }
    //qDebug() << "Container" << node->nodeId() << node->typeName()  << "is" << foundPath;
    return foundPath;
}

namespace {
class ViewBoxItem : public QQuickItem
{
public:
    ViewBoxItem(const QRectF viewBox, QQuickItem *parent = nullptr) : QQuickItem(parent), m_viewBox(viewBox) { setXForm(); }

protected:
    void geometryChange(const QRectF &/*newGeometry*/, const QRectF &/*oldGeometry*/) override
    {
        setXForm();
    }

private:
    void setXForm()
    {
        auto xformProp = transform();
        xformProp.clear(&xformProp);
        bool translate = !qFuzzyIsNull(m_viewBox.x()) || !qFuzzyIsNull(m_viewBox.y());
        if (translate) {
            auto *tr = new QQuickTranslate(this);
            tr->setX(-m_viewBox.x());
            tr->setY(-m_viewBox.y());
            xformProp.append(&xformProp, tr);
        }
        if (!m_viewBox.isEmpty() && width() && height()) {
            auto *scale = new QQuickScale(this);
            qreal sx = width() / m_viewBox.width();
            qreal sy = height() / m_viewBox.height();

            scale->setXScale(sx);
            scale->setYScale(sy);
            xformProp.append(&xformProp, scale);
        }
    }
    QRectF m_viewBox;
};
}

bool SvgLoaderVisitor::visitDefsNodeStart(const QSvgDefs *node)
{
    stream() << "// skipping DEFS \"" << node->nodeId() << "\"";
    return false; // skip to end; TODO: implement defs
}

bool SvgLoaderVisitor::visitStructureNodeStart(const QSvgStructureNode *node)
{
    constexpr bool forceSeparatePaths = false;
    handleBaseNodeSetup(node);
    stream() << "// START " << node->nodeId() << " type: " << node->typeName() << " " << node->type();

    bool isTopLevel = node->type() == QSvgNode::Doc;
    bool hasViewBox = false;
    QRectF viewBox;

    if (isTopLevel) {
        auto *doc = static_cast<const QSvgTinyDocument *>(node);
        viewBox = doc->viewBox();
        hasViewBox = !viewBox.isEmpty();
    }

    if (!forceSeparatePaths && !isTopLevel && isPathContainer(node)) {
        stream() << shapeName() <<" { //combined path container";
        m_indentLevel++;
        if (m_flags & QSvgQmlWriter::CurveRenderer)
            stream() << "preferredRendererType: Shape.CurveRenderer";
        m_indentLevel--;

        m_inShapeItem = true;
        if (m_generateItems) {
            auto *shapeItem = new QQuickShape;
            shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer); // TODO: settable
            m_parentShapeItem = shapeItem;
            addCurrentItem(shapeItem, node);
        }
    } else {
        stream() << "Item { // structure node";
        if (m_generateItems) {
            auto *item = hasViewBox ? new ViewBoxItem(viewBox) : new QQuickItem;
            addCurrentItem(item, node );
            if (isTopLevel)
                m_loadedItem = item;
        }
    }
    if (hasViewBox) {
        m_indentLevel++;
        stream() << "transform: [";
        m_indentLevel++;
        bool translate = !qFuzzyIsNull(viewBox.x()) || !qFuzzyIsNull(viewBox.y());
        if (translate)
            stream() << "Translate { x: " << -viewBox.x() << "; y: " << -viewBox.y() << " },";
        stream() << "Scale { xScale: width / " << viewBox.width() << "; yScale: height / " << viewBox.height() << " }";
        m_indentLevel--;
        stream() << "]";;
        m_indentLevel--;
    }
    handleBaseNode(node);
    m_indentLevel++;
    return true;
}

void SvgLoaderVisitor::visitStructureNodeEnd(const QSvgStructureNode *node)
{
    m_indentLevel--;
    stream() << "} // END " << node->nodeId() << " type: " << node->typeName() << " " << node->type();
    handleBaseNodeEnd(node);
//    qDebug() << "REVERT" << node->nodeId() << node->type() << (m_dummyPainter.pen().style() != Qt::NoPen) << m_dummyPainter.pen().color().name()
//             << (m_dummyPainter.pen().brush().style() != Qt::NoBrush) << m_dummyPainter.pen().brush().color().name();
    m_inShapeItem = false;
    m_parentShapeItem = nullptr;
    if (m_generateItems)
        m_items.pop();
}

void SvgLoaderVisitor::visitRectNode(const QSvgRect *node)
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
    // qDebug() << "Line1" << x2 - rx << y1;
    p.arcTo(x2 - rx * 2, y1, rx * 2, ry * 2, 90, -90); // ARC to x2, y1 + ry
    // qDebug() << "p1" << p;

    p.lineTo(x2, y2 - ry);
    p.arcTo(x2 - rx * 2, y2 - ry * 2, rx * 2, ry * 2, 0, -90); // ARC to x2 - rx, y2

    p.lineTo(x1 + rx, y2);
    p.arcTo(x1, y2 - ry * 2, rx * 2, ry * 2, 270, -90); // ARC to x1, y2 - ry

    p.lineTo(x1, y1 + ry);
    p.arcTo(x1, y1, rx * 2, ry * 2, 180, -90); // ARC to x1 + rx, y1


    stream() << "// Path from rect " << node->nodeId() << " r " << rect.x() << ", " << rect.y()
             << " " << rect.width() << "x" << rect.height() << " R: " << rads.x() << ", " << rads.y();

    handlePathNode(node, p);

    return;

}

void SvgLoaderVisitor::visitEllipseNode(const QSvgEllipse *node)
{
    QRectF rect = node->rect();
    stream() << "// Ellipse" << node->nodeId() << " rect: " << rect.x() << ", " << rect.y()
             << " " << rect.width() << "x" << rect.height();
    QPainterPath p;
    p.addEllipse(rect);

    handlePathNode(node, p);
}

QQuickItem *SvgLoaderVisitor::loadQML(QTextStream *outStream, const QSvgTinyDocument *doc, QQuickItem *parentItem)
{
    Q_ASSERT(outStream);
    m_stream = outStream;
    m_indentLevel = 0;

    stream() << "import QtQuick";
    stream() << "import QtQuick.Shapes" << Qt::endl;

    QRectF viewBox = doc->viewBox();

    m_generateItems = parentItem != nullptr;
    m_items.push(parentItem);

    stream() << "Item {";
    m_indentLevel++;
    stream() << "// viewBox " << viewBox.x() << ", " << viewBox.y()
         << " " << viewBox.width() << "x" << viewBox.height();
    stream() << "// size " << doc->width() << "x" << doc->height();
    double w = doc->width();
    double h = doc->height();
    if (w > 0)
        stream() << "implicitWidth: " << w;
    if (h > 0)
        stream() << "implicitHeight: " << h;

    if (parentItem) {
        m_generateItems = true;
        parentItem->setImplicitWidth(w);
        parentItem->setImplicitHeight(h);
    }
    traverse(doc);
    m_indentLevel--;
    stream() << "}";
    return m_loadedItem;
}

void SvgLoaderVisitor::visitNode(const QSvgNode *node)
{
    handleBaseNodeSetup(node);
    stream() << "//### SVG NODE NOT IMPLEMENTED: " << node->nodeId() << " type: " << node->typeName()  << " " << node->type();
    stream() << "Item {";
    handleBaseNode(node);
    stream() << "}";
    handleBaseNodeEnd(node);
}

QQuickItem *QSvgQmlWriter::loadSVG(const QSvgTinyDocument *doc, const QString &outFileName, GeneratorFlags flags, const QString &typeName, QQuickItem *parentItem, const QString &commentString)
{
    SvgLoaderVisitor visitor;
    if (!typeName.isEmpty())
        visitor.setShapeTypeName(typeName);
    visitor.setFlags(flags);
    QByteArray result;
    QTextStream str(&result);
    if (commentString.isEmpty())
        str << "// Generated from SVG" << Qt::endl;
    else
        str << "// " << commentString << Qt::endl;
    auto *loadedItem = visitor.loadQML(&str, doc, parentItem);
    if (!outFileName.isEmpty()) {
        QFile outFile(outFileName);
        outFile.open(QIODevice::WriteOnly);
        outFile.write(result);
        outFile.close();
    }
#if 0
    result.truncate(300);
    qDebug().noquote() << result;
#endif
    return loadedItem;
}

QT_END_NAMESPACE
