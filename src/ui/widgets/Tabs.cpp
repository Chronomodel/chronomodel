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

#include "Tabs.h"
#include "Painting.h"

#include <QPainterPath>
#include <QMouseEvent>
#include <QApplication>

Tabs::Tabs(QWidget* parent):
    QWidget(parent),
    mTabHeight(40),
    mCurrentIndex(0)
{
    setFont(QFont());
}

Tabs::~Tabs()
{

}

void Tabs::addTab(const QString& name)
{
    mTabNames.append(name);
    if (mTabNames.size() == 1)
        mCurrentIndex = 0;

    mTabWidgets.append(nullptr);
    mTabVisible.append(true);
    mTabIds.append(mTabIds.size());
}

void Tabs::addTab(const QString& name, qsizetype identifier)
{
    mTabNames.append(name);
    if (mTabNames.size() == 1)
        mCurrentIndex = 0;

    mTabWidgets.append(nullptr);
    mTabVisible.append(true);
    mTabIds.append(identifier);
}

void Tabs::addTab(QWidget* wid, const QString& name)
{
    mTabNames.append(name);
    if (mTabNames.size() == 1)
        mCurrentIndex = 0;

    mTabWidgets.append(wid);
    wid->setParent(this);
    for (auto& widChild : wid->children())
        widChild->setParent(this);


    mTabVisible.append(true);
    mTabIds.append(mTabIds.size());
}

QWidget* Tabs::getWidget(const qsizetype &i)
{
    if (i<mTabWidgets.size())
        return mTabWidgets[i];
    else
        return nullptr;
}

QWidget* Tabs::getCurrentWidget()
{
     return getWidget(mCurrentIndex);
}

QRect Tabs::widgetRect()
{
    const QWidget* w = getCurrentWidget();
    return (w != nullptr) ? w->rect() : QRect(0, mTabHeight, width(), 0);
}

/**
 * @brief Tabs::geometry return the position within its parent and the dimension of the current widget
 * @return
 */
QRect Tabs::minimalGeometry() const
{
    const int xMaxTab (mTabRects.at(mTabRects.size()-1).x() + mTabRects.at(mTabRects.size()-1).width());

    const int w =  mTabWidgets.at(mCurrentIndex) ? std::max(xMaxTab, mTabWidgets.at(mCurrentIndex)->rect().width()) : xMaxTab;
    const int h =  mTabHeight + (mTabWidgets[mCurrentIndex] ? mTabWidgets[mCurrentIndex]->rect().height() : 0);

    return QRect(pos().x(), pos().y(),  w, h);
}

int Tabs::minimalHeight() const
{
    const int h =  mTabHeight + (mTabWidgets.at(mCurrentIndex) ? mTabWidgets.at(mCurrentIndex)->rect().height() : 0);
    return h;
}

int Tabs::minimalWidth() const
{
    const int xMaxTab = !mTabRects.isEmpty() ? (mTabRects.last().x() + mTabRects.last().width() ) : 10;

    const int w = mTabWidgets.at(mCurrentIndex) ? std::max(xMaxTab, mTabWidgets.at(mCurrentIndex)->rect().width()) : xMaxTab;

    return  w;
}

QSize Tabs::sizeHint() const
{
    if (mTabNames.isEmpty())
        return QSize(2*mTabHeight, mTabHeight);
    else
        return QSize(minimalWidth(), minimalHeight() );
}


qsizetype Tabs::currentIndex() const
{
    return mCurrentIndex;
}

int Tabs::currentId() const
{
    return (int)mTabIds.at(mCurrentIndex);
}

QString Tabs::currentName() const
{
    return mTabNames.at(mCurrentIndex);
}

void Tabs::showWidget(const qsizetype &i)
{
    Q_ASSERT(i<mTabWidgets.size());

    qsizetype j = 0;
    for (auto && w:mTabWidgets) {
        if (j!= i)
            w->setVisible(false);
        else
            w->setVisible(true);
        ++j;
    }
    update();
}


void Tabs::setTab(const qsizetype &index, bool notify)
{
    Q_ASSERT (index < mTabNames.size());
    if (index != mCurrentIndex) {
        mCurrentIndex = index;
        // we start to hide all widget and after we show the current widget, because if there is the same widget used in the several
        for (auto&& wid : mTabWidgets)
            if (wid) {
                wid->setVisible(false);
                for (auto& widChild : wid->children()) {
                    if (widChild->isWidgetType())
                        static_cast<QWidget*>(widChild)-> setVisible(false);
                }
            }

        // tabs we have to show again
        if (mTabWidgets.at(mCurrentIndex)) {
            mTabWidgets[mCurrentIndex]->setVisible(true);
            for (auto& widChild : mTabWidgets.at(mCurrentIndex)->children())
                if (widChild->isWidgetType())
                    static_cast<QWidget*>(widChild)-> setVisible(true);
        }

        if (notify)
            emit tabClicked(index);

        updateLayout();
    }
}

void Tabs::setTabId(const qsizetype id, bool notify)
{
    qsizetype idx = mTabIds.indexOf(id);
    setTab(idx, notify);
}

void Tabs::setFont(const QFont &font)
{
    QWidget::setFont(font);
    updateLayout();
}

void Tabs::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    const qreal ray = 5;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    for (qsizetype i = 0; i<mTabNames.size(); ++i) {
        QColor backColor = (i == mCurrentIndex) ? Painting::mainColorDark : Qt::black;
        QColor frontColor = (i == mCurrentIndex) ? Qt::white : QColor(200, 200, 200);
        if (mTabVisible.at(i)) {

            const QRectF r = mTabRects.at(i);
            QPainterPath pth;
            /*
             * // 1 rounded corner at left
              pth.moveTo(r.x(), r.y() + r.height());
              pth.lineTo(r.x(), r.y() + ray);

              pth.arcTo(r.x() , r.y(), 2*ray, 2*ray, -180, -90);
              pth.lineTo(r.x() + r.width()  , r.y() );

              pth.lineTo(r.x() + r.width(), r.y() + r.height() );
              pth.closeSubpath();
             */

            // 2 rounded corners
            pth.moveTo(r.x(), r.y() + r.height());
            pth.lineTo(r.x(), r.y() + ray);

            pth.arcTo(r.x() , r.y(), 2*ray, 2*ray, -180, -90);
            pth.lineTo(r.x() + r.width() - ray , r.y() );

            pth.arcTo(r.x() + r.width() -2*ray , r.y(), 2*ray, 2*ray, 90, -90);
            pth.lineTo(r.x() + r.width(), r.y() + r.height() );
            pth.closeSubpath();

            p.fillPath(pth, backColor);

            p.setPen(frontColor);
            p.drawText(r, Qt::AlignCenter, mTabNames.at(i));
       }

    }

    p.setPen(Painting::mainColorDark);
    p.drawLine(0, mTabHeight, width(), mTabHeight);
}

void Tabs::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void Tabs::mousePressEvent(QMouseEvent* e)
{
    for (qsizetype i=0; i<mTabNames.size(); ++i)
        if (i != mCurrentIndex && mTabRects.at(i).contains(e->pos()))
            setTab(i, true);
}

/**
 * @brief Create mTabRects which allows the drawing of tabs
 */
void Tabs::updateLayout()
{
    mTabRects.clear();
    qreal x = 1.;
    const qreal h = mTabHeight - 1.;
    QFontMetrics fm (font());
    const qreal m = 1.0 * fm.lineSpacing();
    int i = 0;
    for (QString& name : mTabNames) {
        if (mTabVisible.at(i)) {
            const qreal w = fm.horizontalAdvance(name);
            mTabRects.append(QRectF(x + i, 1,  2*m + w, h));
            x += 2*m + w;
        } else {
            mTabRects.append(QRectF(x+ i, 1, 0, h));
        }

        ++i;
    }
    QWidget* wi = getCurrentWidget();
    if (wi != nullptr) {
        wi->setGeometry(0, mTabHeight, width(), height() - mTabHeight);
    }

    update();
}
