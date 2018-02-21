#include "CheckBox.h"
#include "Painting.h"
#include <QtWidgets>


CheckBox::CheckBox(QWidget* parent):QCheckBox(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFont(parentWidget()->font());
}

CheckBox::CheckBox(const QString& text, QWidget* parent):QCheckBox(text, parent)
{
    setCursor(Qt::PointingHandCursor);
    setFont(parentWidget()->font());
}

CheckBox::~CheckBox()
{

}

void CheckBox::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.setFont(parentWidget()->font());
    drawCheckbox(p, rect(), text(), checkState());
}

