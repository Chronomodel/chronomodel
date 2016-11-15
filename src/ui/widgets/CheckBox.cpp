#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>


CheckBox::CheckBox(QWidget* parent):QCheckBox(parent)
{
    setCursor(Qt::PointingHandCursor);
}

CheckBox::CheckBox(const QString& text, QWidget* parent):QCheckBox(text, parent)
{
    
}

CheckBox::~CheckBox()
{

}

void CheckBox::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    drawCheckbox(p, rect(), text(), checkState());
}

