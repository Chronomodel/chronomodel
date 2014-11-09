#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "ProjectManager.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewEvent::GraphViewEvent(QWidget *parent):GraphViewResults(parent),
mEvent(0),
mShowVariances(false)
{
    //setMainColor(QColor(100, 100, 120));
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewEvent::~GraphViewEvent()
{
    mEvent = 0;
}

void GraphViewEvent::setEvent(Event* event)
{
    if(event)
        mEvent = event;
    update();
}

void GraphViewEvent::showVariances(bool show)
{
    mShowVariances = show;
    refresh();
}

void GraphViewEvent::refresh()
{
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        if(mCurrentResult == eHisto)
        {
            if(!mShowVariances)
            {
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
                mGraph->setRangeY(0, 0.00001f);
                
                if(mShowAllChains)
                {
                    GraphCurve curve;
                    curve.mName = "histo full";
                    curve.mPen.setColor(color);
                    curve.mData = equal_areas(mEvent->mTheta.fullHisto(), 100.f);
                    mGraph->addCurve(curve);
                    
                    float yMax = 1.1f * map_max_value(curve.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "histo HPD full";
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mData = equal_areas(mEvent->mTheta.generateFullHPD(mThresholdHPD), mThresholdHPD);
                        mGraph->addCurve(curveHPD);
                    }
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curve;
                        curve.mName = QString("histo chain " + QString::number(i));
                        curve.mPen.setColor(col);
                        curve.mData = equal_areas(mEvent->mTheta.histoForChain(i), 100.f);
                        mGraph->addCurve(curve);
                        
                        float yMax = 1.1f * map_max_value(curve.mData);
                        mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                        
                        if(mShowHPD)
                        {
                            GraphCurve curveHPD;
                            curveHPD.mName = QString("histo HPD chain " + QString::number(i));
                            curveHPD.mPen.setColor(color);
                            curveHPD.mFillUnder = true;
                            curveHPD.mData = equal_areas(mEvent->mTheta.generateHPDForChain(i, mThresholdHPD), mThresholdHPD);
                            mGraph->addCurve(curveHPD);
                        }
                    }
                }
            }
            else
            {
                // On est en train de regarder les variances des data
                // On affiche donc ici la superposition des variances (et pas le résultat de theta f)
                
                for(int i=0; i<mEvent->mDates.size(); ++i)
                {
                    Date& date = mEvent->mDates[i];
                    if(mShowAllChains)
                    {
                        GraphCurve curve;
                        curve.mName = "histo full date " + QString::number(i);
                        curve.mPen.setColor(color);
                        curve.mData = equal_areas(date.mSigma.fullHisto(), 100.f);
                        mGraph->addCurve(curve);
                        
                        float yMax = 1.1f * map_max_value(curve.mData);
                        mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                        
                        if(mShowHPD)
                        {
                            GraphCurve curveHPD;
                            curveHPD.mName = "histo HPD full";
                            curveHPD.mPen.setColor(color);
                            curveHPD.mFillUnder = true;
                            curveHPD.mData = equal_areas(date.mSigma.generateFullHPD(mThresholdHPD), mThresholdHPD);
                            mGraph->addCurve(curveHPD);
                        }
                    }
                    for(int j=0; j<mShowChainList.size(); ++j)
                    {
                        if(mShowChainList[j])
                        {
                            QColor col = Painting::chainColors[j];
                            
                            GraphCurve curve;
                            curve.mName = QString("histo sigma data " + QString::number(i) + " for chain" + QString::number(j));
                            curve.mPen.setColor(col);
                            curve.mData = equal_areas(date.mSigma.histoForChain(j), 100.f);
                            mGraph->addCurve(curve);
                            
                            float yMax = 1.1f * map_max_value(curve.mData);
                            mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                            
                            if(mShowHPD)
                            {
                                GraphCurve curveHPD;
                                curveHPD.mName = QString("hpd sigma data " + QString::number(i) + " for chain" + QString::number(j));
                                curveHPD.mPen.setColor(color);
                                curveHPD.mFillUnder = true;
                                curveHPD.mData = equal_areas(date.mSigma.generateHPDForChain(j, mThresholdHPD), mThresholdHPD);
                                mGraph->addCurve(curveHPD);
                            }
                        }
                    }
                }
            }
        }
        if(mCurrentResult == eTrace)
        {
            mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
            
            // ------
            
            GraphZone zoneBurn;
            zoneBurn.mXStart = 0;
            zoneBurn.mXEnd = mMCMCSettings.mNumBurnIter;
            zoneBurn.mColor = QColor(200, 200, 200);
            zoneBurn.mText = "Burn";
            mGraph->addZone(zoneBurn);
            
            GraphZone zoneAdapt;
            zoneAdapt.mXStart = mMCMCSettings.mNumBurnIter;
            zoneAdapt.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
            zoneAdapt.mColor = QColor(230, 230, 230);
            zoneAdapt.mText = "Adapt";
            mGraph->addZone(zoneAdapt);
            
            GraphZone zoneAquire;
            zoneAquire.mXStart = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
            zoneAquire.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter;
            zoneAquire.mColor = QColor(255, 255, 255);
            zoneAquire.mText = "Aquire";
            mGraph->addZone(zoneAquire);
            
            // ------
            
            float min = 999999;
            float max = -999999;
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col = Painting::chainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("trace chain " + QString::number(i)).toUtf8();
                    curve.mUseVectorData = true;
                    curve.mDataVector = mEvent->mTheta.traceForChain(i, mShowChainList.size());
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    min = qMin(vector_min_value(curve.mDataVector), min);
                    max = qMax(vector_max_value(curve.mDataVector), max);
                }
            }
            if(min < max)
                mGraph->setRangeY(min, max);
            else
                mGraph->setRangeY(0, 1);
        }
        else if(mCurrentResult == eAccept)
        {
            mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
            mGraph->setRangeY(0, 100);
            
            // ------
            
            GraphZone zoneBurn;
            zoneBurn.mXStart = 0;
            zoneBurn.mXEnd = mMCMCSettings.mNumBurnIter;
            zoneBurn.mColor = QColor(200, 200, 200);
            zoneBurn.mText = "Burn";
            mGraph->addZone(zoneBurn);
            
            GraphZone zoneAdapt;
            zoneAdapt.mXStart = mMCMCSettings.mNumBurnIter;
            zoneAdapt.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
            zoneAdapt.mColor = QColor(230, 230, 230);
            zoneAdapt.mText = "Adapt";
            mGraph->addZone(zoneAdapt);
            
            GraphZone zoneAquire;
            zoneAquire.mXStart = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
            zoneAquire.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter;
            zoneAquire.mColor = QColor(255, 255, 255);
            zoneAquire.mText = "Aquire";
            mGraph->addZone(zoneAquire);
            
            // ------
            
            GraphCurve curveTarget;
            curveTarget.mName = "target";
            curveTarget.mIsHorizontalLine = true;
            curveTarget.mHorizontalValue = 44;
            curveTarget.mPen.setStyle(Qt::DashLine);
            curveTarget.mPen.setColor(QColor(180, 10, 20));
            mGraph->addCurve(curveTarget);
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col = Painting::chainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("accept history chain " + QString::number(i));
                    curve.mDataVector = mEvent->mTheta.acceptationForChain(i, mShowChainList.size());
                    curve.mUseVectorData = true;
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    if(curve.mDataVector.size() > 0)
                    {
                        QString info(tr("Chain") + " " + QString::number(i+1) + " : " + QString::number(curve.mDataVector[curve.mDataVector.size()-1], 'f', 1) + " %");
                        mGraph->addInfo(info);
                    }
                }
            }
        }
        else if(mCurrentResult == eCorrel)
        {
            
        }
    }
}

void GraphViewEvent::saveGraphData()
{
    if(mEvent)
    {
        QString filter = tr("CSV (*.csv)");
        QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                        tr("Save graph data as..."),
                                                        ProjectManager::getCurrentPath(),
                                                        filter);
        QFile file(filename);
        if(file.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream output(&file);
            
            bool firstWritten = false;
            if(mCurrentResult == eHisto)
            {
                if(mShowAllChains)
                {
                    const QMap<float, float>& data = mEvent->mTheta.fullHisto();
                    if(!firstWritten)
                    {
                        QMapIterator<float, float> iter(data);
                        QStringList abscisses;
                        abscisses << "";
                        while(iter.hasNext())
                        {
                            iter.next();
                            abscisses << QString::number(iter.key());
                        }
                        output << abscisses.join(";");
                        output << "\n";
                    }
                    QMapIterator<float, float> iter(data);
                    QStringList ordonnees;
                    ordonnees << tr("All chains");
                    while(iter.hasNext())
                    {
                        iter.next();
                        ordonnees << QString::number(iter.value());
                    }
                    output << ordonnees.join(";");
                    output << "\n";
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        const QMap<float, float>& data = mEvent->mTheta.histoForChain(i);
                        if(!firstWritten)
                        {
                            QMapIterator<float, float> iter(data);
                            QStringList abscisses;
                            abscisses << "";
                            while(iter.hasNext())
                            {
                                iter.next();
                                abscisses << QString::number(iter.key());
                            }
                            output << abscisses.join(";");
                            output << "\n";
                        }
                        QMapIterator<float, float> iter(data);
                        QStringList ordonnees;
                        ordonnees << tr("Chain") + " " + QString::number(i);
                        while(iter.hasNext())
                        {
                            iter.next();
                            ordonnees << QString::number(iter.value());
                        }
                        output << ordonnees.join(";");
                        output << "\n";
                    }
                }
            }
            file.close();
        }
    }
}

void GraphViewEvent::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mEvent)
    {
        QColor backCol = mEvent->mColor;
        QColor foreCol = getContrastedColor(backCol);
        
        QRect topRect(0, 0, mGraphLeft, mLineH);
        p.fillRect(topRect.adjusted(1, 1, -1, 0), backCol);
        
        p.setPen(foreCol);
        QFont font;
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        QString type = (mEvent->mType == Event::eDefault) ? tr("Event") : tr("Bound");
        p.drawText(topRect.adjusted(mMargin, 0, -mMargin, 0),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   type + " : " + mEvent->mName);
    }
}
