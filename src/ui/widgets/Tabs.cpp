/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "Tabs.h"
#include "Painting.h"

Tabs::Tabs(QWidget* parent):QWidget(parent),
mTabHeight(40),
mCurrentIndex(0)
{
    setFont(QFont());
 //   setFixedHeight(mTabHeight);

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

void Tabs::addTab(const QString& name, int identifier)
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

QWidget* Tabs::getWidget(const int &i)
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
    int xMaxTab (mTabRects.at(mTabRects.size()-1).x() + mTabRects.at(mTabRects.size()-1).width());

    int w =  mTabWidgets.at(mCurrentIndex) ? std::max(xMaxTab, mTabWidgets.at(mCurrentIndex)->rect().width()) : xMaxTab;
    int h =  mTabHeight + (mTabWidgets[mCurrentIndex] ? mTabWidgets[mCurrentIndex]->rect().height() : 0);

    return QRect(pos().x(), pos().y(),  w, h);
}

int Tabs::minimalHeight() const
{
    const int h =  mTabHeight + (mTabWidgets.at(mCurrentIndex) ? mTabWidgets.at(mCurrentIndex)->rect().height() : 0);
    return h;
}

int Tabs::minimalWidth() const
{
    const int xMaxTab = !mTabRects.isEmpty() ? (mTabRects.at(mTabRects.size()-1).x() + mTabRects.at(mTabRects.size()-1).width()) : 10;

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


int Tabs::currentIndex() const
{
    return mCurrentIndex;
}

int Tabs::currentId() const
{
    return mTabIds.at(mCurrentIndex);
}

QString Tabs::currentName() const
{
    return mTabNames.at(mCurrentIndex);
}

void Tabs::showWidget(const int &i)
{
    Q_ASSERT(i<mTabWidgets.size());

    int j(0);
    for (auto &&w:mTabWidgets) {
        if (j!= i)
            w->setVisible(false);
        else
            w->setVisible(true);
        ++j;
    }
    update();
}


void Tabs::setTab(const int &i, bool notify)
{
    Q_ASSERT (i < mTabNames.size());
    if (i != mCurrentIndex) {
        mCurrentIndex = i;
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
            emit tabClicked(i);

        updateLayout();
    }
}

void Tabs::setTabId(const int identifier, bool notify)
{
    int idx = mTabIds.indexOf(identifier);
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

    for (int i = 0; i<mTabNames.size(); ++i) {
        QColor backColor = (i == mCurrentIndex) ? Painting::mainColorDark : Qt::black;
        QColor frontColor = (i == mCurrentIndex) ? Qt::white : QColor(200, 200, 200);
        
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
        p.drawText(r, Qt::AlignCenter, mTabNames[i]);

    }

    p.setPen(Painting::mainColorDark);
    p.drawLine(0, mTabHeight, width(), mTabHeight);
}

void Tabs::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void Tabs::mousePressEvent(QMouseEvent* e)
{
    for (int i=0; i<mTabNames.size(); ++i)
        if (i != mCurrentIndex && mTabRects.at(i).contains(e->pos()))
            setTab(i, true);
}

void Tabs::updateLayout()
{
    mTabRects.clear();
    qreal x = 1.;
    const qreal h = mTabHeight - 1.;
    QFontMetrics fm(font());

    int i = 0;
    for (QString& name : mTabNames) {
        if (mTabVisible.at(i)) {
            const qreal w = fm.boundingRect(name).width();
            const qreal m = 1.5 * fm.boundingRect(QString("H")).width();
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
