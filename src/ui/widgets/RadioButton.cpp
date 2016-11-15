#include "RadioButton.h"
#include "Painting.h"
#include <QtWidgets>


RadioButton::RadioButton(QWidget* parent):QRadioButton(parent)
{
    setCursor(Qt::PointingHandCursor);
}

RadioButton::RadioButton(const QString& text, QWidget* parent):QRadioButton(text, parent)
{
    
}

RadioButton::~RadioButton()
{

}

void RadioButton::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    drawRadio(p, rect(), text(), isChecked());
}

