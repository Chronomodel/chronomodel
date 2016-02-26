 #include "GraphViewResults.h"
#include "Button.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "MHVariable.h"
#include <QtWidgets>
#include <QtSvg>
#include <QMessageBox>




#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mCurrentTypeGraph(ePostDistrib),
mCurrentVariable(eTheta),
mShowAllChains(true),
mShowCredibility(false),
mShowCalib(false),
mShowWiggle(false),
mShowNumResults(false),
mMainColor(QColor(50, 50, 50)),
mMargin(5),
mLineH(20),
mGraphLeft(128),
mTopShift(0),
mButtonsVisible(true),
mHeightForVisibleAxis(100)
{
    setMouseTracking(true);
    
    mGraph = new GraphView(this);
    
    mGraph->setCanControlOpacity(true);
    mGraph->setCurvesOpacity(30);
    
    mGraph->showXAxisArrow(true);
    mGraph->showXAxisTicks(true);
    mGraph->showXAxisSubTicks(true);
    mGraph->showXAxisValues(true);
    
    mGraph->showYAxisArrow(true);
    mGraph->showYAxisTicks(true);
    mGraph->showYAxisSubTicks(true);
    mGraph->showYAxisValues(true);
    
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eMinMax);
    
    mGraph->setMargins(50, 10, 5, 30);
    mGraph->setRangeY(0, 1);
    mGraph->setMarginBottom(mGraph->font().pointSizeF() + 10);
    
    mTextArea = new QTextEdit(this);
    mTextArea->setFrameStyle(QFrame::HLine);
    QPalette palette = mTextArea->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    mTextArea->setPalette(palette);
    QFont font = mTextArea->font();
    font.setPointSizeF(pointSize(11));
    mTextArea->setFont(font);
    mTextArea->setText(tr("Nothing to display"));
    mTextArea->setVisible(false);
    mTextArea->setReadOnly(true);
    
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
    
    mAnimation = new QPropertyAnimation(this);
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);
    
    connect(mImageSaveBut, &Button::clicked, this, &GraphViewResults::saveAsImage);
    connect(mImageClipBut, &Button::clicked, this, &GraphViewResults::imageToClipboard);
    connect(mResultsClipBut, &Button::clicked, this, &GraphViewResults::resultsToClipboard);
    connect(mDataSaveBut, &Button::clicked, this, &GraphViewResults::saveGraphData);

    //connect(this, &GraphViewResults::setGraphsThickness, mGraph, &GraphView::updateCurvesThickness);

    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        
}

GraphViewResults::~GraphViewResults()
{
    mGraph = 0;
    mImageSaveBut = 0;
    mImageClipBut = 0;
    mResultsClipBut = 0;
    mDataSaveBut = 0;
    mAnimation = 0;

}

void GraphViewResults::generateCurves(TypeGraph typeGraph, Variable variable)
{
    mCurrentTypeGraph = typeGraph;
    mCurrentVariable = variable;
}

void GraphViewResults::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    mShowCredibility = showCredibility;
    mShowCalib = showCalib;
    mShowWiggle = showWiggle;
    
    // Repaint the title area above the graph :
    int leftShift = mButtonsVisible ? mGraphLeft : 0;
    QRectF textRect(leftShift, 1, this->width()-leftShift, mTopShift-1);
    update(textRect.toRect());
}

void GraphViewResults::toggle(const QRect& targetGeometry)
{
    if(geometry() != targetGeometry)
    {
        mAnimation->setStartValue(geometry());
        mAnimation->setEndValue(targetGeometry);
        mAnimation->start();
    }
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc, const QList<ChainSpecs>& chains)
{
    mMCMCSettings = mcmc;
    mChains = chains;
}

/**
 @brief set date range on all the study
 */
void GraphViewResults::setRange(double min, double max)
{
    mGraph->setRangeX(min, max);
}
void GraphViewResults::setCurrentX(double min, double max)
{
    mGraph->setCurrentX(min, max);
}

void GraphViewResults::zoom(double min, double max)
{
   // ici ca marche qDebug()<<"avant GraphViewResults::zoom"<<mGraph->getCurrentMaxX();
    mGraph->zoomX(min, max);
   // qDebug()<<"apres GraphViewResults::zoom"<<mGraph->getCurrentMaxX();
   
}

void GraphViewResults::setMainColor(const QColor& color)
{
    mMainColor = color;
    update();
}
#pragma mark Export Image & Data


void GraphViewResults::saveAsImage()
{
    //GraphView::Rendering memoRendering= mGraph->getRendering();
    /* We can not have a svg graph in eSD Rendering Mode */
    //mGraph->setRendering(GraphView::eHD);
/*    QRect r(0, 0, mGraph->width(), mGraph->height());
    QFileInfo fileInfo = saveWidgetAsImage(mGraph, r, tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    
 
    
    
    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
 */
   // mGraph->setRendering(memoRendering);
    QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph image as..."),
                                                    MainWindow::getInstance()->getCurrentPath(),
                                                    filter);
    QFileInfo fileInfo;
    fileInfo = QFileInfo(fileName);
    QString fileExtension = fileInfo.suffix();
    if(!fileName.isEmpty())
    {
        QFileInfo fileInfo = QFileInfo(fileName);
        bool asSvg = fileName.endsWith(".svg");
        if(asSvg) {
            if(mGraph)
            {
                mGraph->saveAsSVG(fileName, mTitle, "GraphViewResults",true);
            }
        }
        else {
        
            //---
            GraphView::Rendering memoRendering= mGraph->getRendering();
            setRendering(GraphView::eHD);
            short pr = MainWindow::getInstance()->getAppSettings().mPixelRatio;
            int versionHeight = 20;
            //QSize wSize = widget->size();
            QImage image(mGraph->width() * pr, (mGraph->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

            if (image.isNull() ) {
                qDebug()<< " image width = 0";
                
            }

            
            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);
            
            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);
            
            mGraph->render(&p, QPoint(0, 0), QRegion(0, 0, mGraph->width(), mGraph->height()));
        
            p.setPen(Qt::black);
            p.drawText(0, mGraph->height(), mGraph->width(), versionHeight,
                       Qt::AlignCenter,
                       qApp->applicationName() + " " + qApp->applicationVersion());
            
            p.end();

            //image.save(fileName, "PNG");
            // char formatExt[];
            if (fileExtension=="png") {
                //   formatExt[] = "png";
                image.save(fileName, "png");
            }
            else if (fileExtension == "jpg") {
                //formatExt[5] = "jpg";
                image.save(fileName, "jpg",50);
            }
            else if (fileExtension == "bmp") {
                
                image.save(fileName, "bmp");
            }
            
            mGraph->setRendering(memoRendering);
        }
        //---
        
    }
}

void GraphViewResults::imageToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(mGraph->grab());
}

void GraphViewResults::resultsToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(mResultsText);
}
/**
 * @brief Export data from the visible curves, there is two kinds of data in a graphView: mData, QMap type and mDataVector, QVector type
 * @brief So there is two export functon exportCurrentVectorCurves and exportCurrentDensityCurves.
 * @brief In the Posterior Density tab, the Credibility bar is not save 
 */
void GraphViewResults::saveGraphData() const
{
    AppSettings settings = MainWindow::getInstance()->getAppSettings();
    QString csvSep = settings.mCSVCellSeparator;

    QLocale csvLocal = settings.mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
    csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
    
    int offset = 0;
    
    if(mCurrentTypeGraph == eTrace || mCurrentTypeGraph == eAccept)
    {
        QMessageBox messageBox;
        messageBox.setWindowTitle(tr("Save all trace"));
        messageBox.setText(tr("Do you want the entire trace from the beginning of the process or only the aquisition part"));
        QAbstractButton *allTraceButton = messageBox.addButton(tr("All trace"), QMessageBox::YesRole);
        QAbstractButton *acquireTraceButton = messageBox.addButton(tr("Only acquired part"), QMessageBox::NoRole);
        
        messageBox.exec();
        if (messageBox.clickedButton() == allTraceButton)  {
            mGraph->exportCurrentVectorCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, 0);
        }
        else if (messageBox.clickedButton() == acquireTraceButton) {
                int chainIdx = -1;
                for(int i=0; i<mShowChainList.size(); ++i)
                    if(mShowChainList[i]) chainIdx = i;
                if(chainIdx != -1) {
                    offset = mChains[chainIdx].mNumBurnIter + mChains[chainIdx].mBatchIndex * mChains[chainIdx].mNumBatchIter;
                }
                mGraph->exportCurrentVectorCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, offset);
        }
        else return;
    }
    
    else if(mCurrentTypeGraph == eCorrel) {
        mGraph->exportCurrentVectorCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, 0);
    }
    
    // All visible curves are saved in the same file, the credibility bar is not save
    else if(mCurrentTypeGraph == ePostDistrib) {
        mGraph->exportCurrentDensityCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep,  mSettings.mStep);
    }

}

void GraphViewResults::setNumericalResults(const QString& resultsHTML, const QString& resultsText)
{
    mResultsText = resultsText;
    mTextArea->setText(resultsHTML);
}

void GraphViewResults::showNumericalResults(bool show)
{
    mShowNumResults = show;
    mTextArea->setVisible(show);
    updateLayout();
}

void GraphViewResults::setRendering(GraphView::Rendering render)
{
    mGraph->setRendering(render);
}

void GraphViewResults::setGraphFont(const QFont& font)
{
    setFont(font);
    mGraph->setGraphFont(font);
    mGraph->setMarginBottom(mGraph->font().pointSizeF() + 10);
   
    // Recalcule mTopShift based on the new font, and position the graph according :
    updateLayout();
}

void GraphViewResults::setGraphsThickness(int value)
{
    mGraph->setGraphsThickness(value);
}

void GraphViewResults::setGraphsOpacity(int value)
{
    mGraph->setCurvesOpacity(value);
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    updateLayout();
}

#pragma mark Layout

void GraphViewResults::updateLayout()
{
    int h = height();
    int butInlineMaxH = 50;
    
    bool axisVisible = (h > mHeightForVisibleAxis);
    mGraph->showXAxisValues(axisVisible);
    mGraph->setMarginBottom(axisVisible ? mGraph->font().pointSizeF() + 10 : 10);
    
    if(mButtonsVisible)
    {
        qreal bw = mGraphLeft / 4;
        int bh = height() - mLineH;
        bh = std::min(bh, butInlineMaxH);
        
        mImageSaveBut   -> setGeometry(0, mLineH, bw, bh);
        mDataSaveBut    -> setGeometry(bw, mLineH, bw, bh);
        mImageClipBut   -> setGeometry(2*bw, mLineH, bw, bh);
        mResultsClipBut -> setGeometry(mGraphLeft-bw, mLineH, bw, bh);
    }
    
    QFont fontTitle(this->font());
    fontTitle.setPointSizeF(this->font().pointSizeF()*1.1);
    QFontMetrics fmTitle(fontTitle);
    mTopShift = fmTitle.height()+4+1;
    
    int leftShift = mButtonsVisible ? mGraphLeft : 0;
    QRect graphRect(leftShift, mTopShift, this->width() - leftShift, height()-mTopShift);
    
    if(mShowNumResults) {
        mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
        mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
    }
    else {
        mGraph->setGeometry(graphRect);
    }
    update();
}
void GraphViewResults::paintEvent(QPaintEvent* )
{
    //int h = height();
    int leftShift = mButtonsVisible ? mGraphLeft : 0;
    
    QPainter p;
    p.begin(this);
    
    // Left part of the view (title, buttons, ...)
    if(mButtonsVisible)
    {
        QColor backCol = mItemColor;
        QColor foreCol = getContrastedColor(backCol);
        
        // affiche le texte dans la boite de droite avec les boutons
        QRectF topRect(0, 1, mGraphLeft-p.pen().widthF(), mLineH-p.pen().widthF() - 1);
        
        p.setPen(backCol);
        p.setBrush(backCol);
        p.drawRect(topRect);
        
        p.setPen(foreCol);
        QFont font;
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        
        p.drawText(QRectF(0, 1, mGraphLeft-p.pen().widthF(), mLineH-p.pen().widthF()-1),
                   Qt::AlignVCenter | Qt::AlignCenter,
                   mItemTitle);
    }
    
    // Right part of the view (graph, title, ...)
    
    // write mTitle above the graph
    QFont fontTitle(this->font());
    fontTitle.setPointSizeF(this->font().pointSizeF()*1.1);
    QFontMetrics fmTitle(fontTitle);
    
    QRectF textRect(leftShift, 1, this->width()-leftShift, mTopShift-1);
    p.fillRect(textRect, mGraph->getBackgroundColor());
    
    p.setFont(fontTitle);
    p.setPen(Qt::black);
    
    p.drawText(QRect(leftShift + 50, 3, fmTitle.width(mTitle), mTopShift - 1), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
    
    p.drawText(QRect(width() - fmTitle.width(mGraph->getInfo()), 3, fmTitle.width(mGraph->getInfo()), mTopShift-1), Qt::AlignVCenter | Qt::AlignLeft, mGraph->getInfo());
    
    p.end();

}

 void GraphViewResults::setItemColor(const QColor& itemColor)
{
    mItemColor = itemColor;
}

void GraphViewResults::setItemTitle(const QString& itemTitle)
{
    mItemTitle = itemTitle;
}

void GraphViewResults::setButtonsVisible(const bool visible)
{
    if(mButtonsVisible != visible){
        mButtonsVisible = visible;
        
        mImageSaveBut   -> setVisible(mButtonsVisible);
        mDataSaveBut    -> setVisible(mButtonsVisible);
        mImageClipBut   -> setVisible(mButtonsVisible);
        mResultsClipBut -> setVisible(mButtonsVisible);
        
        updateLayout();
    }
}

#pragma mark Generate Typical curves for Chronomodel
GraphCurve GraphViewResults::generateDensityCurve(QMap<double, double>& data,
                                                  const QString& name,
                                                  const QColor& lineColor,
                                                  const Qt::PenStyle penStyle,
                                                  const QBrush& brush) const{
    GraphCurve curve;
    curve.mName = name;
    if(!data.isEmpty()) {
        curve.mData = data;
        curve.mPen = QPen(lineColor, 1, penStyle);
        curve.mBrush = brush;
        curve.mIsHisto = false;
        curve.mIsRectFromZero = true; // for typo. calibs., invisible for others!
   }
     return curve;
}

GraphCurve GraphViewResults::generateHPDCurve(QMap<double, double>& data,
                                              const QString& name,
                                              const QColor& color) const{
    GraphCurve curve;
    curve.mName = name;
    curve.mData = data;
    curve.mPen = color;
    curve.mBrush = QBrush(color);
    curve.mIsHisto = false;
    curve.mIsRectFromZero = true;
    
    return curve;
}

GraphCurve GraphViewResults::generateCredibilityCurve(const QPair<double, double>& section,
                                                      const QString& name,
                                                      const QColor& color) const{
    GraphCurve curve;
    curve.mName = name;
    curve.mSections.append(section);
    curve.mPen.setColor(color);
    curve.mPen.setWidth(3);
    curve.mPen.setStyle(Qt::SolidLine);
    curve.mIsTopLineSections = true;
    
    return curve;
}

GraphCurve GraphViewResults::generateHorizontalLine(const double yValue,
                                                    const QString& name,
                                                    const QColor& color,
                                                    const Qt::PenStyle penStyle) const
{
    GraphCurve curve;
    curve.mName = name;
    curve.mIsHorizontalLine = true;
    curve.mHorizontalValue = yValue;
    curve.mPen.setStyle(penStyle);
    curve.mPen.setColor(color);
    return curve;
}

void GraphViewResults::generateTraceCurves(const QList<ChainSpecs> &chains,
                                           MetropolisVariable* variable,
                                           const QString& name)
{
    QString prefix = name.isEmpty() ? name : name + " ";
    
    for(int i=0; i<chains.size(); ++i)
    {

        GraphCurve curve;
        curve.mUseVectorData = true;
        curve.mName = prefix + "Trace " + QString::number(i);
        curve.mDataVector = variable->fullTraceForChain(chains, i);
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mIsHisto = false;
        mGraph->addCurve(curve);
        
        const double min = vector_min_value(curve.mDataVector);
        const double max = vector_max_value(curve.mDataVector);
        mGraph->setRangeY(floor(min), ceil(max));
        
        const Quartiles& quartiles = variable->mChainsResults.at(i).quartiles;
        
        GraphCurve curveQ1 = generateHorizontalLine(quartiles.Q1, prefix + "Q1 " + QString::number(i), Qt::green);
        mGraph->addCurve(curveQ1);
        
        GraphCurve curveQ2 = generateHorizontalLine(quartiles.Q2, prefix + "Q2 " + QString::number(i), Qt::red);
        mGraph->addCurve(curveQ2);
        
        GraphCurve curveQ3 = generateHorizontalLine(quartiles.Q3, prefix + "Q3 " + QString::number(i), Qt::green);
        mGraph->addCurve(curveQ3);
    }
}


void GraphViewResults::generateAcceptCurves(const QList<ChainSpecs> &chains,
                                            MHVariable* variable){
    for(int i=0; i<chains.size(); ++i)
    {
        GraphCurve curve;
        curve.mName = QString("Accept " + QString::number(i));
        curve.mDataVector = variable->acceptationForChain(chains, i);
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mUseVectorData = true;
        curve.mIsHisto = false;
        mGraph->addCurve(curve);
    }
}

void GraphViewResults::generateCorrelCurves(const QList<ChainSpecs> &chains,
                                            MHVariable* variable){
    for(int i=0; i<chains.size(); ++i)
    {
        GraphCurve curve;
        curve.mName = QString("Correl " + QString::number(i));
        curve.mDataVector = variable->correlationForChain(i);
        curve.mUseVectorData = true;
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mIsHisto = false;
        mGraph->addCurve(curve);
        
        //to do, we only need the totalIter number?
        double n = variable->runRawTraceForChain(mChains, i).size();
        double limit = 1.96f / sqrt(n);
        
        GraphCurve curveLimitLower = generateHorizontalLine(-limit,
                                                            "Correl Limit Lower " + QString::number(i),
                                                            Qt::red,
                                                            Qt::DotLine);
        GraphCurve curveLimitUpper = generateHorizontalLine(limit,
                                                            "Correl Limit Upper " + QString::number(i),
                                                            Qt::red,
                                                            Qt::DotLine);
        mGraph->addCurve(curveLimitLower);
        mGraph->addCurve(curveLimitUpper);
    }
}
