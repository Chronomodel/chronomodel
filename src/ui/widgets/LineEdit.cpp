#include "LineEdit.h"
#include "Painting.h"
#include <QtWidgets>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent)
{
    /*QFont f = font();
    f.setPointSize(pointSize(11));
    setFont(f);*/
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    setAlignment(Qt::AlignHCenter);
}

void LineEdit::setFont(const QFont& font)
{
    QWidget::setFont(font);
}

LineEdit::~LineEdit()
{

}
