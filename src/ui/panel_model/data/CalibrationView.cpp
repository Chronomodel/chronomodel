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
    
    mZoomLab = new Label(tr("Scale"), this);
    mZoomSlider = new QSlider(Qt::Horizontal, this);
    mZoomSlider->setRange(0, 100);
    mZoomSlider->setValue(0);
    mZoomSlider->setSingleStep(1);
    
    mScrollBar = new QScrollBar(Qt::Horizontal, this);
    mScrollBar->setRange(0, 0);
    
    mHPDLab = new Label(tr("HPD (%)"), this);
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
    mCopyTextBut = new Button(tr("Stat. to clipboard"), this);
    setMouseTracking(true);
    
    connect(mZoomSlider, &QSlider::valueChanged, this, &CalibrationView::updateZoom);
    connect(mScrollBar, &QScrollBar::valueChanged, this, &CalibrationView::updateScroll);
    connect(mHPDEdit, &LineEdit::textEdited, this, &CalibrationView::updateGraphs);
    connect(mExportPlotBut, &Button::clicked, this, &CalibrationView::exportImage);
    connect(mCopyTextBut, &Button::clicked, this, &CalibrationView::copyText);
}

CalibrationView::~CalibrationView()
{
    
}

void CalibrationView::setDate(const QJsonObject& date)
{
    if(date.isEmpty())
        return;
    Project* project = MainWindow::getInstance()->getProject();
    const QJsonObject state = project->state();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    mSettings = ProjectSettings::fromJson(settings);

    try {
        mDate = Date::fromJson(date);
        mDate.autoSetTiSampler(false);
        qDebug()<<"CalibrationView::setDate "<<mDate.mCalibration->mName;
      /*  if (!mDate.isNull() ) {
            if (mDate.mCalibration->mCurve.isEmpty())
                mDate.calibrate(mSettings, project);
      */
            mTopLab->setText(mDate.mName + " (" + mDate.mPlugin->getName() + ")");
      //  }
        updateGraphs();
    }
    catch(QString error) {
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
    mCalibGraph->removeAllZones();
    
    const QLocale locale;
    
    // The current ref graph belongs in memory to a plugin
    // So, we simply remove it without deleting it, for further use
    if (mRefGraphView) {
        mRefGraphView->setParent(0);
        mRefGraphView->setVisible(false);
    }
    
    
    if (!mDate.isNull()) {
        //double tminCalib = mDate.getTminCalib();
        //double tmaxCalib = mDate.getTmaxCalib();
        
        const double t1 ( mSettings.getTminFormated() );
        const double t2 ( mSettings.getTmaxFormated() );
        const double t3 ( mDate.getFormatedTminCalib() );
        const double t4 ( mDate.getFormatedTmaxCalib() );

        double tminDisplay = qMin(t1,t3);
        double tmaxDisplay = qMax(t2,t4);
       

        mCalibGraph->setRangeX(tminDisplay, tmaxDisplay);
        mCalibGraph->setCurrentX(tminDisplay, tmaxDisplay);
        
        mZoomSlider->setValue(0);
        
        // ------------------------------------------------------------
        //  Show zones if calibrated data are outside study period
        // ------------------------------------------------------------
        if (tminDisplay < mSettings.getTminFormated()) {
            GraphZone zone;
            zone.mXStart = tminDisplay;
            zone.mXEnd = mSettings.getTminFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }
        if (tmaxDisplay > mSettings.getTmaxFormated()) {
            GraphZone zone;
            zone.mXStart = mSettings.getTmaxFormated();
            zone.mXEnd = tmaxDisplay;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }
        
        // ------------------------------------------------------------
        //  Calibration curve
        // ------------------------------------------------------------
        QColor penColor = Painting::mainColorDark;
        QColor brushColor = Painting::mainColorLight;
        brushColor.setAlpha(100);

        // Fill under distrib. of calibrated date only if typo :
        const bool isTypo = (mDate.mPlugin->getName() == "Typo");
        mHPDLab->setVisible(!isTypo);
        mHPDEdit->setVisible(!isTypo);

        // Fill HPD only if not typo :
        mResultsLab->clear();
        QMap<double, double> calibMap = mDate.getFormatedCalibMap();

        if (!calibMap.isEmpty()) {
            // ------------------------------------------------------------
            //  Display numerical results
            // ------------------------------------------------------------
            QString resultsStr;
            
            DensityAnalysis results;
            results.analysis = analyseFunction(calibMap);

            results.quartiles = quartilesForRepartition(mDate.getFormatedRepartition(), mDate.getFormatedTminCalib(), mSettings.mStep);


            resultsStr = densityAnalysisToString(results);
            
            GraphCurve calibCurve;
            calibCurve.mName = "Calibration";
            calibCurve.mPen.setColor(penColor);
            calibCurve.mIsHisto = false;
            calibCurve.mData = calibMap;
            calibCurve.mIsRectFromZero = isTypo;
            calibCurve.mBrush = isTypo ? QBrush(brushColor) : QBrush(Qt::NoBrush);

            type_data yMax = map_max_value(calibCurve.mData);
            yMax = (yMax > 0) ? yMax : 1;
            mCalibGraph->setRangeY(0, 1.1 * yMax);

            mCalibGraph->addCurve(calibCurve);
            mCalibGraph->setVisible(true);

            mCalibGraph->setTipXLab("t");

            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);
            
            const double thresh = qBound(0., mHPDEdit->text().toDouble(), 100.);

            QMap<type_data, type_data> hpd = create_HPD(calibCurve.mData, thresh);
            
            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen = penColor;
            hpdCurve.mBrush = brushColor;
            hpdCurve.mIsHisto = false;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            mCalibGraph->addCurve(hpdCurve);
            
            yMax = map_max_value(hpdCurve.mData);

            mCalibGraph->setRangeY(0, 1.1 * yMax);

            mCalibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mCalibGraph->setFormatFunctX(formatValueToAppSettingsPrecision);
            mCalibGraph->setFormatFunctY(formatValueToAppSettingsPrecision);
            
            double realThresh = map_area(hpd) / map_area(calibCurve.mData);
            
            resultsStr += + "<br> HPD (" + locale.toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), DateUtils::dateToString);
            
            mResultsLab->setWordWrap(true);
            mResultsLab->setText(resultsStr);
            
        }

        
        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        
        // Get the ref graph for this plugin and this date
        mRefGraphView = mDate.mPlugin->getGraphViewRef();
        if (mRefGraphView) {
            mRefGraphView->setFormatFunctX(DateUtils::dateToString); // must be before setDate, because setDate use it
            mRefGraphView->setDate(mDate, mSettings);
            
            mRefGraphView->setParent(this);
            mRefGraphView->setVisible(true);
        }
        
        // ------------------------------------------------------------
        //  Labels
        // ------------------------------------------------------------
        if (mRefGraphView) {
            mProcessTitle->setText(tr("Calibration process") );
            mDistribTitle->setText(tr("Distribution of calibrated date"));
        } else {
            mProcessTitle->setText(tr("No calibration process to display") + " !");
            mDistribTitle->setText(tr("Typological date"));
        }
    }
    
    // Raise markers on top of recently added graphs
    mMarkerX->raise();
    mMarkerY->raise();
    
    updateLayout();
}

void CalibrationView::updateZoom()
{
    type_data min = mCalibGraph->minimumX();
    type_data max = mCalibGraph->maximumX();
    type_data minProp = 5. / (max - min);
    type_data prop = (100. - (type_data)mZoomSlider->value()) / 100.;
    if (prop < minProp)
        prop = minProp;
    
    if (prop != 1) {
        // Remember old scroll position
        type_data posProp (0);
        type_data rangeBefore = (type_data)mScrollBar->maximum();
        if(rangeBefore > 0)
            posProp = (type_data)mScrollBar->value() / rangeBefore;
        
        // Update Scroll Range
        int fullScrollSteps = 1000;
        int scrollSteps = (int) ((1. - prop)) * fullScrollSteps;
        mScrollBar->setRange(0, scrollSteps);
        mScrollBar->setPageStep(fullScrollSteps);

        // Set scroll to correct position
        type_data pos (0.);
        type_data rangeAfter = (type_data)mScrollBar->maximum();
        if(rangeAfter > 0)
            pos = floor(posProp * rangeAfter);
        mScrollBar->setValue(pos);
        
    } else
        mScrollBar->setRange(0, 0);
    
    updateScroll();
}

void CalibrationView::updateScroll()
{
    const type_data min = mCalibGraph->minimumX();
    const type_data max = mCalibGraph->maximumX();
    const type_data minProp = 5.f / (max - min);
    type_data prop = (100. - (type_data) mZoomSlider->value()) / 100.;
    if (prop < minProp)
        prop = minProp;
    
    if (prop != 1) {
        // Update graphs with new zoom
        const type_data delta = prop * (max - min);
        const type_data deltaStart = (max - min) - delta;
        type_data start = min + deltaStart * ((type_data)mScrollBar->value() / (type_data)mScrollBar->maximum()) ;
        start = (floor(start)<min ? min : floor(start));
        type_data end = start + delta;
        end = (ceil(end)>max ? max : ceil(end));
        mCalibGraph->zoomX(start, end);
        if(mRefGraphView)
            mRefGraphView->zoomX(start, end);
        
    } else {
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
    QString text = mTopLab->text() +"<br>" + mDate.getDesc() + "<br>" + mResultsLab->text();
    p_Clipboard->setText(text.replace("<br>", "\r"));
   
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

    QFontMetrics fm(qApp->font());

    //const float min = mCalibGraph->minimumX();
    const type_data max = mCalibGraph->maximumX();
    //const int marginLeft = (int)floor(fm.width(locale().toString(min))/2);
    const int marginRight = (int)floor(fm.width(locale().toString(max))/2);

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
    if(mRefGraphView) {
        mRefGraphView->setGeometry(m2+1, m1 + topH + sbe + titleH, w, refH);
        mRefGraphView->setMarginRight(marginRight);
    }
    mDistribTitle->setGeometry(m2 + graphLeft, m1 + topH + sbe + titleH + refH, w - graphLeft, titleH);
    mCalibGraph->setGeometry(m2+1, m1 + topH + sbe + 2*titleH + refH, w, calibH);
    mCalibGraph->setMarginRight(marginRight);

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

