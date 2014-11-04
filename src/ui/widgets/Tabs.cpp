#include "Tabs.h"
#include "Painting.h"
#include <QtWidgets>


Tabs::Tabs(QWidget* parent):QWidget(parent),
mCurrentIndex(-1)
{
    mFont.setPointSizeF(pointSize(11));
}

Tabs::~Tabs()
{
    
}

void Tabs::addTab(const QString& name)
{
    mTabNames.append(name);
    if(mTabNames.size() == 1)
        mCurrentIndex = 0;
}

int Tabs::currentIndex() const
{
    return mCurrentIndex;
}

void Tabs::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(mFont);
    
    p.fillRect(rect(), QColor(200, 200, 200));
    p.setPen(QColor(100, 100, 100));
    p.setBrush(QColor(200, 200, 200));
    
    for(int i=0; i<mTabNames.size(); ++i)
    {
        if(i != mCurrentIndex)
        {
            QRectF r = mTabRects[i];
            p.drawRect(r);
            p.drawText(r, Qt::AlignCenter, mTabNames[i]);
        }
    }
    
    if(mCurrentIndex != -1)
    {
        p.setPen(mainColorLight);
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
    for(int i=0; i<mTabNames.size(); ++i)
    {
        if(i != mCurrentIndex && mTabRects[i].contains(e->pos()))
        {
            emit tabClicked(i);
            mCurrentIndex = i;
            update();
        }
    }
}

void Tabs::updateLayout()
{
    float m = 10.f;
    mTabRects.clear();
    float x = 1.f;
    float h = height()-1;
    QFontMetrics metrics(mFont);
    
    for(int i=0; i<mTabNames.size(); ++i)
    {
        float w = metrics.width(mTabNames[i]);
        mTabRects.append(QRectF(x, 1, 2*m + w, h));
        x += 2*m + w;
    }
    update();
}
