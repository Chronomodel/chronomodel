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

#include "Collapsible.h"

#include <QPainter>
#include <QPainterPath>

CollapsibleHeader::CollapsibleHeader(Collapsible* collapsible, Qt::WindowFlags flags):
    QWidget(collapsible, flags),
    mCollapsible(collapsible)
{
    setCursor(QCursor(Qt::PointingHandCursor));
}

CollapsibleHeader::~CollapsibleHeader()
{

}

void CollapsibleHeader::setTitle(const QString& title)
{
    mTitle = title;
    update();
}

void CollapsibleHeader::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Précalculer les couleurs
    const QColor backgroundColor(100, 100, 100);
    const QColor foregroundColor(200, 200, 200);


    p.fillRect(rect(), backgroundColor);

    // Calculer les rectangles
    const int dr = 7;
    const int ds = 1;
    const QRect r1(dr, dr, height() - 2 * dr, height() - 2 * dr);
    const QRect r2(height(), 0, width() - height(), height());

    p.setPen(foregroundColor);
    p.setBrush(foregroundColor);
    p.drawText(r2, Qt::AlignLeft | Qt::AlignVCenter, mTitle);

    QPainterPath path;

    if (mCollapsible->opened()) {
        path.moveTo(r1.x() + ds, r1.y());
        path.lineTo(r1.x() + r1.width() - ds, r1.y());
        path.lineTo(r1.x() + r1.width()/2, r1.y() + r1.height());

    } else {
        path.moveTo(r1.x(), r1.y() + ds);
        path.lineTo(r1.x(), r1.y() + r1.height() - ds);
        path.lineTo(r1.x() + r1.width(), r1.y() + r1.height()/2);
    }
    p.fillPath(path, foregroundColor);
}

void CollapsibleHeader::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if(mCollapsible)
        mCollapsible->open(!mCollapsible->opened());
}


Collapsible::Collapsible(const QString& title, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
    mWidget(nullptr),
    mHeaderHeight(30),
    mWidgetHeight(0),
    mOpened(false)
{
    mHeader = new CollapsibleHeader(this);
    setTitle(title);

    mAnimation = new QPropertyAnimation();
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);

    resize(100, mHeaderHeight);

    connect(mAnimation, &QPropertyAnimation::valueChanged, this, &Collapsible:: sendCollapsing);
}

Collapsible::~Collapsible()
{

}

void Collapsible::setWidget(QWidget* widget, int h)
{
    if (mWidget) {
        mWidget->setVisible(false);
        mWidget->setParent(nullptr);
    }
    mWidgetHeight = h;
    mWidget = widget;
    mWidget->setParent(this);
    mWidget->setVisible(true);

    updateLayout();
}

void Collapsible::setTitle(const QString& title)
{
    mHeader->setTitle(title);
}

void Collapsible::open(bool open)
{
    if (mWidget && mOpened != open) {
        mOpened = open;

        QRect startRect(x(), y(), width(), mHeaderHeight);
        QRect endRect = startRect.adjusted(0, 0, 0, mWidgetHeight);

        mAnimation->setStartValue(startRect);
        mAnimation->setEndValue(endRect);

        mAnimation->setDirection(mOpened ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
        mAnimation->start();

    } else {
        setFixedHeight(mHeaderHeight);
    }
}

bool Collapsible::opened() const
{
    return mOpened;
}

void Collapsible::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);

    p.setPen(QColor(100, 100, 100));
    p.setBrush(QColor(180, 180, 180));
    p.drawRect(rect());
}

void Collapsible::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void Collapsible::updateLayout()
{
    mHeader->setGeometry(0, 0, width(), mHeaderHeight);
    mWidget->setGeometry(0, mHeaderHeight, width(), mWidgetHeight);
}

void Collapsible::sendCollapsing(const QVariant& variant)
{
    emit collapsing(variant.toRect().height());
}
