/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "SceneGlobalView.h"
#include "Painting.h"

#include <QtWidgets>

SceneGlobalView::SceneGlobalView(QGraphicsScene* scene, QGraphicsView* view, QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
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

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect();
    p.setPen(Painting::borderDark);
    //r.adjust(0.5, 0.5, -0.5, -0.5);
    p.setBrush(QColor(100, 100, 100));
    p.drawRect(r);


    if (mScene) {
        // --------------------------------------------------
        //  Target Rect
        // --------------------------------------------------
        QRectF sceneRect = mScene->sceneRect();
        QTransform matrix = mView->transform();
        sceneRect.setWidth( qMax(sceneRect.width() * matrix.m11(), 1.));
        sceneRect.setHeight( qMax(sceneRect.height() * matrix.m22(), 1.));

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
        QPointF visiblePos = mView->mapToScene( int (viewRect.x()), int (viewRect.y()));
        QRectF visibleRect(visiblePos.x(),
                           visiblePos.y(),
                           viewRect.width(),
                           viewRect.height());


        double propX = (visibleRect.x() - sceneRect.x()) / sceneRect.width();
        double propY = (visibleRect.y() - sceneRect.y()) / sceneRect.height();
        double propW = visibleRect.width() / sceneRect.width() ;
        double propH = visibleRect.height() / sceneRect.height();

        propX = (propX < 0) ? 0 : propX;
        propY = (propY < 0) ? 0 : propY;
        propW = (propW > 1) ? 1 : propW;
        propH = (propH > 1) ? 1 : propH;

        const QTransform m = mView->transform();
        const QRectF targetVisibleRect(targetRect.x() + targetRect.width() * propX * m.m11(),
                                 targetRect.y() + targetRect.height() * propY * m.m22(),
                                 targetRect.width() * propW,
                                 targetRect.height() * propH);

        p.setPen(Qt::red);
        p.setBrush(Qt::NoBrush);
        p.drawRect(targetVisibleRect);

    }

    p.setPen(Painting::borderDark);
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);
}

QRectF SceneGlobalView::getTargetRect()
{
    const int w  = width();
    const int h = height();

    QRectF sceneRect = mScene->sceneRect();
    const QTransform matrix = mView->transform();
    sceneRect.setWidth(sceneRect.width() * matrix.m11());
    sceneRect.setHeight(sceneRect.height() * matrix.m22());

    const double sceneProp = sceneRect.width() / sceneRect.height();
    QSizeF targetSize;

    if (sceneProp > w / h) {
        targetSize.setWidth(w);
        targetSize.setHeight(w / sceneProp);
    } else {
        targetSize.setHeight(h);
        targetSize.setWidth(h * sceneProp);
    }

    const QRectF targetRect((w - targetSize.width()) / 2,
                      (h - targetSize.height()) / 2,
                      targetSize.width(),
                      targetSize.height());

    return targetRect;
}

void SceneGlobalView::mousePressEvent(QMouseEvent* e)
{
    mIsDragging = true;
    setPosition(e->pos());
}

void SceneGlobalView::mouseReleaseEvent(QMouseEvent* e)
{
    (void)e;
    mIsDragging = false;
}

void SceneGlobalView::mouseMoveEvent(QMouseEvent* e)
{
    if (mIsDragging)
        setPosition(e->pos());
}

void SceneGlobalView::setPosition(const QPoint& pos)
{
    const QRectF &targetRect = getTargetRect();

    if (targetRect.contains(pos)) {
        const double propX = double (pos.x() - targetRect.x()) / targetRect.width();
        const double propY = double (pos.y() - targetRect.y()) / targetRect.height();

        const QRectF &sceneRect = mScene->sceneRect();
        const QPointF scenePos(sceneRect.x() + sceneRect.width() * propX,
                         sceneRect.y() + sceneRect.height() * propY);

        mView->centerOn(scenePos);
        update();
    }
}
