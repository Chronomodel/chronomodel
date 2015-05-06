#include "GraphViewResults.h"
#include "Button.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include <QtWidgets>
#include <QtSvg>




#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mButtonVisible(true),
mCurrentTypeGraph(eHisto),
mCurrentVariable(eTheta),
mMinHeighttoDisplayTitle(100),
mShowAllChains(true),
mShowHPD(false),
mThresholdHPD(95),
mShowCalib(false),
mShowWiggle(false),
mShowRawResults(false),
mShowNumResults(false),
mMainColor(QColor(50, 50, 50)),
mMargin(5),
mLineH(20),
mGraphLeft(128)
{
    setMouseTracking(true);
    
    mGraph = new GraphView(this);
    
    mGraph->showHorizGrid(false);
    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eMinMax);
    
    mGraph->setMargins(50, 10, 5, 30);
    mGraph->setRangeY(0, 1);
    
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
    mImageClipBut->setToolTip(tr("Copy text results to clipboard"));
    
    mDataSaveBut = new Button(tr("Save"), this);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));
    
    mAnimation = new QPropertyAnimation();
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);
    
    connect(mImageSaveBut, SIGNAL(clicked()), this, SLOT(saveAsImage()));
    connect(mImageClipBut, SIGNAL(clicked()), this, SLOT(imageToClipboard()));
    connect(mResultsClipBut, SIGNAL(clicked()), this, SLOT(resultsToClipboard()));
    connect(mDataSaveBut, SIGNAL(clicked()), this, SLOT(saveGraphData()));
    
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    
    
    if(mCurrentTypeGraph == eAccept || mCurrentTypeGraph == eTrace ) {
        Chain& chain = mChains[0];
        mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter / chain.mThinningInterval);
    }
    
    if(mCurrentTypeGraph == eHisto)
    {
        if(mCurrentVariable == eTheta)
        {
            mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
        }
        if(mCurrentVariable == eSigma){
            mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
        }
    }
    if(mCurrentTypeGraph == eCorrel){
        mGraph->setRangeX(0, 100);
    }
    
}

GraphViewResults::~GraphViewResults()
{
    
}

void GraphViewResults::setResultToShow(TypeGraph typGraph, Variable variable, bool showAllChains, const QList<bool>& showChainList, bool showHpd, float threshold, bool showCalib, bool showPosterior, bool showWiggle, bool showRawResults)
{
    mCurrentTypeGraph = typGraph;
    mCurrentVariable = variable;
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    mShowHPD = showHpd;
    mThresholdHPD = threshold;
    mShowPosterior = showPosterior;
    mShowCalib = showCalib;
    mShowWiggle = showWiggle;
    mShowRawResults = showRawResults;
    refresh();
}

void GraphViewResults::toggle(const QRect& targetGeometry)
{
    if(geometry() != targetGeometry)
    {
        //qDebug() << "Graph From : " << geometry() << ", To : " << targetGeometry;
        
        mAnimation->setStartValue(geometry());
        mAnimation->setEndValue(targetGeometry);
        mAnimation->start();
    }
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains)
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
            short pr = MainWindow::getInstance()->getAppSettings().mPixelRatio;// 4.;//0.005;
            //qDebug()<<"GraphViewResults::saveAsImage() pr="<<pr;
            int versionHeight = 20;
            //QSize wSize = widget->size();
            QImage image(mGraph->width() * pr, (mGraph->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32
        
            //    qDebug()<<"r.width() * pr"<< (r.width() * pr)<<" (r.height() + versionHeight) * pr"<<(r.height() + versionHeight) * pr;

            if (image.isNull() ) {
                qDebug()<< " image width = 0";
                
            }
            
            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);
            
            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);
            //QRectF tgtRect = image.rect();

//            mGraph->render(&p, QPoint(0, 0), QRegion(mGraph->x(), mGraph->y(), mGraph->width(), mGraph->height()));
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
    clipboard->setText(mResults);
}

void GraphViewResults::saveGraphData() const
{
    AppSettings settings = MainWindow::getInstance()->getAppSettings();
    QString csvSep = settings.mCSVCellSeparator;
    
    int offset = 0;
    if(mCurrentTypeGraph == eTrace)
    {
        int chainIdx = -1;
        for(int i=0; i<mShowChainList.size(); ++i)
            if(mShowChainList[i])
                chainIdx = i;
        if(chainIdx != -1)
        {
            offset = mChains[chainIdx].mNumBurnIter + mChains[chainIdx].mBatchIndex * mChains[chainIdx].mNumBatchIter;
        }
    }
    
    mGraph->exportCurrentCurves(MainWindow::getInstance()->getCurrentPath(), csvSep, false, offset);
}

void GraphViewResults::setNumericalResults(const QString& results)
{
    mResults = results;
    mTextArea->setText(mResults);
}

void GraphViewResults::showNumericalResults(bool show)
{
    mShowNumResults = show;
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
    mGraph->setMarginBottom(font.pointSizeF() + 10);
   // updateLayout();
}

void GraphViewResults::setGraphsThickness(int value)
{
    mGraph->setCurvesThickness(value);
    updateLayout();
}

void GraphViewResults::paintEvent(QPaintEvent* )
{
    QPainter p(this);
    if (mButtonVisible) { // the box under the button
        p.fillRect(0, 0, mGraphLeft, height(), mMainColor);
    }

    // draw a horizontal ligne
 /*   p.setPen(QColor(200, 200, 200));
    p.drawLine(0, height(), width(), height());
  */  
    updateLayout();
    p.end();
/*    if(height() >= mMinHeighttoDisplayTitle) // juste write mTitle
    {
        QRectF textRect(mGraphLeft, 0, mGraph->width(), 25);
        p.fillRect(textRect, mGraph->getBackgroundColor());
        
        p.setPen(Qt::black);
        p.setFont(this->font());
        
        p.drawText(textRect.adjusted(mGraph->marginLeft(), 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
    }*/
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

#pragma mark Layout

void GraphViewResults::updateLayout()
{
    int h = height();
    int butMinH = 30;
    int butInlineMaxH = 50;
    //int bh = (height() - mLineH) / 2;
    //bh = qMin(bh, 100);
    QPainter p;
    p.begin(this);
    
    int leftShift = 0;
   // int topShift = 0;
    
   // if(height() >= mMinHeighttoDisplayTitle) {
   //     topShift = mLineH;
   // }
   
    
    if (mButtonVisible) {
        
        bool showButs = (h >= mLineH + butMinH);
        //showButs = false;
        leftShift = mGraphLeft;
     
        mImageSaveBut   -> setVisible(showButs);
        mDataSaveBut    -> setVisible(showButs);
        mImageClipBut   -> setVisible(showButs);
        mResultsClipBut -> setVisible(showButs);
        
        
        QColor backCol = mItemColor;
        QColor foreCol = getContrastedColor(backCol);
        
    
        if(showButs)  {
            qreal bw = mGraphLeft / 4;
            int bh = height() - mLineH;
            bh = std::min(bh, butInlineMaxH);
        
            mImageSaveBut   -> setGeometry(0, mLineH, bw, bh);
            mDataSaveBut    -> setGeometry(bw, mLineH, bw, bh);
            mImageClipBut   -> setGeometry(2*bw, mLineH, bw, bh);
            mResultsClipBut -> setGeometry(mGraphLeft-bw, mLineH, bw, bh);
            
            
            
            QRect topRect(0, 0, mGraphLeft, mLineH);
            p.setPen(backCol);
            p.setBrush(backCol);
            p.drawRect(topRect);
            
            
        }
    
        
    
    /*    QColor backCol = mItemColor;
        QColor foreCol = getContrastedColor(backCol);
    
//      QRect topRect(0, 0, mGraphLeft, mLineH);
        QRect topRect(0, 0, mGraphLeft, mLineH);
        p.setPen(backCol);
        p.setBrush(backCol);
        p.drawRect(topRect);
   */
       // p.setPen(Qt::black);
       // p.drawLine(0, height(), mGraphLeft, height());
    
        p.setPen(foreCol);
        QFont font;
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        // affiche le texte dans la boite de droite avec les boutons
        
        p.drawText(QRectF(0, 0, mGraphLeft-p.pen().widthF(), mLineH-p.pen().widthF()),
                   Qt::AlignVCenter | Qt::AlignCenter,
                   mItemTitle);
        //qDebug()<<"GraphViewResults::updateLayout() mItemTitle "<<mItemTitle;
        
    }
    else {
        leftShift = 0;
        mImageSaveBut   -> setVisible(false);
        mDataSaveBut    -> setVisible(false);
        mImageClipBut   -> setVisible(false);
        mResultsClipBut -> setVisible(false);
    }
   
    if(h <= mLineH + butMinH)
    {
        mGraph -> setYAxisMode(GraphView::eHidden);
    }
    else
    {
        mGraph->setYAxisMode(GraphView::eMinMax);
    }
    
    
    p.setPen(Qt::black);
    p.setFont(this->font());
    
 /*   QRect graphRect(leftShift, topShift, this->width() - leftShift, height()-1-topShift);
    if(mShowNumResults && height() >= mMinHeighttoDisplayTitle/2)//100)
    {
        mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
        mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
        mTextArea -> setVisible(true);
    }
    else
    {
        mGraph    -> setGeometry(graphRect);
        mTextArea -> setVisible(false);
    }*/
    // write mTitle under the curve field
    QFontMetrics fm = this->fontMetrics();
    int topShift = fm.height()+4;
    //QRectF textRect(leftShift, 0, this->width()-leftShift,topShift);
    QRectF textRect(leftShift, 2, this->width()-leftShift,topShift);
    p.fillRect(textRect, mGraph->getBackgroundColor());
    
    //p.drawText(textRect.adjusted(50, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
    //QFont fontTitle(this->font());
    //fontTitle.weight()= fontTitle.weight()*1.5;
    //p.font()=fontTitle;
    p.drawText(QRect(50, 0, topShift-2, fm.width(mTitle)), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
    
    p.drawText(textRect.adjusted(100, 0, 0, 0), Qt::AlignVCenter | Qt::AlignRight, mGraph->getInfo());
    
    
    
    QRect graphRect(leftShift, topShift, this->width() - leftShift, height()-1-topShift);
    
    if(mShowNumResults) {
        
        mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
        mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
        mTextArea -> setVisible(true);
    }
    else {
        mGraph    -> setGeometry(graphRect);
        mTextArea -> setVisible(false);
    }
    
    if(height() >= mMinHeighttoDisplayTitle)  {
        mGraph -> setXAxisMode(GraphView::eAllTicks);
        mGraph -> setMarginBottom(mGraph->font().pointSizeF() + 10);
        
        //QRect graphRect(leftShift, topShift, this->width() - leftShift, height()-1-topShift);
       /* if(mShowNumResults) {
            
            mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
            mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
            mTextArea -> setVisible(true);
        }
        else {
            mGraph    -> setGeometry(graphRect);
            mTextArea -> setVisible(false);
        }
        */
        
        
        
        // write mTitle under the curve field
      /*  QRectF textRect(leftShift, 0, this->width()-leftShift,mLineH);
        p.fillRect(textRect, mGraph->getBackgroundColor());
        
        p.drawText(textRect.adjusted(50, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
      */  
        
    }
    else
    {
        mGraph -> setXAxisMode(GraphView::eHidden);

        mGraph -> setMarginBottom(10);
        
        //QRect graphRect(leftShift, topShift, this->width() - leftShift, height()-1-topShift);
       /* if(mShowNumResults) {
            
            mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
            mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
            mTextArea -> setVisible(true);
        }
        else {
            mGraph    -> setGeometry(graphRect);
            mTextArea -> setVisible(false);
        }
       */
      /*  QRectF textRect(leftShift, 0, this->width()-leftShift,mLineH);
        p.fillRect(textRect, mGraph->getBackgroundColor());
        
        p.drawText(textRect.adjusted(50, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
       */
        
     /*   QFontMetrics fm(this->font());
        qreal textWidth = fm.width(mTitle);
        //QRectF textRect(leftShift+100, mLineH, this->width()-leftShift,mLineH);
        QRectF textRect(leftShift+100, this->height()/2, this->width()-leftShift,mLineH);
        //p.fillRect(textRect, mGraph->getBackgroundColor());
       // p.setBrush(QBrush(Qt::NoBrush));
      //  p.drawText(textRect.adjusted(50, mLineH, this->width()-50-textWidth, 0), Qt::AlignVCenter | Qt::AlignLeft, mTitle);
        qDebug()<<"GraphViewResults::updateLayout() inside mTitle "<<mTitle;
         p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, mTitle);
        p.setBrush(QBrush(Qt::SolidPattern));
      */
    }
    
    
   /* QRect graphRect(leftShift, topShift, this->width() - leftShift, height()-1-topShift);
    if(mShowNumResults)// && height() >= 100)
    {
        mGraph    -> setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
        mTextArea -> setGeometry(graphRect.adjusted(0, graphRect.height()/2, 0, 0));
        mTextArea -> setVisible(true);
    }
    else
    {
        mGraph    -> setGeometry(graphRect);
        mTextArea -> setVisible(false);
    }*/
    
    p.end();
    refresh();
}

 void GraphViewResults::setItemColor(const QColor& itemColor)
{
    mItemColor = itemColor;
}

void GraphViewResults::setItemTitle(const QString& ItemTitle)
{
    mItemTitle = ItemTitle;
}
