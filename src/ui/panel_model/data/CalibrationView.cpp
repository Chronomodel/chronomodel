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
#include "Label.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "DoubleValidator.h"
#include <QtWidgets>
#include <QClipboard>
//#include <QStringBuilder>


CalibrationView::CalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mCalibGraph(0),
mRefGraphView(0)
{
    mCalibGraph = new GraphView(this);
    
    mCalibGraph->setRendering(GraphView::eHD);
    mCalibGraph->setYAxisMode(GraphView::eHidden);
    mCalibGraph->setXAxisMode(GraphView::eAllTicks);
    
    mMarkerX = new Marker(this);
    mMarkerY = new Marker(this);
    
    mTopLab = new Label(tr(""), this);
    mProcessTitle = new Label(tr(""), this);
    mDistribTitle = new Label(tr(""), this);
    
    mTopLab->setAlignment(Qt::AlignCenter);
    mProcessTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mDistribTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    QFont titleFont;
    titleFont.setPointSizeF(pointSize(12.));
    titleFont.setBold(true);
    mProcessTitle->setFont(titleFont);
    mDistribTitle->setFont(titleFont);
    titleFont.setPointSizeF(pointSize(16.));
    mTopLab->setFont(titleFont);
    
    mResultsLab = new QLabel(this);
    mResultsLab->setWordWrap(true);
    QFont font;
    font.setPointSizeF(pointSize(12.));
    mResultsLab->setFont(font);
    
    mZoomLab = new Label(tr("Scale") + " : ", this);
    mZoomSlider = new QSlider(Qt::Horizontal, this);
    mZoomSlider->setRange(0, 100);
    mZoomSlider->setValue(0);
    mZoomSlider->setSingleStep(1);
    
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    
    mHPDLab = new Label(tr("HPD (%) : "), this);
    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    mHPDLab->raise();
    mHPDEdit->raise();
    
    mExportPlotBut = new Button(tr("Export Image"), this);
    mCopyTextBut = new Button(tr("Copy Text"), this);
    setMouseTracking(true);
    
    connect(mZoomSlider, SIGNAL(valueChanged(int)), this, SLOT(updateZoom()));
    connect(mScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateScroll()));
    connect(mHPDEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateGraphs()));
    connect(mExportPlotBut, SIGNAL(clicked()), this, SLOT(exportImage()));
    connect(mCopyTextBut, SIGNAL(clicked()), this, SLOT(copyText()));
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
    
    try{
        mDate = Date::fromJson(date);
        mDate.autoSetTiSampler(false);
        if(!mDate.isNull())
        {
            mDate.calibrate(mSettings);
            mTopLab->setText(mDate.getName() + " (" + mDate.mPlugin->getName() + ")");
            mCalibGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            mCalibGraph->setCurrentX(mSettings.mTmin, mSettings.mTmax);
            mZoomSlider->setValue(0);
        }
        updateGraphs();
    }
    catch(QString error){
        QMessageBox message(QMessageBox::Critical,
                            qApp->applicationName() + " " + qApp->applicationVersion(),
                            tr("Error : ") + error,
                            QMessageBox::Ok,
                            qApp->activeWindow(),
                            Qt::Sheet);
        message.exec();
    }
}

void CalibrationView::updateGraphs()
{
    mCalibGraph->removeAllCurves();
    QLocale locale;
    
    // The current ref graph belongs to a plugin
    // So, we simply remove it without deleting it, for further use
    if(mRefGraphView)
    {
        mRefGraphView->setParent(0);
        mRefGraphView->setVisible(false);
    }
    
    
    if(!mDate.isNull())
    {
        DensityAnalysis results;
        results.analysis = analyseFunction(mDate.getCalibMap());
        results.quartiles = quartilesForRepartition(mDate.mRepartition, mSettings.mTmin, mSettings.mStep);
        //mResultsLab->setText(densityAnalysisToString(results));
        QString resultsStr = densityAnalysisToString(results);
        // ------------------------------------------------------------
        //  Calibration curve
        // ------------------------------------------------------------
        QColor penColor = Painting::mainColorDark; //Painting::mainColorLight;
        QColor brushColor = Painting::mainColorLight; //mDate.mPlugin->getColor();
        
        GraphCurve calibCurve;
        calibCurve.mName = "Calibration";
        calibCurve.mPen.setColor(penColor);
        calibCurve.mIsHisto = false;
        calibCurve.mData = mDate.getCalibMap();
        
        // Fill under distrib.of calibrated date only if typo ref :
        bool isTypo = (mDate.mPlugin->getName() == "Typo Ref.");
        calibCurve.mIsRectFromZero = isTypo;
        calibCurve.mBrush = isTypo ? QBrush(brushColor) : QBrush(Qt::NoBrush);
        
        mHPDLab->setVisible(!isTypo);
        mHPDEdit->setVisible(!isTypo);
        
        double yMax = map_max_value(calibCurve.mData);
        yMax = (yMax > 0) ? yMax : 1;
        mCalibGraph->setRangeY(0, 1.1f * yMax);
        mCalibGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
        mCalibGraph->setCurrentX(mSettings.mTmin, mSettings.mTmax);
        
        mCalibGraph->addCurve(calibCurve);
        mCalibGraph->setVisible(true);
        
        // Fill HPD only if not typo ref. :
        if(!isTypo)
        {
            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);
            
            double thresh = mHPDEdit->text().toDouble();
            thresh = qMin(thresh, 100.);
            thresh = qMax(thresh, 0.);
            
            QMap<double, double> hpd = create_HPD(calibCurve.mData, thresh);
            
            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen = penColor;
            hpdCurve.mBrush = brushColor;
            hpdCurve.mIsHisto = false;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            mCalibGraph->addCurve(hpdCurve);
            
            yMax = map_max_value(hpdCurve.mData);
            mCalibGraph->setRangeY(0, qMax(1.1f * yMax, mCalibGraph->maximumY()));
            
            mCalibGraph->mLegendX = DateUtils::getAppSettingsFormat();
            mCalibGraph->setFormatFunctX(DateUtils::convertToAppSettingsFormatStr);
            mCalibGraph->setFormatFunctY(formatValueToAppSettingsPrecision);
            
            double realThresh = map_area(hpd) / map_area(calibCurve.mData);
            //mResultsLab->setText(mResultsLab->text() % "HPD (" % locale.toString(100. * realThresh, 'f', 1) + "%) : " % getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormat(), DateUtils::convertToAppSettingsFormatStr)); //  % concatenation with QStringBuilder
            resultsStr += + "<br> HPD (" + locale.toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormat(), DateUtils::convertToAppSettingsFormatStr);
            mResultsLab->setWordWrap(true);
            mResultsLab->setText(resultsStr);
            
            
            //mResultsLab->setText(mResultsLab->text() + "\n HPD (" + locale.toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormat(), DateUtils::convertToAppSettingsFormatStr));
        }
        
        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        
        // Get the ref graph for this plugin and this date
        mRefGraphView = mDate.mPlugin->getGraphViewRef();
        if(mRefGraphView)
        {
            mRefGraphView->setFormatFunctX(DateUtils::convertToAppSettingsFormatStr); // must be before setDate, because setDate use it
            mRefGraphView->setDate(mDate, mSettings);
            
            mRefGraphView->setParent(this);
            mRefGraphView->setVisible(true);
        }
        
        // ------------------------------------------------------------
        //  Labels
        // ------------------------------------------------------------
        if(mRefGraphView)
        {
            mProcessTitle->setText(tr("Calibration process") + " :");
            mDistribTitle->setText(tr("Distribution of calibrated date") + " :");
        }
        else
        {
            mProcessTitle->setText(tr("No calibration process to display") + " !");
            mDistribTitle->setText(tr("Typological date") + " :");
        }
    }
    
    // Raise markers on top of recently added graphs
    mMarkerX->raise();
    mMarkerY->raise();
    
    updateLayout();
}

void CalibrationView::updateZoom()
{
    float min = mCalibGraph->minimumX();
    float max = mCalibGraph->maximumX();
    float minProp = 5 / (max - min);
    float prop = (100. - mZoomSlider->value()) / 100.;
    if(prop < minProp) prop = minProp;
    
    if(prop != 1)
    {
        // Remember old scroll position
        double posProp = 0;
        double rangeBefore = (double)mScrollBar->maximum();
        if(rangeBefore > 0)
            posProp = (double)mScrollBar->value() / rangeBefore;
        
        // Update Scroll Range
        int fullScrollSteps = 1000;
        int scrollSteps = (1.f - prop) * fullScrollSteps;
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);

        // Set scroll to correct position
        double pos = 0;
        double rangeAfter = (double)mScrollBar->maximum();
        if(rangeAfter > 0)
            pos = floor(posProp * rangeAfter);
        mScrollBar->setValue(pos);
    }
    else
    {
        mScrollBar->setRange(0, 0);
    }
    updateScroll();
}

void CalibrationView::updateScroll()
{
    double min = mCalibGraph->minimumX();
    double max = mCalibGraph->maximumX();
    double minProp = 5 / (max - min);
    double prop = (100. - mZoomSlider->value()) / 100.;
    if(prop < minProp) prop = minProp;
    
    if(prop != 1)
    {
        // Update graphs with new zoom
        //double delta = prop * (max - min);
        double delta = prop * (max - min);
        double deltaStart = (max - min) - delta;
        double start = min + deltaStart * ((double)mScrollBar->value() / (double)mScrollBar->maximum()) ;
        start = (floor(start)<min ? min : floor(start));
        double end = start + delta;
        end = (ceil(end)>max ? max : ceil(end));
        mCalibGraph->zoomX(start, end);
        if(mRefGraphView)
            mRefGraphView->zoomX(start, end);
    }
    else
    {
        mCalibGraph->zoomX(min, max);
        if(mRefGraphView)
            mRefGraphView->zoomX(min, max);
    }
}

void CalibrationView::exportImage()
{
    mExportPlotBut -> setVisible(false);
    mCopyTextBut   -> setVisible(false);
    mZoomLab       -> setVisible(false);
    mZoomSlider    -> setVisible(false);
    mScrollBar     -> setVisible(false);
    mHPDEdit       -> setVisible(false);
    mHPDLab        -> setVisible(false);
    mMarkerX       -> setVisible(false);
    mMarkerY       -> setVisible(false);
    mResultsLab    -> setVisible(false);
    
    int m = 5;
    QRect r(m, m, this->width() - 2*m, this->height() - 2*m);
    
    QFileInfo fileInfo = saveWidgetAsImage(this, r, tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    mExportPlotBut -> setVisible(true);
    mCopyTextBut   -> setVisible(true);
    mZoomLab       -> setVisible(true);
    mZoomSlider    -> setVisible(true);
    mScrollBar     -> setVisible(true);
    mHPDEdit       -> setVisible(true);
    mHPDLab        -> setVisible(true);
    mMarkerX       -> setVisible(true);
    mMarkerY       -> setVisible(true);
    mResultsLab    -> setVisible(true);
}
void CalibrationView::copyText()
{
     QClipboard *p_Clipboard = QApplication::clipboard();
    //str.remove(QRegExp("<[^>]*>"));
    //p_Clipboard->setText(mResultsLab->text().simplified());
    /*QTextDocument doc;
    doc.setHtml( mResultsLab->text() );
    p_Clipboard->setText(doc.toPlainText());
     */
    p_Clipboard->setText(mResultsLab->text().replace("<br>", "\n"));
   
}

void CalibrationView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(100, 100, 100), 2*m));
    p.setBrush(Qt::white);
    p.drawRect(rect());
}

void CalibrationView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void CalibrationView::updateLayout()
{
    int m1 = 5;
    int m2 = 10;
    
    int w = width() - 2*m2;
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    int topH = 40;
    int botH = 60;
    int calibH = 200;
    int titleH = 30;
    int refH = height() - calibH - 2*titleH - topH - botH - sbe - 2*m2;
    
    int lineH = 20;
    int editW = 30;
    int checkW = 70;
    //int rulerH = 40;
    int graphLeft = 50;
    int sliderW = 100;
    int zoomLabW = 60;
    int butW = 100;
    int butH = 25;
    
    mExportPlotBut->setGeometry(width() - m2 - butW, m1 + (topH - butH)/2, butW, butH);
    mCopyTextBut->setGeometry(width() - m2 - 10 - 2* butW, m1 + (topH - butH)/2, butW, butH);
    
    mTopLab->setGeometry(m2, m1, w, topH);
    mScrollBar->setGeometry(m2 + graphLeft, m1 + topH, w - graphLeft - zoomLabW - sliderW - m1, sbe);
    mZoomLab->setGeometry(width() - m2 - sliderW - m1 - zoomLabW, m1 + topH, zoomLabW, lineH);
    mZoomSlider->setGeometry(width() - m2 - sliderW, m1 + topH, sliderW, lineH);
    
    mProcessTitle->setGeometry(m2 + graphLeft, m1 + topH + sbe, w - graphLeft, titleH);
    if(mRefGraphView)
        mRefGraphView->setGeometry(m2+1, m1 + topH + sbe + titleH, w, refH);
    mDistribTitle->setGeometry(m2 + graphLeft, m1 + topH + sbe + titleH + refH, w - graphLeft, titleH);
    mCalibGraph->setGeometry(m2+1, m1 + topH + sbe + 2*titleH + refH, w, calibH);
    
    mResultsLab->setGeometry(m2 + graphLeft, height() - m2 - botH, w, botH);
    
    mHPDEdit->setGeometry(width() - m2 - 10 - editW, height() - m2 - botH, editW, lineH);
    mHPDLab->setGeometry(width() - m2 - 10 - editW - checkW, height() - m2 - botH, checkW, lineH);
    
    mMarkerX->setGeometry(mMarkerX->pos().x(), m1 + sbe, mMarkerX->thickness(), height() - 2*m1 - sbe - 8.f); // 8 = graph margin bottom
    mMarkerY->setGeometry(m1 + graphLeft, mMarkerY->pos().y(), width() - 2*m1 - graphLeft, mMarkerY->thickness());
    
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
    // draw the red cross lines
    mMarkerX->setGeometry(x, mMarkerX->pos().y(), mMarkerX->width(), mMarkerX->height());
    mMarkerY->setGeometry(mMarkerY->pos().x(), y, mMarkerY->width(), mMarkerY->height());
}
