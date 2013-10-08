#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "GraphViewAbstract.h"
#include <QWidget>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPixmap>


class GraphView: public QWidget, public GraphViewAbstract
{
    Q_OBJECT
public:
    explicit GraphView(QWidget *parent = 0);
    virtual ~GraphView();

    void setBackgroundColor(const QColor& aColor);
    void setTitleColor(const QColor& aColor);
    void setAxisColor(const QColor& aColor);
    void setSubAxisColor(const QColor& aColor);
    void setValuesColor(const QColor& aColor);
    void setSubValuesColor(const QColor& aColor);
    void setTitle(const QString& aTitle);
    void setTitleFont(const QFont& aFont);
    void setAxisFont(const QFont& aFont);

    void setValues(const QVector<double>& aValues);

protected:
    void repaintGraph(const bool aAlsoPaintBackground);

    virtual void resizeEvent(QResizeEvent* aEvent);
    virtual void paintEvent(QPaintEvent* aEvent);

    virtual void drawBackground(QPainter& painter);
    virtual void drawTitle(QPainter& painter);
    virtual void drawXAxis(QPainter& painter);
    virtual void drawYAxis(QPainter& painter);

    // Overload this one to do the curve drawing
    virtual void drawContent(QPainter& painter);

protected:
    QPixmap	mBufferedImage;
    QColor	mBackgroundColor;
    QColor	mTitleColor;
    QColor	mAxisColor;
    QColor	mSubAxisColor;
    QColor	mValuesColor;
    QColor	mSubValuesColor;
    QString	mTitle;
    QFont	mTitleFont;
    QFont	mAxisFont;

    QVector<double> mValues;
};

#endif // GRAPHVIEW_H
