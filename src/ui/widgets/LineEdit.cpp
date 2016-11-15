#include "LineEdit.h"
#include "Painting.h"
#include <QtWidgets>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent)
{
    QFont f = font();
    f.setPointSize(pointSize(11));
    setFont(f);
}

LineEdit::~LineEdit()
{

}
