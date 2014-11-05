#ifndef CalibrationView_H
#define CalibrationView_H

#include <QWidget>
#include <QJsonObject>

class GraphView;
class GraphViewRefAbstract;
class Date;
class Marker;
class Ruler;


class CalibrationView: public QWidget
{
    Q_OBJECT
public:
    CalibrationView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~CalibrationView();
    
    void setDate(const QJsonObject& date);
    
protected:
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private slots:
    void updateGraphs();
    
public:
    QJsonObject mDate;
    
    Ruler* mRuler;
    GraphView* mCalibGraph;
    GraphViewRefAbstract* mRefGraphView;
    
    Marker* mMarkerX;
    Marker* mMarkerY;
};

#endif
