#include "Label.h"
#include "Painting.h"
#include <QtWidgets>


Label::Label(QWidget* parent):QLabel(parent),
mIsTitle(false),
mAdjustText(true)
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
void Label::adjustFont()
{
    if (!text().isEmpty() && mAdjustText) {
        const QFontMetrics fm (qApp->font());
        const QRect textRect = fm.boundingRect(text());
        const qreal wR = width() - 10;
        const qreal xfactor (textRect.width()> wR ? textRect.width()/wR : 1);
        const qreal yfactor (textRect.height()>height() ? textRect.height()/height() : 1) ;
        const qreal factor  = ( xfactor > yfactor ? xfactor : yfactor);
        QFont ft = qApp->font();
        ft.setPointSizeF(ft.pointSizeF()/factor);
        setFont(ft);
    }
}

void Label::init()
{
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPalette = parentWidget()->palette();
    setFont(qApp->font());
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
     }

}

void Label::setAdjustText(bool ad)
{
    mAdjustText = ad;
}
void Label::resizeEvent(QResizeEvent* e)
{
    (void) e;
    adjustFont();

}

void Label::paintEvent(QPaintEvent*)
{
   // adjustFont();
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
