#include "LineEdit.h"
#include "Painting.h"
#include <QFont>
#include <QtWidgets>
//#include <QFont>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent)
{
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    setAlignment(Qt::AlignHCenter);
}

void LineEdit::setFont(const QFont& font)
{
    QWidget::setFont(font);
}

LineEdit::~LineEdit()
{

}
