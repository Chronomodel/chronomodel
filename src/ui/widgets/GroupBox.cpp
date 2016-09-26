#include "GroupBox.h"
#include <QtWidgets>

int GroupBox::sTitleHeight = 30;

GroupBox::GroupBox(const QString& title, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mLineH(20),
mTitle(title)
{
    
}

GroupBox::~GroupBox()
{
    
}

void GroupBox::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    
    p.setPen(QColor(100, 100, 100));
    p.setBrush(QColor(180, 180, 180));
    p.drawRect(rect());
    
    p.fillRect(0, 0, width(), sTitleHeight, QColor(100, 100, 100));
    p.setPen(QColor(200, 200, 200));
    p.drawText(mMargin, mMargin, width() - 2*mMargin, sTitleHeight - 2*mMargin, Qt::AlignVCenter | Qt::AlignLeft, mTitle);
}
