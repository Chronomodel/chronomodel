#include "Label.h"
#include "Painting.h"
#include <QtWidgets>


Label::Label(QWidget* parent):QLabel(parent)
{
    init();
}

Label::Label(const QString& text, QWidget* parent):QLabel(text, parent)
{
    init();
}

Label::~Label()
{

}

void Label::init()
{
    QFont f = font();
    f.setPointSize(pointSize(11));
    setFont(f);
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
}

void Label::setLight()
{
    QPalette palette = QLabel::palette();
    palette.setColor(QPalette::WindowText, QColor(200, 200, 200));
    setPalette(palette);
}

void Label::setDark()
{
    QPalette palette = QLabel::palette();
    palette.setColor(QPalette::Text, Qt::black);
    setPalette(palette);
}