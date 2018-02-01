#include "LineEdit.h"
#include "Painting.h"
#include <QFont>
#include <QtWidgets>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent)
{
    setParent(parent);
    //QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    setAlignment(Qt::AlignHCenter);
    setFont(parentWidget()->font());
}

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}


void LineEdit::setFont(const QFont& font)
{
    QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    QLineEdit::setStyleSheet(styleSh);
}

LineEdit::~LineEdit()
{

}
