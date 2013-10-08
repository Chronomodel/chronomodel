#include "GraphView.h"
#include <QtGui>

void GraphView::setValues(const QVector<double>& aValues)
{
    mValues = aValues;
    repaintGraph(false);
}

void GraphView::drawContent(QPainter& painter)
{
    QPainterPath path;
    path.moveTo(0, height());
    for(int i=0; i<mValues.count(); ++i)
    {
        path.lineTo(mMarginLeft + i*mGraphWidth/mValues.count(), mMarginTop + mGraphHeight * (1-mValues.at(i)));
    }
    painter.setPen(QPen(QColor(100, 200, 240), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter.setRenderHints(QPainter::Antialiasing);
    painter.drawPath(path);
}





#pragma mark Constructor / Destructor

GraphView::GraphView(QWidget *parent):QWidget(parent),
mBackgroundColor(Qt::white),
mTitleColor(Qt::black),
 mAxisColor(QColor(50, 50, 50)),
mSubAxisColor(QColor(180, 180, 180)),
mValuesColor(Qt::black),
mSubValuesColor(Qt::black)
{
    mTitleFont.setPointSize(12);
    mAxisFont.setPointSize(11);

    setRangeX(-1000, 2000);
    setRangeY(0.f, 1.f);
    setXPrecision(0);
    setYPrecision(3);

    setXNumTicks(2);
    setXNumSubTicks(20);
    setXShowTicksValues(true);
    setXShowSubTicksValues(false);

    setYNumTicks(2);
    setYNumSubTicks(8);
    setYShowTicksValues(true);
    setYShowSubTicksValues(true);
}

GraphView::~GraphView(){}

void GraphView::setBackgroundColor(const QColor& aColor)
{
    mBackgroundColor = aColor;
    repaintGraph(true);
}
void GraphView::setTitleColor(const QColor& aColor)
{
    mTitleColor = aColor;
    repaintGraph(true);
}
void GraphView::setAxisColor(const QColor& aColor)
{
    mAxisColor = aColor;
    repaintGraph(true);
}
void GraphView::setSubAxisColor(const QColor& aColor)
{
    mSubAxisColor = aColor;
    repaintGraph(true);
}
void GraphView::setValuesColor(const QColor& aColor)
{
    mValuesColor = aColor;
    repaintGraph(true);
}
void GraphView::setSubValuesColor(const QColor& aColor)
{
    mSubValuesColor = aColor;
    repaintGraph(true);
}
void GraphView::setTitle(const QString& aTitle)
{
    mTitle = aTitle;
    repaintGraph(true);
}
void GraphView::setTitleFont(const QFont& aFont)
{
    mTitleFont = aFont;
    repaintGraph(true);
}
void GraphView::setAxisFont(const QFont& aFont)
{
    mAxisFont = aFont;
    repaintGraph(true);
}

void GraphView::repaintGraph(const bool aAlsoPaintBackground)
{
    if(aAlsoPaintBackground)
        mBufferedImage = QPixmap();
    update();
}

#pragma mark Overloads

void GraphView::resizeEvent(QResizeEvent*)
{
    mGraphWidth = width() - mMarginLeft - mMarginRight;
    mGraphHeight = height() - mMarginTop - mMarginBottom;
    repaintGraph(true);
}

void GraphView::paintEvent(QPaintEvent*)
{
    if(mBufferedImage.isNull())
    {
        mBufferedImage = QPixmap(width(), height());
        QPainter painter(&mBufferedImage);
        //painter.setRenderHints(QPainter::Antialiasing);

        painter.setFont(mAxisFont);
        drawBackground(painter);
        drawXAxis(painter);
        drawYAxis(painter);
        drawTitle(painter);
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, mBufferedImage);
    drawContent(painter);
}

#pragma mark Painting

void GraphView::drawBackground(QPainter& painter)
{
    painter.fillRect(0, 0, width(), height(), mBackgroundColor);
}
void GraphView::drawTitle(QPainter& painter)
{
    painter.setPen(mTitleColor);
    painter.setFont(mTitleFont);
    painter.drawText(QRect(mMarginLeft, 0, mGraphWidth, mMarginTop), Qt::AlignCenter, mTitle);
}
void GraphView::drawXAxis(QPainter& painter)
{
    painter.setFont(mAxisFont);
    const float lDeltaX = (float)mGraphWidth / ((float)mXNumTicks - 1.f);
    bool lIsMoreThanThousand = false;

    for(int i=0; i<mXNumTicks; ++i)
    {
        const int x = mMarginLeft + floor(i * lDeltaX + 0.5f);
        const float lValue = getValueForX(x);
        QString lUnitText;

        if(mXShowTicks)
        {
            painter.setPen(mAxisColor);
            painter.drawLine(x, mMarginTop, x, mMarginTop + mGraphHeight);
        }
        if(mXShowTicksValues)
        {
            float lDisplayValue = lValue;
            if(lDisplayValue >= 1000.f)
            {
                lDisplayValue /= 1000.f;
                if(!lIsMoreThanThousand)
                {
                    lUnitText = QString(" k") + QString(unitText(mXUnit));
                    lIsMoreThanThousand = true;
                }
            }
            else if(i == 0)
                lUnitText = QString(" ") + QString(unitText(mXUnit));
            QString lValueText = QString::number(lDisplayValue * mXFactor, 'g', mXPrecision);
            lValueText += lUnitText;

            painter.setPen(mValuesColor);
            painter.drawText(QRect(x - 20, mMarginTop + mGraphHeight + mXValueMargin, 40, 20), Qt::AlignCenter, lValueText);
        }

        if(i > 0)
        {
            const int lPrevX = mMarginLeft + floor((i-1.f) * lDeltaX + 0.5f);
            const float lPrevValue = getValueForX(lPrevX);
            const float lDeltaValue = lValue - lPrevValue;
            const float lSubDeltaValue = lDeltaValue / (mXNumSubTicks + 1.f);

            for(int j=1; j<=mXNumSubTicks; ++j)
            {
                const float lSubValue = lPrevValue + j*lSubDeltaValue;
                const float lSubX = getXForValue(lSubValue);

                if(mXShowSubTicks)
                {
                    painter.setPen(mSubAxisColor);
                    painter.drawLine(lSubX, mMarginTop, lSubX, mMarginTop + mGraphHeight);
                }
                if(mXShowSubTicksValues)
                {
                    painter.setPen(mSubValuesColor);
                    QString lSubValueText = mXPrecision == 0 ? QString((int)floor(lSubValue * mXFactor + 0.5f)) : QString::number(lSubValue * mXFactor, 'g', mXPrecision);
                    painter.drawText(QRect(lSubX - 20, mMarginTop + mGraphHeight + mXValueMargin, 40, 20), Qt::AlignCenter, lSubValueText);
                }
            }
        }
    }
}

void GraphView::drawYAxis(QPainter& painter)
{
    painter.setFont(mAxisFont);
    const float lDeltaY = (float)mGraphHeight / ((float)mYNumTicks - 1.f);
    bool lIsMoreThanThousand = false;

    for(int i=0; i<mYNumTicks; ++i)
    {
        const int y = mMarginTop + mGraphHeight - floor(i * lDeltaY + 0.5f);
        const float lValue = getValueForY(y);
        QString lUnitText;

        if(mYShowTicks)
        {
            painter.setPen(mAxisColor);
            painter.drawLine(mMarginLeft, y, mMarginLeft + mGraphWidth, y);
        }
        if(mYShowTicksValues)
        {
            float lDisplayValue = lValue;
            if(lDisplayValue >= 1000.f)
            {
                lDisplayValue /= 1000.f;
                if(!lIsMoreThanThousand)
                {
                    lUnitText = QString(" k") + QString(unitText(mYUnit));
                    lIsMoreThanThousand = true;
                }
            }
            else if(i == 0)
                lUnitText = QString(" ") + QString(unitText(mYUnit));
            QString lValueText = (mYPrecision == 0) ? QString((int)floor(lDisplayValue * mYFactor + 0.5f)) : QString::number(lDisplayValue * mYFactor, 'g', mYPrecision);
            lValueText += lUnitText;

            painter.setPen(mValuesColor);
            painter.drawText(QRect(0, y - 10, mMarginLeft - mYValueMargin, 20), Qt::AlignCenter, lValueText);
        }

        if(i > 0)
        {
            const int lPrevY = mMarginTop + mGraphHeight - floor((i-1.f) * lDeltaY + 0.5f);
            const float lPrevValue = getValueForY(lPrevY);
            const float lDeltaValue = lValue - lPrevValue;
            const float lSubDeltaValue = lDeltaValue / (mYNumSubTicks + 1.f);

            for(int j=1; j<=mYNumSubTicks; ++j)
            {
                const float lSubValue = lPrevValue + j*lSubDeltaValue;
                const float lSubY = getYForValue(lSubValue);

                if(mYShowSubTicks)
                {
                    painter.setPen(mSubAxisColor);
                    painter.drawLine(mMarginLeft, lSubY, mMarginLeft + mGraphWidth, lSubY);
                }
                if(mYShowSubTicksValues)
                {
                    painter.setPen(mSubValuesColor);
                    QString lSubValueText = mYPrecision == 0 ? QString((int)floor(lSubValue * mYFactor + 0.5f)) : QString(lSubValue * mYFactor, mYPrecision);
                    painter.drawText(QRect(0, lSubY - 10, mMarginLeft - mYValueMargin, 20), Qt::AlignCenter, lSubValueText);
                }
            }
        }
    }
}
