#ifndef CalibrationView_H
#define CalibrationView_H

#include "Date.h"
#include "ProjectSettings.h"
#include <QWidget>
#include <QJsonObject>

class GraphView;
class GraphViewRefAbstract;
class Date;
class Marker;
class LineEdit;
class Button;
class QTextEdit;
class QLabel;
class Label;
class QSlider;
class QScrollBar;


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
    void updateZoom();
    void updateScroll();
    void exportImage();
    
signals:
    void closed();
    
public:
    Date mDate;
    ProjectSettings mSettings;
    
    GraphView* mCalibGraph;
    GraphViewRefAbstract* mRefGraphView;
    
    Marker* mMarkerX;
    Marker* mMarkerY;
    
    Label* mTopLab;
    Label* mProcessTitle;
    Label* mDistribTitle;
    
    Label* mZoomLab;
    QSlider* mZoomSlider;
    QScrollBar* mScrollBar;
    Label* mHPDLab;
    LineEdit* mHPDEdit;
    QLabel* mResultsLab;
    Button* mExportBut;
};

#endif
