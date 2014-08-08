#ifndef IMAGENODE_H
#define IMAGENODE_H

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgtexturematerial_p.h>

typedef QVarLengthArray<QPainter::PixmapFragment, 16> QPixmapFragmentsArray;

struct QTileRules
{
    inline QTileRules(Qt::TileRule horizontalRule, Qt::TileRule verticalRule)
            : horizontal(horizontalRule), vertical(verticalRule) {}
    inline QTileRules(Qt::TileRule rule = Qt::StretchTile)
            : horizontal(rule), vertical(rule) {}
    Qt::TileRule horizontal;
    Qt::TileRule vertical;
};

#ifndef Q_QDOC
// For internal use only.
namespace QDrawBorderPixmap
{
    enum DrawingHint
    {
        OpaqueTopLeft = 0x0001,
        OpaqueTop = 0x0002,
        OpaqueTopRight = 0x0004,
        OpaqueLeft = 0x0008,
        OpaqueCenter = 0x0010,
        OpaqueRight = 0x0020,
        OpaqueBottomLeft = 0x0040,
        OpaqueBottom = 0x0080,
        OpaqueBottomRight = 0x0100,
        OpaqueCorners = OpaqueTopLeft | OpaqueTopRight | OpaqueBottomLeft | OpaqueBottomRight,
        OpaqueEdges = OpaqueTop | OpaqueLeft | OpaqueRight | OpaqueBottom,
        OpaqueFrame = OpaqueCorners | OpaqueEdges,
        OpaqueAll = OpaqueCenter | OpaqueFrame
    };

    Q_DECLARE_FLAGS(DrawingHints, DrawingHint)
}
#endif

namespace SoftwareContext {

void qDrawBorderPixmap(QPainter *painter, const QRect &targetRect, const QMargins &targetMargins,
                       const QPixmap &pixmap, const QRect &sourceRect,const QMargins &sourceMargins,
                       const QTileRules &rules, QDrawBorderPixmap::DrawingHints hints);

}

class ImageNode : public QSGImageNode
{
public:
    ImageNode();

    virtual void setTargetRect(const QRectF &rect);
    virtual void setInnerTargetRect(const QRectF &rect);
    virtual void setInnerSourceRect(const QRectF &rect);
    virtual void setSubSourceRect(const QRectF &rect);
    virtual void setTexture(QSGTexture *texture);
    virtual void setMirror(bool mirror);
    virtual void setMipmapFiltering(QSGTexture::Filtering filtering);
    virtual void setFiltering(QSGTexture::Filtering filtering);
    virtual void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode);
    virtual void setVerticalWrapMode(QSGTexture::WrapMode wrapMode);
    virtual void update();

    void paint(QPainter *painter);

private:
    QRectF m_targetRect;
    QRectF m_innerTargetRect;
    QRectF m_innerSourceRect;
    QRectF m_subSourceRect;

    QPixmap m_pixmap;
    bool m_mirror;
    bool m_smooth;
    bool m_tileHorizontal;
    bool m_tileVertical;
};

#endif // IMAGENODE_H
