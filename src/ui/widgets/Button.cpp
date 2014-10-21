#include "Button.h"
#include "Painting.h"
#include <QtWidgets>


Button::Button(QWidget* parent):QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
}

Button::Button(const QString& text, QWidget* parent):QPushButton(text, parent)
{
    
}

Button::~Button()
{

}

void Button::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    drawButton(p, rect(), isChecked() || isDown(), isEnabled(), text(), icon(), isFlat());
}

