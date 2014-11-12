#include "CalibrationView.h"
#include "Ruler.h"
#include "Date.h"
#include "Event.h"
#include "Marker.h"
#include "../PluginAbstract.h"
#include "../GraphViewRefAbstract.h"
#include "MainWindow.h"
#include "Project.h"
#include "GraphView.h"
#include "StdUtilities.h"
#include "Painting.h"
#include <QtWidgets>


CalibrationView::CalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mCalibGraph(0),
mRefGraphView(0)
{
    mRuler = new Ruler(this);
    mRuler->showScrollBar(true);
    mRuler->showControls(true);
    
    mCalibGraph = new GraphView(this);
    mCalibGraph->showYValues(true);
    mCalibGraph->showAxis(false);
    
    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);
    
    setMouseTracking(true);
    
    connect(mRuler, SIGNAL(zoomChanged(float, float)), mCalibGraph, SLOT(zoomX(float, float)));
}

CalibrationView::~CalibrationView()
{
    
}

void CalibrationView::setDate(const QJsonObject& date)
{
    mDate = date;
    updateGraphs();
}

void CalibrationView::updateGraphs()
{
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    
    QJsonObject settings = state[STATE_SETTINGS].toObject();
    ProjectSettings s = ProjectSettings::fromJson(settings);
    
    mCalibGraph->removeAllCurves();
    mRuler->setRange(s.mTmin, s.mTmax);
    mRuler->zoomDefault();
    
    // The current ref graph belongs to a plugin
    // So, we simply remove it without deleting it, for further use
    if(mRefGraphView)
    {
        disconnect(mRuler, SIGNAL(zoomChanged(float, float)), mRefGraphView, SLOT(zoomX(float, float)));
        mRefGraphView->setParent(0);
        mRefGraphView->setVisible(false);
    }
    
    Date date = Date::fromJson(mDate);
    if(!date.isNull())
    {
        date.calibrate(s.mTmin, s.mTmax, s.mStep);
        
        // ------------------------------------------------------------
        //  Calibration curve
        // ------------------------------------------------------------
        
        QColor c = Painting::mainColorLight;
        
        GraphCurve calibCurve;
        calibCurve.mName = "Calibration";
        calibCurve.mPen.setColor(c);
        calibCurve.mFillUnder = true;
        calibCurve.mData = date.mCalibration;
        
        float yMin = map_min_value(date.mCalibration);
        float yMax = map_max_value(date.mCalibration);
        
        ProjectSettings s = ProjectSettings::fromJson(project->state()[STATE_SETTINGS].toObject());
        
        mCalibGraph->setRangeX(s.mTmin, s.mTmax);
        mCalibGraph->setRangeY(yMin, yMax);
        mCalibGraph->addCurve(calibCurve);
        mCalibGraph->setVisible(true);
        
        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        
        // Get the ref graph for this plugin and this date
        mRefGraphView = date.mPlugin->getGraphViewRef();
        if(mRefGraphView)
        {
            mRefGraphView->setDate(date, s);
            mRefGraphView->setParent(this);
            mRefGraphView->setVisible(true);
            connect(mRuler, SIGNAL(zoomChanged(float, float)), mRefGraphView, SLOT(zoomX(float, float)));
        }
    }
    
    // Raise markers on top of recently added graphs
    mMarkerX->raise();
    mMarkerY->raise();
    
    updateLayout();
}

void CalibrationView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    int calibH = 200;
    int titleH = 30;
    int rulerH = 40;
    int graphLeft = 50.f;
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(100, 100, 100), 2*m));
    p.setBrush(Qt::white);
    p.drawRect(rect());
    
    p.setPen(Qt::black);
    QFont font = p.font();
    font.setPointSize(pointSize(14));
    p.setFont(font);
    
    QRectF title1Rect(m + graphLeft, m + rulerH, width() - 2*m - graphLeft, titleH);
    QRectF title2Rect(m + graphLeft, height() - m - calibH - titleH, width() - 2*m - graphLeft, titleH);
    
    if(mRefGraphView)
    {
        p.drawText(title1Rect, Qt::AlignVCenter | Qt::AlignLeft, tr("Calibration process") + " :");
        p.drawText(title2Rect, Qt::AlignVCenter | Qt::AlignLeft, tr("Distribution of calibrated date") + " :");
    }
    else
    {
        p.drawText(title1Rect, Qt::AlignVCenter | Qt::AlignLeft, tr("No calibration process to display") + " !");
        p.drawText(title2Rect, Qt::AlignVCenter | Qt::AlignLeft, tr("Typological date") + " :");
    }
}

void CalibrationView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void CalibrationView::updateLayout()
{
    int m = 5;
    int calibH = 200;
    int titleH = 30;
    int rulerH = 40;
    int graphLeft = 50.f;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    mRuler->setGeometry(m + graphLeft, m, width() - 2*m - graphLeft, rulerH);
    
    if(mRefGraphView)
    {
        mRefGraphView->setGeometry(m,
                                   m + rulerH + titleH,
                                   width() - 2*m,
                                   height() - 2*titleH - rulerH - 2*m - calibH);
    }
    mCalibGraph->setGeometry(m, height() - calibH - m, width() - 2*m, calibH);
    
    mMarkerX->setGeometry(mMarkerX->pos().x(), m + sbe, mMarkerX->thickness(), height() - 2*m - sbe - 8.f); // 8 = graph margin bottom
    mMarkerY->setGeometry(m + graphLeft, mMarkerY->pos().y(), width() - 2*m - graphLeft, mMarkerY->thickness());
    
    update();
}

void CalibrationView::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->pos().x()-2;
    x = (x < 0) ? 0 : x;
    x = (x > width()) ? width() : x;
    
    int y = e->pos().y()-2;
    y = (y < 0) ? 0 : y;
    y = (y > height()) ? height() : y;
    
    mMarkerX->setGeometry(x, mMarkerX->pos().y(), mMarkerX->width(), mMarkerX->height());
    mMarkerY->setGeometry(mMarkerY->pos().x(), y, mMarkerY->width(), mMarkerY->height());
}
