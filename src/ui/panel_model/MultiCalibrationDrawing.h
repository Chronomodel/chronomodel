#ifndef MULTICALIBRATIONDRAWING_H
#define MULTICALIBRATIONDRAWING_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QScrollArea>

#include "GraphView.h"
#include "Marker.h"

class ColoredBar: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    ColoredBar(QWidget *parent = nullptr);
    ~ColoredBar();
    void setColor(const QColor & color) {mColor = color;}
    QColor color() const {return mColor;}
static int mWidth;
protected:
    void paintEvent(QPaintEvent* e);

private:
    QColor mColor;

};

class MultiCalibrationDrawing: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int graphHeight READ graphHeight WRITE setGraphHeight)
public:
    MultiCalibrationDrawing(QWidget *parent = nullptr);
    ~MultiCalibrationDrawing();
    virtual QPixmap grab();

    void setGraphList(QList<GraphView*> &list);
    void setEventsColorList(QList<QColor> &colorList);
    QList<GraphView*> *getGraphList() {return &mListCalibGraph;}
    void updateLayout();
    void forceRefresh();
    void setGraphHeight(const int & height);
    int graphHeight() const {return mGraphHeight;}

    void hideMarker();
    void showMarker();

    QWidget* getGraphWidget() const {return mGraphWidget;}

private:

    QList<GraphView*> mListCalibGraph;
    QList<QColor> mListEventsColor;
    QList<ColoredBar*> mListBar;

    QScrollArea* mScrollArea;
    QWidget* mGraphWidget;

    Marker* mMarkerX;

    int mVerticalSpacer;

    int mGraphHeight;

    QFont mGraphFont;

    bool mMouseOverCurve;

protected:
  //  void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

};

#endif // MULTICALIBRATIONDRAWING_H
