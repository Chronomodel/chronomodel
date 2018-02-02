#include "LineEdit.h"
#include "Painting.h"
#include <QFont>
#include <QtWidgets>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent)
{
    setParent(parent);
    setAlignment(Qt::AlignHCenter);
    setFont(parentWidget()->font());
}

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}


void LineEdit::setFont(const QFont& font)
{
 //#ifdef Q_OS_MAC
    //QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    //QLineEdit::setStyleSheet(styleSh);
//#endif

//#ifdef Q_OS_WIN
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    QLineEdit::setFont(font);
// #endif
}

LineEdit::~LineEdit()
{

}
