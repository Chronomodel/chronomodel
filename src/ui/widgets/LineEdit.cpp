#include "LineEdit.h"
#include "Painting.h"
#include <QFont>
#include <QtWidgets>


LineEdit::LineEdit(QWidget* parent):QLineEdit(parent),
  mAdjustText(true)
{
    setParent(parent);
    setAlignment(Qt::AlignHCenter);
    setFont(parentWidget()->font());
}

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}

void LineEdit::setAdjustText(bool ad)
{
    mAdjustText = ad;
}

void LineEdit::adjustFont()
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

void LineEdit::resizeEvent(QResizeEvent* e)
{
    (void) e;
    adjustFont();

}

void LineEdit::setFont(const QFont& font)
{
#ifdef Q_OS_MAC
    QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    QLineEdit::setStyleSheet(styleSh);
#else
//#ifdef Q_OS_WIN
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    QLineEdit::setFont(font);
 #endif
}

LineEdit::~LineEdit()
{

}
