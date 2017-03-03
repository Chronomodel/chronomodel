#include "Tabs.h"
#include "Painting.h"
#include <QtWidgets>


Tabs::Tabs(QWidget* parent):QWidget(parent),
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
}

int Tabs::currentIndex() const
{
    return mCurrentIndex;
}

void Tabs::setTab(int i, bool notify)
{
    mCurrentIndex = i;
    if (notify)
        emit tabClicked(i);
    update();
}
void Tabs::setFont(const QFont &font)
{
    updateLayout();
}

void Tabs::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());
    
    p.setPen(QColor(100, 100, 100));
    p.setBrush(QColor(200, 200, 200));
    
    for (int i=0; i<mTabNames.size(); ++i) {
        if (i != mCurrentIndex) {
            const QRectF r = mTabRects[i];
            p.drawRect(r);
            p.drawText(r, Qt::AlignCenter, mTabNames[i]);
        }
    }
    
    if (mCurrentIndex != -1) {
        p.setPen(Painting::mainColorLight);
        p.setBrush(Qt::white);
        
        p.drawRect(mTabRects[mCurrentIndex].adjusted(0, 0, 0, 2));
        p.drawText(mTabRects[mCurrentIndex], Qt::AlignCenter, mTabNames[mCurrentIndex]);
    }
}

void Tabs::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void Tabs::mousePressEvent(QMouseEvent* e)
{
    for (int i=0; i<mTabNames.size(); ++i) {
        if (i != mCurrentIndex && mTabRects[i].contains(e->pos()))
            setTab(i, true);
    }
}

void Tabs::updateLayout()
{
    qreal m = 10.;
    mTabRects.clear();
    qreal x = 1.;
    qreal h = height()-1;
    QFontMetrics metrics(font());

    for (auto && name : mTabNames) {
        const qreal w = metrics.width(name);
        qDebug()<<"Tabs::updateLayout()"<<w;
        mTabRects.append(QRectF(x, 1, 2*m + w, h));
        x += 2*m + w;
    }
    update();
}
