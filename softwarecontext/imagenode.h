#ifndef IMAGENODE_H
#define IMAGENODE_H

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgtexturematerial_p.h>

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
