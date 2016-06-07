#include "SceneGlobalView.h"
#include <QtWidgets>


SceneGlobalView::SceneGlobalView(QGraphicsScene* scene, QGraphicsView* view, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mScene(scene),
mView(view),
mIsDragging(false)
{
    this->setGeometry(parentWidget()->rect());
}

SceneGlobalView::~SceneGlobalView()
{
    
}

void SceneGlobalView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QRectF r = rect();
    r.adjust(0.5, 0.5, -0.5, -0.5);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(50, 50, 50));
    p.setBrush(QColor(100, 100, 100));
    p.drawRect(r);
    
    if (mScene) {
        // --------------------------------------------------
        //  Target Rect
        // --------------------------------------------------
        QRectF sceneRect = mScene->sceneRect();
        QMatrix matrix = mView->matrix();
        sceneRect.setWidth(sceneRect.width() * matrix.m11());
        sceneRect.setHeight(sceneRect.height() * matrix.m22());
        
        QRectF targetRect = getTargetRect();
        
        // --------------------------------------------------
        //  Scene image
        // --------------------------------------------------
        p.fillRect(targetRect, QColor(240, 240, 240));
        mScene->render(&p, targetRect, mScene->sceneRect(), Qt::KeepAspectRatio);
        
        // --------------------------------------------------
        //  Visible Rect
        // --------------------------------------------------
        QRectF viewRect = mView->rect();
        QPointF visiblePos = mView->mapToScene(viewRect.x(), viewRect.y());
        QRectF visibleRect(visiblePos.x(),
                           visiblePos.y(),
                           viewRect.width(),
                           viewRect.height());
        
        double propX = (visibleRect.x() - sceneRect.x()) / sceneRect.width();
        double propY = (visibleRect.y() - sceneRect.y()) / sceneRect.height();
        double propW = visibleRect.width() / sceneRect.width();
        double propH = visibleRect.height() / sceneRect.height();
        
        propX = (propX < 0) ? 0 : propX;
        propY = (propY < 0) ? 0 : propY;
        propW = (propW > 1) ? 1 : propW;
        propH = (propH > 1) ? 1 : propH;
        
        QRectF targetVisibleRect(targetRect.x() + targetRect.width() * propX,
                                 targetRect.y() + targetRect.height() * propY,
                                 targetRect.width() * propW,
                                 targetRect.height() * propH);
        
        p.setPen(Qt::red);
        p.setBrush(Qt::NoBrush);
        p.drawRect(targetVisibleRect);
        
     
        //qDebug() << "-----";
        //qDebug() << targetRect;
        //qDebug() << visibleTargetRect;
        
        //qDebug() << sceneRect;
        //qDebug() << visibleRect;
        //qDebug() << propX << ", " << propY << ", " << propW << ", " << propH;
        //qDebug() << "-----------";
    }
}

QRectF SceneGlobalView::getTargetRect()
{
    double w = width();
    double h = height();
    
    QRectF sceneRect = mScene->sceneRect();
    QMatrix matrix = mView->matrix();
    sceneRect.setWidth(sceneRect.width() * matrix.m11());
    sceneRect.setHeight(sceneRect.height() * matrix.m22());
    
    double sceneProp = sceneRect.width() / sceneRect.height();
    QSizeF targetSize;
    
    if (sceneProp > w / h) {
        targetSize.setWidth(w);
        targetSize.setHeight(w / sceneProp);
    } else {
        targetSize.setHeight(h);
        targetSize.setWidth(h * sceneProp);
    }
    
    QRectF targetRect((w - targetSize.width()) / 2,
                      (h - targetSize.height()) / 2,
                      targetSize.width(),
                      targetSize.height());
    
    return targetRect.adjusted(1, 1, -1, -1);
}

void SceneGlobalView::mousePressEvent(QMouseEvent* e)
{
    mIsDragging = true;
    setPosition(e->pos());
}

void SceneGlobalView::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsDragging = false;
}

void SceneGlobalView::mouseMoveEvent(QMouseEvent* e)
{
    if (mIsDragging)
        setPosition(e->pos());
}

void SceneGlobalView::setPosition(const QPoint& pos)
{
    QRectF targetRect = getTargetRect();
    
    if (targetRect.contains(pos)) {
        double propX = (double)(pos.x() - targetRect.x()) / targetRect.width();
        double propY = (double)(pos.y() - targetRect.y()) / targetRect.height();
        
        QRectF sceneRect = mScene->sceneRect();
        QPointF scenePos(sceneRect.x() + sceneRect.width() * propX,
                         sceneRect.y() + sceneRect.height() * propY);
        
        mView->centerOn(scenePos);
        update();
    }
}

