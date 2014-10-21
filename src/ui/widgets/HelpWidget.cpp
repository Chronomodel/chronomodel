#include "HelpWidget.h"
#include "Painting.h"
#include <QtWidgets>


HelpWidget::HelpWidget(QWidget* parent):QWidget(parent)
{
    mFont = font();
    mFont.setPointSize(pointSize(11));
}

HelpWidget::HelpWidget(const QString& text, QWidget* parent):QWidget(parent),
mText(text)
{
    mFont = font();
    mFont.setPointSize(pointSize(11));
}

HelpWidget::~HelpWidget()
{
    
}

void HelpWidget::setText(const QString& text)
{
    mText = text;
    update();
}

int HelpWidget::heightForWidth(int w) const
{
    QFontMetrics metrics(mFont);
    QRect rect = metrics.boundingRect(QRect(0, 0, w - 10, 1000),
                                      Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft,
                                      mText);
    return rect.height() + 10;
}

void HelpWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(187, 174, 105));
    p.setBrush(QColor(242, 238, 184));
    p.drawRoundedRect(rect(), 4, 4);
    
    p.setPen(QColor(104, 74, 64));
    p.setFont(mFont);
    QTextOption options;
    options.setWrapMode(QTextOption::WordWrap);
    options.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    p.drawText(rect().adjusted(5, 5, -5, -5), mText, options);
}
