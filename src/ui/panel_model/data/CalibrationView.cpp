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
#include "CheckBox.h"
#include "LineEdit.h"
#include "Button.h"
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
    
    mCalibGraph->setRendering(GraphView::eHD);
    mCalibGraph->setYAxisMode(GraphView::eMinMax);
    mCalibGraph->setXAxisMode(GraphView::eAllTicks);
    
    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);
    
    mResultsLab = new QLabel(this);
    mResultsLab->setWordWrap(true);
    QFont font;
    font.setPointSizeF(pointSize(9.));
    mResultsLab->setFont(font);
    
    mHPDCheck = new CheckBox(tr("HPD (%)"), this);
    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");
    
    mHPDCheck->raise();
    mHPDEdit->raise();
    
    setMouseTracking(true);
    
    connect(mRuler, SIGNAL(zoomChanged(double, double)), mCalibGraph, SLOT(zoomX(double, double)));
    
    connect(mHPDCheck, SIGNAL(toggled(bool)), this, SLOT(updateGraphs()));
    connect(mHPDEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateGraphs()));
}

CalibrationView::~CalibrationView()
{
    
}

void CalibrationView::setDate(const QJsonObject& date)
{
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    QJsonObject settings = state[STATE_SETTINGS].toObject();
    mSettings = ProjectSettings::fromJson(settings);
    
    mDate = Date::fromJson(date);
    
    if(!mDate.isNull())
    {
        mDate.calibrate(mSettings);
        
        mRuler->setRange(mSettings.mTmin, mSettings.mTmax);
        mRuler->zoomDefault();
        mCalibGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    }
    updateGraphs();
}

void CalibrationView::updateGraphs()
{
    mCalibGraph->removeAllCurves();
    
    // The current ref graph belongs to a plugin
    // So, we simply remove it without deleting it, for further use
    if(mRefGraphView)
    {
        disconnect(mRuler, SIGNAL(zoomChanged(double, double)), mRefGraphView, SLOT(zoomX(double, double)));
        mRefGraphView->setParent(0);
        mRefGraphView->setVisible(false);
    }
    
    if(!mDate.isNull())
    {
        DensityAnalysis results;
        results.analysis = analyseFunction(mDate.getCalibMap());
        results.quartiles = quartilesForRepartition(mDate.mRepartition, mSettings.mTmin, mSettings.mStep);
        mResultsLab->setText(densityAnalysisToString(results));
        
        // ------------------------------------------------------------
        //  Calibration curve
        // ------------------------------------------------------------
        
        QColor c = Painting::mainColorLight;
        
        GraphCurve calibCurve;
        calibCurve.mName = "Calibration";
        calibCurve.mPen.setColor(c);
        calibCurve.mFillUnder = false;
        calibCurve.mIsHisto = false;
        calibCurve.mData = mDate.getCalibMap();
        
        double yMax = map_max_value(calibCurve.mData);
        yMax = (yMax > 0) ? yMax : 1;
        mCalibGraph->setRangeY(0, 1.1f * yMax);
        
        mCalibGraph->addCurve(calibCurve);
        mCalibGraph->setVisible(true);
        
        if(mHPDCheck->isChecked())
        {
            QMap<double, double> hpd = create_HPD(calibCurve.mData, 1, mHPDEdit->text().toDouble());
            
            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen.setColor(c);
            hpdCurve.mFillUnder = true;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            mCalibGraph->addCurve(hpdCurve);
            mResultsLab->setText(mResultsLab->text() + "HPD : " + getHPDText(hpd));
        }
        
        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        
        // Get the ref graph for this plugin and this date
        mRefGraphView = mDate.mPlugin->getGraphViewRef();
        if(mRefGraphView)
        {
            mRefGraphView->setDate(mDate, mSettings);
            mRefGraphView->setParent(this);
            mRefGraphView->setVisible(true);
            connect(mRuler, SIGNAL(zoomChanged(double, double)), mRefGraphView, SLOT(zoomX(double, double)));
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
    int lineH = 25;
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
    
    QRectF title1Rect(m + graphLeft, 2*m + lineH + rulerH, width() - 2*m - graphLeft, titleH);
    QRectF title2Rect(m + graphLeft, height() - 3*m - calibH - titleH - 2*lineH, width() - 2*m - graphLeft, titleH);
    
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
    int lineH = 25;
    int editW = 30;
    int checkW = 70;
    int rulerH = 40;
    int graphLeft = 50.f;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    mRuler->setGeometry(m + graphLeft, 2*m + lineH, width() - 3*m - graphLeft, rulerH);
    
    if(mRefGraphView)
    {
        mRefGraphView->setGeometry(m,
                                   2*m + rulerH + titleH + lineH,
                                   width() - 3*m,
                                   height() - 2*titleH - rulerH - 5*m - calibH - 3*lineH);
    }
    mCalibGraph->setGeometry(m, height() - calibH - 3*m - 2*lineH, width() - 3*m, calibH);
    mResultsLab->setGeometry(m + graphLeft, height() - 2*m - 2*lineH, width() - 3*m, 2*lineH);
    
    mHPDEdit->setGeometry(width() - 2*m - editW, height() - 3*m - 2*lineH, editW, lineH);
    mHPDCheck->setGeometry(width() - 3*m - editW - checkW, height() - 3*m - 2*lineH, checkW, lineH);
    
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
