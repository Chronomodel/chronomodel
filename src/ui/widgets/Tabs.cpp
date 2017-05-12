#include "Tabs.h"
#include "Painting.h"
#include <QtWidgets>


Tabs::Tabs(QWidget* parent):QWidget(parent),
mTabHeight(30),
mCurrentIndex(-1)
{

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
}

void Tabs::addTab(QWidget* wid, const QString& name)
{
    mTabNames.append(name);
    if (mTabNames.size() == 1)
        mCurrentIndex = 0;

    mTabWidgets.append(wid);
    wid->setParent(this);
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
     return mTabWidgets[mCurrentIndex];
}

QRect Tabs::widgetRect() const
{
    return mTabWidgets[mCurrentIndex]->rect();
}

/**
 * @brief Tabs::geometry return the position within its parent and the dimension of the current widget
 * @return
 */
QRect Tabs::minimalGeometry() const
{
    int xMaxTab (mTabRects[mTabRects.size()-1].x() + mTabRects[mTabRects.size()-1].width());

    int w =  mTabWidgets[mCurrentIndex] ? std::max(xMaxTab, mTabWidgets[mCurrentIndex]->rect().width()) : xMaxTab;
    int h =  mTabHeight + (mTabWidgets[mCurrentIndex] ? mTabWidgets[mCurrentIndex]->rect().height() : 0);

    return QRect(pos().x(), pos().y(),  w, h);
}

int Tabs::minimalHeight() const
{
    const int h =  mTabHeight + (mTabWidgets[mCurrentIndex] ? mTabWidgets[mCurrentIndex]->rect().height() : 0);
    return h;
}

int Tabs::minimalWidth() const
{
    const int xMaxTab = !mTabRects.isEmpty() ? (mTabRects[mTabRects.size()-1].x() + mTabRects[mTabRects.size()-1].width()) : 10;

    const int w =  mTabWidgets[mCurrentIndex] ? std::max(xMaxTab, mTabWidgets[mCurrentIndex]->rect().width()) : xMaxTab;

    return  w;
}

int Tabs::currentIndex() const
{
    return mCurrentIndex;
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

}


void Tabs::setTab(int i, bool notify)
{
    Q_ASSERT (i<mTabNames.size());

    mCurrentIndex = i;

    //resize(width(), height());

    if (notify)
        emit tabClicked(i);
    update();
}

void Tabs::setFont(const QFont &font)
{
    (void) font;
    updateLayout();
}

void Tabs::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    if (mTabWidgets[mCurrentIndex])
        resize(width(), height());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());

    for (int i=0; i<mTabNames.size(); ++i)
        if (i != mCurrentIndex) {
            const QRectF r = mTabRects[i];
            p.fillRect(r, Qt::black);
            p.setPen(QColor(200, 200, 200));
            p.drawText(r, Qt::AlignCenter, mTabNames[i]);
        }
    
    if (mCurrentIndex != -1) {
        p.fillRect(mTabRects[mCurrentIndex], Painting::mainColorDark);
        p.setPen(Qt::white);//QColor(200, 200, 200));
        p.drawText(mTabRects[mCurrentIndex], Qt::AlignCenter, mTabNames[mCurrentIndex]);
    }

    p.setPen(Painting::mainColorDark);
    if (mTabWidgets[mCurrentIndex]) {
        const int x0 = (width() - mTabWidgets[mCurrentIndex]->width() )/2;
//qDebug()<<"Tabs: x0"<<x0;
        mTabWidgets[mCurrentIndex]->move(x0, mTabHeight);
        p.drawRoundRect(mTabWidgets[mCurrentIndex]->geometry(), 2, 2);

    } else {
        p.drawLine(0, mTabHeight - p.pen().width(), mTabRects[mCurrentIndex].x(), mTabHeight - p.pen().width());
        p.drawLine(mTabRects[mCurrentIndex].x() + mTabRects[mCurrentIndex].width(), mTabHeight - p.pen().width(),width() , mTabHeight - p.pen().width());
    }
}

void Tabs::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void Tabs::mousePressEvent(QMouseEvent* e)
{
    for (int i=0; i<mTabNames.size(); ++i)
        if (i != mCurrentIndex && mTabRects[i].contains(e->pos()))
            setTab(i, true);
}

void Tabs::updateLayout()
{
    const qreal m = 10.;
    mTabRects.clear();
    qreal x = 1.;
    const qreal h = mTabHeight - 1.;
    QFontMetrics metrics(font());

    for (auto && name : mTabNames) {
        const qreal w = metrics.width(name);
        mTabRects.append(QRectF(x, 1, 2*m + w, h));
        x += 2*m + w;
    }
    update();
}
