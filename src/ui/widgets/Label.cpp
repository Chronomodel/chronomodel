#include "Label.h"
#include "Painting.h"
#include <QtWidgets>


Label::Label(QWidget* parent):QLabel(parent),
mIsTitle(false)
{
    init();
}

Label::Label(const QString& text, QWidget* parent):QLabel(text, parent),
mIsTitle(false)
{
    init();
}

Label::~Label()
{

}

void Label::init()
{
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPalette = this->parentWidget()->palette();
}

void Label::setPalette(QPalette &palette)
{
    mPalette = palette;
}

void Label::setBackground(QColor color)
{
    mPalette.setColor(QPalette::Background, color);
}

void Label::setIsTitle(bool isTitle)
{
    mIsTitle = isTitle;
    
    if (mIsTitle) {
        mPalette.setColor(QPalette::Text, Qt::white);
        mPalette.setColor(QPalette::Background, Painting::mainColorGrey);
        setAlignment(Qt::AlignCenter);
        setFixedHeight(20);
    }

}

void Label::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());
    const QRectF r = rect();

    p.fillRect(r, mPalette.background().color());
    p.setPen(mPalette.text().color());
    p.drawText(r, alignment(), text());

}


void Label::setLight()
{
    mPalette.setColor(QPalette::Text, QColor(200, 200, 200));
}

void Label::setDark()
{
    mPalette.setColor(QPalette::Text, Qt::black);
}
