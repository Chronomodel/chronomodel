#include "Collapsible.h"
#include <QtWidgets>


CollapsibleHeader::CollapsibleHeader(Collapsible* collapsible, Qt::WindowFlags flags):QWidget(collapsible, flags),
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
    
    p.fillRect(rect(), QColor(100, 100, 100));
    
    QRect r1(0, 0, height(), height());
    int dr = 7;
    r1.adjust(dr, dr, -dr, -dr);
    QRect r2(height(), 0, width() - height(), height());
    
    p.setPen(QColor(200, 200, 200));
    p.setBrush(QColor(200, 200, 200));
    p.drawText(r2, Qt::AlignLeft | Qt::AlignVCenter, mTitle);
    
    QPainterPath path;
    int ds = 1;
    if(mCollapsible->opened())
    {
        path.moveTo(r1.x() + ds, r1.y());
        path.lineTo(r1.x() + r1.width() - ds, r1.y());
        path.lineTo(r1.x() + r1.width()/2, r1.y() + r1.height());
    }
    else
    {
        path.moveTo(r1.x(), r1.y() + ds);
        path.lineTo(r1.x(), r1.y() + r1.height() - ds);
        path.lineTo(r1.x() + r1.width(), r1.y() + r1.height()/2);
    }
    p.fillPath(path, QColor(200, 200, 200));
}

void CollapsibleHeader::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if(mCollapsible)
        mCollapsible->open(!mCollapsible->opened());
}


Collapsible::Collapsible(const QString& title, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mWidget(0),
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
    if(mWidget)
    {
        mWidget->setVisible(false);
        mWidget->setParent(0);
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
    if(mWidget && mOpened != open)
    {
        mOpened = open;
        
        QRect startRect(x(), y(), width(), mHeaderHeight);
        QRect endRect = startRect.adjusted(0, 0, 0, mWidgetHeight);
        
        mAnimation->setStartValue(startRect);
        mAnimation->setEndValue(endRect);
        
        mAnimation->setDirection(mOpened ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
        mAnimation->start();
    }
    else
    {
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
