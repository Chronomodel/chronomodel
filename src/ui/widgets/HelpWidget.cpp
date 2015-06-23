#include "HelpWidget.h"
#include "Painting.h"
#include <QtWidgets>


HelpWidget::HelpWidget(QWidget* parent):QWidget(parent)
{
    construct();
}

HelpWidget::HelpWidget(const QString& text, QWidget* parent):QWidget(parent)
{
    construct();
    setText(text);
}

void HelpWidget::construct()
{
    mFont = font();
    mFont.setPointSize(pointSize(11));
    
    mHyperLink = new QLabel(this);
    mHyperLink->setTextFormat(Qt::RichText);
    mHyperLink->setFont(mFont);
    mHyperLink->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mHyperLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mHyperLink->setOpenExternalLinks(true);
    
    // Not yet supported with retina display in Qt 5.3
#ifndef Q_OS_MAC
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(4);
    shadow->setOffset(1, 1);
    setGraphicsEffect(shadow);
#endif
}

HelpWidget::~HelpWidget()
{
    
}

void HelpWidget::setText(const QString& text)
{
    mText = text;
    update();
}
void HelpWidget::setLink(const QString& url)
{
    mHyperLink->setText("<a href=\"" + url + "\">More...</a>");
}

int HelpWidget::heightForWidth(int w) const
{
    QFontMetrics metrics(mFont);
    QRect rect = metrics.boundingRect(QRect(0, 0, w - 10, 1000),
                                      Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft,
                                      mText);
    return rect.height() + 10 + 5 + 15; // 15 is the height of the link, and 5 its margin
}

void HelpWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QRectF r = rect().adjusted(1, 1, -1, -1);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(187, 174, 105));
    p.setBrush(QColor(242, 238, 184));
    p.drawRoundedRect(r, 4, 4);
    
    p.setPen(QColor(104, 74, 64));
    p.setFont(mFont);
    QTextOption options;
    options.setWrapMode(QTextOption::WordWrap);
    options.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    p.drawText(rect().adjusted(5, 5, -5, -5), mText, options);
}

void HelpWidget::resizeEvent(QResizeEvent*)
{
    mHyperLink->setGeometry(5, height() - 20, width() - 10, 15);
}
