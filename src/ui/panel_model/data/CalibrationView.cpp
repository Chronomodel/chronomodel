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
mCalibGraph(nullptr),
mRefGraphView(nullptr),
mMinimalButtonWidth(10),
mButtonHeight(50),
mResultsHeight(90)
{
    QFont font = parent->font();
    QFontMetrics fm (font);

    setButtonWidth(20 * fm.width("_"));

    mImageSaveBut = new Button(tr("Save"), this);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));

    mImageClipBut = new Button(tr("Copy"), this);
    mImageClipBut->setIcon(QIcon(":picture_copy.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));

    mResultsClipBut = new Button(tr("Copy"), this);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));

    mDataSaveBut = new Button(tr("Save"), this);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));

    frameSeparator = new QFrame(this);
    frameSeparator->setFrameStyle(QFrame::Panel);
    frameSeparator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    frameSeparator->setStyleSheet("QFrame { border: 5px solid rgb(49, 112, 176); }");//same color as Painting::mainColorLight = QColor(49, 112, 176);


    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAlignment(Qt::AlignHCenter);
    mHPDLab->setLight();

    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");
  //  mHPDEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
  //  mHPDEdit->setAlignment(Qt::AlignHCenter);
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(3);
    mHPDEdit->setValidator(percentValidator);

    mStartLab = new Label(tr("Start"), this);
    mStartLab->setAlignment(Qt::AlignHCenter);
    mStartLab->setLight();

    mStartEdit = new LineEdit(this);
    mStartEdit->setText("-1000");
    //mStartEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    //mStartEdit->setAlignment(Qt::AlignHCenter);

    mEndLab = new Label(tr("End"), this);
    mEndLab->setAlignment(Qt::AlignHCenter);
    mEndLab->setLight();

    mEndEdit = new LineEdit(this);
    mEndEdit->setText("1000");
    //mEndEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    //mEndEdit->setAlignment(Qt::AlignHCenter);



    mHPDLab->raise();
    mHPDEdit->raise();


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
    
   /* mResultsLab = new QLabel(this);
    mResultsLab->setWordWrap(true);
    QFont resultFont;
    resultFont.setPointSizeF(pointSize(12.));
    mResultsLab->setFont(resultFont);
   */
    mResultsText = new QTextEdit(this);
    mResultsText->setFrameStyle(QFrame::HLine);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    mResultsText->setPalette(palette);
    //QFont Resultfont = mResultsLab->font();
    //Resultfont.setPointSizeF(pointSize(11));
    //mResultsLab->setFont(Resultfont);
    mResultsText->setText(tr("Nothing to display"));
    //mResultsLab->setVisible(false);
    mResultsText->setReadOnly(true);
    

    
   // mExportPlotBut = new Button(tr("Export Image"), this);
   // mCopyTextBut = new Button(tr("Stat. to clipboard"), this);
    setMouseTracking(true);

    //Position of Widget
  //  int y (0);
    // the button has the same size of the plugin button in EventProperties.cpp
  //  int buttonHeight (50);
   // int buttonWidth (80);
/*
    mImageSaveBut->setGeometry(0, y, mButtonSize, buttonHeight);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(0, y, mButtonSize, buttonHeight);
    y += mImageClipBut->height();
    mResultsClipBut->setGeometry(0, y, mButtonSize, buttonHeight);
    y += mResultsClipBut->height();
    mDataSaveBut->setGeometry(0, y, mButtonSize, buttonHeight);
    y += mDataSaveBut->height();


    frameSeparator->setGeometry(0, y, mButtonSize, 5);

    y += horizontalLine->height() + 5;
    mHPDLab->setGeometry(0, y, mButtonSize, fm.height()+3);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(3, y, mButtonSize-6, fm.height()+3);
    y += mHPDEdit->height() + 5;
    mStartLab->setGeometry(0, y, mButtonSize, fm.height()+3);
    y += mStartLab->height();
    mStartEdit->setGeometry(3, y, mButtonSize-6, fm.height()+3);
    y += mStartEdit->height() + 5;
    mEndLab->setGeometry(0, y, mButtonSize, fm.height()+3);
    y += mEndLab->height();
    mEndEdit->setGeometry(3, y, mButtonSize-6, fm.height()+3);
*/
    // Connection
   // mImageSaveBut->setEnabled(true);
    //connect(mZoomSlider, &QSlider::valueChanged, this, &CalibrationView::updateZoom);
    //connect(mScrollBar, &QScrollBar::valueChanged, this, &CalibrationView::updateScroll);
    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CalibrationView::updateScroll);
    connect(mHPDEdit, &QLineEdit::textEdited, this, &CalibrationView::updateGraphs);
    connect(mImageSaveBut, &Button::clicked, this, &CalibrationView::exportImage);
    connect(mResultsClipBut, &Button::clicked, this, &CalibrationView::copyText);

}

CalibrationView::~CalibrationView()
{
    
}

void CalibrationView::setButtonWidth(const qreal& width)
{
    mButtonWidth = width>mMinimalButtonWidth ? width : mMinimalButtonWidth;

    repaint();
}

void CalibrationView::setFont(const QFont &font)
{
    QFontMetrics fm (font);

    setButtonWidth(10 * fm.width("_"));
    // we must force setFont on QLineEdi !!
    mHPDEdit->setFont(font);
    mStartEdit->setFont(font);
    mEndEdit->setFont(font);
    repaint();
}

void CalibrationView::setDate(const QJsonObject& date)
{
    if (date.isEmpty())
        return;
    Project* project = MainWindow::getInstance()->getProject();
    const QJsonObject state = project->state();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    mSettings = ProjectSettings::fromJson(settings);

    try {
        mDate = Date::fromJson(date);
        mDate.autoSetTiSampler(false);
        mTopLab->setText(mDate.mName + " (" + mDate.mPlugin->getName() + ")");

        const double t1 ( mSettings.getTminFormated() );
        const double t2 ( mSettings.getTmaxFormated() );
        const double t3 ( mDate.getFormatedTminCalib() );
        const double t4 ( mDate.getFormatedTmaxCalib() );

        mTminDisplay = qMin(t1,t3);
        mTmaxDisplay = qMax(t2,t4);

        mStartEdit->setText(stringWithAppSettings(mTminDisplay, false));
        mEndEdit->setText(stringWithAppSettings(mTmaxDisplay, false));


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
        mRefGraphView->setParent(nullptr);
        mRefGraphView->setVisible(false);
    }
    
    if (!mDate.isNull()) {
     /*   const double t1 ( mSettings.getTminFormated() );
        const double t2 ( mSettings.getTmaxFormated() );
        const double t3 ( mDate.getFormatedTminCalib() );
        const double t4 ( mDate.getFormatedTmaxCalib() );

        //double tminDisplay = qMin(t1,t3);
        //double tmaxDisplay = qMax(t2,t4);
     */

        mCalibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
        mCalibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
        
        // ------------------------------------------------------------
        //  Show zones if calibrated data are outside study period
        // ------------------------------------------------------------
        if (mTminDisplay < mSettings.getTminFormated()) {
            GraphZone zone;
            zone.mXStart = -INFINITY;
            zone.mXEnd = mSettings.getTminFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }
        if (mTmaxDisplay > mSettings.getTmaxFormated()) {
            GraphZone zone;
            zone.mXStart = mSettings.getTmaxFormated();
            zone.mXEnd = INFINITY;
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
        mResultsText->clear();
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
            mCalibGraph->setRangeY(0., 1. * yMax);

            mCalibGraph->addCurve(calibCurve);
            mCalibGraph->setVisible(true);

            mCalibGraph->setTipXLab("t");

            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);
            QLocale loc = QLocale();

            const double thresh = qBound(0., loc.toDouble(input), 100.);

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

            mCalibGraph->setRangeY(0., 1.1 * yMax);

            mCalibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mCalibGraph->setFormatFunctX(stringWithAppSettings);
            mCalibGraph->setFormatFunctY(stringWithAppSettings);
            
            double realThresh = map_area(hpd) / map_area(calibCurve.mData);
            
            resultsStr += + "<br> HPD (" + locale.toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), stringWithAppSettings);
            
            //mResultsLab->setWordWrap(true);
            mResultsText->setText(resultsStr);
            
        }

        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        
        // Get the ref graph for this plugin and this date
        mRefGraphView = mDate.mPlugin->getGraphViewRef();
        if (mRefGraphView) {


            mRefGraphView->setFormatFunctX(stringWithAppSettings); // must be before setDate, because setDate use it
            ProjectSettings tmpSettings;
            tmpSettings.mTmax = mTmaxDisplay;
            tmpSettings.mTmin = mTminDisplay;
            tmpSettings.mStep = 1.;
            mRefGraphView->setDate(mDate, tmpSettings);
            mRefGraphView->zoomX(mTminDisplay, mTmaxDisplay);
            //mRefGraphView->setCurrentX(mTminDisplay, mTmaxDisplay);

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
    type_data prop = 1;//(100. - (type_data)(mZoomSlider->value())) / 100.;
    if (prop < minProp)
        prop = minProp;
    
    if (prop != 1) {
        // Remember old scroll position
        type_data posProp (0);
        type_data rangeBefore = (type_data) 10;//mScrollBar->maximum();
        if(rangeBefore > 0)
            posProp = 1;//(type_data)mScrollBar->value() / rangeBefore;
        
        // Update Scroll Range
        int fullScrollSteps (1000);
        int scrollSteps = (int) ((1. - prop)) * fullScrollSteps;
        //mScrollBar->setRange(0, scrollSteps);
        //mScrollBar->setPageStep(fullScrollSteps);

        // Set scroll to correct position
        type_data pos (0.);
        type_data rangeAfter = (type_data) 10;//mScrollBar->maximum();
        if (rangeAfter > 0)
            pos = floor(posProp * rangeAfter);
        //mScrollBar->setValue(pos);
        
    } else
        //mScrollBar->setRange(0, 0);
    //mScrollBar->setRange(min, max);
    updateScroll();
}

void CalibrationView::updateScroll()
{
    bool ok;
    QLocale loc = QLocale();
    double val = loc.toDouble(mStartEdit->text(),&ok);
    if (ok)
        mTminDisplay = val;
    else
        exit;

    val = loc.toDouble(mEndEdit->text(),&ok);
    if (ok)
        mTmaxDisplay = val;
    else
        exit;
/*
    //const type_data max = mCalibGraph->maximumX();
    //const type_data minProp = 5. / (max - min);
    type_data prop = 1;//(100. - (type_data) mZoomSlider->value()) / 100.;
    if (prop < minProp)
        prop = minProp;
    
    if (prop != 1.) {
        // Update graphs with new zoom
        const type_data delta = prop * (max - min);
        const type_data deltaStart = (max - min) - delta;
        //qDebug()<<mScrollBar->value() << mScrollBar->maximum();
        type_data start = min ;//+ deltaStart * ((type_data)(mScrollBar->value()) / ((type_data)mScrollBar->maximum()) ) ;
        start = (floor(start)<min ? min : floor(start));
        type_data end = start + delta;
        end = (ceil(end)>max ? max : ceil(end));
        mCalibGraph->zoomX(start, end);
        if (mRefGraphView)
            mRefGraphView->zoomX(start, end);
        
    } else {
        mCalibGraph->zoomX(min, max);
        if (mRefGraphView)
            mRefGraphView->zoomX(min, max);
    }
    */
   updateGraphs();

   /* mCalibGraph->zoomX(mTminDisplay, mTmaxDisplay);
    if (mRefGraphView)
        mRefGraphView->zoomX(mTminDisplay, mTmaxDisplay);*/
}

void CalibrationView::exportImage()
{
    mHPDEdit       -> setVisible(false);
    mHPDLab        -> setVisible(false);
    mMarkerX       -> setVisible(false);
    mMarkerY       -> setVisible(false);
    mResultsText   -> setVisible(false);
    
   // int m = 5;
   // QRect r(m, m, this->width() - 2*m, this->height() - 2*m);
    //int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int graphLeft = mImageSaveBut->width();
    int graphWidth = width() - graphLeft;// - sbe;

    // The same rectangle than the background in paintEvent
    QRect back = QRect(graphLeft, 0, graphWidth, height());
    
    QFileInfo fileInfo = saveWidgetAsImage(this, back, tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    mHPDEdit       -> setVisible(true);
    mHPDLab        -> setVisible(true);
    mMarkerX       -> setVisible(true);
    mMarkerY       -> setVisible(true);
    mResultsText    -> setVisible(true);
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
    QString text = mTopLab->text() +"<br>" + mDate.getDesc() + "<br>" + mResultsText->toPlainText();// ->text();
    p_Clipboard->setText(text.replace("<br>", "\r"));
   
}

void CalibrationView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QFontMetrics fm (font());
    //int m (1);
    //Position of Widget
    int y (0);

    mImageSaveBut->setGeometry(0, y, mButtonWidth, mButtonHeight);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(0, y, mButtonWidth, mButtonHeight);
    y += mImageClipBut->height();
    mResultsClipBut->setGeometry(0, y, mButtonWidth, mButtonHeight);
    y += mResultsClipBut->height();
    mDataSaveBut->setGeometry(0, y, mButtonWidth, mButtonHeight);
    y += mDataSaveBut->height();


    frameSeparator->setGeometry(0, y, mButtonWidth, 5);

    y += frameSeparator->height() + 5;
    mHPDLab->setGeometry(0, y, mButtonWidth, fm.height()+3);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(3, y, mButtonWidth-6, fm.height()+3);
    y += mHPDEdit->height() + 5;
    mStartLab->setGeometry(0, y, mButtonWidth, fm.height()+3);
    y += mStartLab->height();
    mStartEdit->setGeometry(3, y, mButtonWidth-6, fm.height()+3);
    y += mStartEdit->height() + 5;
    mEndLab->setGeometry(0, y, mButtonWidth, fm.height()+3);
    y += mEndLab->height();
    mEndEdit->setGeometry(3, y, mButtonWidth-6, fm.height()+3);

    //int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    const int graphLeft = mImageSaveBut->x()+mImageSaveBut->width();
    int graphWidth = width() - graphLeft;// - sbe;


    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing); // not necessary
    // drawing a background under button
    QRect leftRect = QRect(0, 0, graphLeft, height());
    p.fillRect(leftRect, QColor(50, 50, 50));

    // drawing a background under curve
    QRect graphRect = QRect(graphLeft, 0, graphWidth, height());
    p.fillRect(graphRect, Qt::white);

    mResultsText->setGeometry(graphLeft, height() - mResultsHeight, graphWidth, height());

}

void CalibrationView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void CalibrationView::updateLayout()
{
    int m1 (5);
    int m2 (10);

    QFontMetrics fm(font());

    const type_data max = mCalibGraph->maximumX();
    const int marginRight = (int) floor(fm.width(locale().toString(max))/2);

   // int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent); // useless
    
    int topH = 40;
    int botH = 60;
    int calibH = 200;
    int titleH = 30;
    //int refH = height() - calibH - 2*titleH - topH - botH - sbe - 2*m2;
    int refH = height() - calibH - 2*titleH - topH - botH - 2*m2;
    
    const int graphLeft = mImageSaveBut->x()+mImageSaveBut->width();
    int graphWidth = width() - graphLeft;

    mTopLab->setGeometry(graphLeft, m1, graphWidth, topH);
    
    mProcessTitle->setGeometry(graphLeft, m1 + topH, graphWidth - graphLeft, titleH);

    if (mRefGraphView) {
        mRefGraphView->setGeometry(graphLeft+1, m1 + topH + titleH, graphWidth-2, refH);
        mRefGraphView->setMarginRight(marginRight);
    }
    mDistribTitle->setGeometry(graphLeft, m1 + topH + titleH + refH, graphWidth - graphLeft, titleH);
    mCalibGraph->setGeometry(graphLeft, m1 + topH + 2*titleH + refH, graphWidth, calibH);
    mCalibGraph->setMarginRight(marginRight);

    
    mMarkerX->setGeometry(mMarkerX->pos().x(), m1, mMarkerX->thickness(), height() - 2*m1 - 8.); // 8 = graph margin bottom
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

