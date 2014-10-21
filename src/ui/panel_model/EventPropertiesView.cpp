#include "EventPropertiesView.h"
#include "ColorPicker.h"
#include "Event.h"
#include "EventKnown.h"
#include "DatesList.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include "GraphView.h"
#include "Painting.h"
#include "../PluginAbstract.h"
#include "PluginManager.h"
#include "ProjectManager.h"
#include "Project.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>


EventPropertiesView::EventPropertiesView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    Project* project = ProjectManager::getProject();
    connect(project, SIGNAL(currentEventChanged(const QJsonObject&)), this, SLOT(setEvent(const QJsonObject&)));
    
    // -------------
    
    mNameLab = new Label(tr("Name") + " :", this);
    mColorLab = new Label(tr("Color") + " :", this);
    
    mNameEdit = new LineEdit(this);
    mColorPicker = new ColorPicker(Qt::black, this);
    
    mDefaultView = new QWidget(this);
    
    // -------------
    
    mMethodLab = new Label(tr("Method") + " :", mDefaultView);
    mMethodCombo = new QComboBox(mDefaultView);
    mMethodCombo->addItem(tr("Gibbs : reject method with double-exp"));
    mMethodCombo->addItem(tr("Gibbs : simulation with Box-Müller"));
    mMethodCombo->addItem(tr("MH : proposal = adaptative Gauss"));
    
    connect(mNameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateEventName(const QString&)));
    connect(mColorPicker, SIGNAL(colorChanged(QColor)), this, SLOT(updateEventColor(QColor)));
    connect(mMethodCombo, SIGNAL(activated(int)), this, SLOT(updateEventMethod(int)));
    
    // -------------
    
    mDatesList = new DatesList(mDefaultView);
    
    // -------------
    
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for(int i=0; i<plugins.size(); ++i)
    {
        Button* button = new Button(plugins[i]->getName(), mDefaultView);
        button->setIcon(plugins[i]->getIcon());
        button->setFlatVertical();
        connect(button, SIGNAL(clicked()), this, SLOT(createDate()));
        
        if(plugins[i]->doesCalibration())
            mPluginButs1.append(button);
        else
            mPluginButs2.append(button);
    }
    
    mDeleteBut = new Button(tr("Delete"), mDefaultView);
    mDeleteBut->setIcon(QIcon(":delete.png"));
    mDeleteBut->setFlatVertical();
    connect(mDeleteBut, SIGNAL(clicked()), this, SLOT(deleteSelectedDates()));
    
    mRecycleBut = new Button(tr("Restore"), mDefaultView);
    mRecycleBut->setIcon(QIcon(":restore.png"));
    mRecycleBut->setFlatVertical();
    connect(mRecycleBut, SIGNAL(clicked()), this, SLOT(recycleDates()));
    
    // ---------------
    
    mMergeBut = new Button(tr("Combine"), mDefaultView);
    mMergeBut->setFlatVertical();
    mMergeBut->setEnabled(false);
    
    mSplitBut = new Button(tr("Split"), mDefaultView);
    mSplitBut->setFlatVertical();
    mSplitBut->setEnabled(false);
    
    
    connect(mMergeBut, SIGNAL(clicked()), this, SLOT(sendMergeSelectedDates()));
    connect(mSplitBut, SIGNAL(clicked()), this, SLOT(sendSplitDate()));
    
    // ---------------
    
    mKnownView = new QWidget(this);
    
    mKnownFixedLab = new Label(tr("Value") + " :", mKnownView);
    mKnownStartLab = new Label(tr("Start") + " :", mKnownView);
    mKnownEndLab = new Label(tr("End") + " :", mKnownView);
    mKnownGaussMeasureLab = new Label(tr("Measure") + " :", mKnownView);
    mKnownGaussErrorLab = new Label(tr("Error") + " :", mKnownView);
    
    mKnownFixedRadio = new RadioButton(tr("Fixed") + " :", mKnownView);
    mKnownUniformRadio = new RadioButton(tr("Uniform") + " :", mKnownView);
    mKnownGaussRadio = new RadioButton(tr("Gauss") + " :", mKnownView);
    
    connect(mKnownFixedRadio, SIGNAL(clicked()), this, SLOT(updateKnownType()));
    connect(mKnownUniformRadio, SIGNAL(clicked()), this, SLOT(updateKnownType()));
    connect(mKnownGaussRadio, SIGNAL(clicked()), this, SLOT(updateKnownType()));
    
    mKnownFixedEdit = new LineEdit(mKnownView);
    mKnownStartEdit = new LineEdit(mKnownView);
    mKnownEndEdit = new LineEdit(mKnownView);
    mKnownGaussMeasure = new LineEdit(mKnownView);
    mKnownGaussError = new LineEdit(mKnownView);
    
    mKnownGraph = new GraphView(mKnownView);
    mKnownGraph->showAxis(false);
    mKnownGraph->showGrid(true);
    mKnownGraph->showYValues(true);
    mKnownGraph->showScrollBar(false);
    
    connect(mKnownFixedEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateKnownFixed()));
    connect(mKnownStartEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateKnownUnifStart()));
    connect(mKnownEndEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateKnownUnifEnd()));
    connect(mKnownGaussMeasure, SIGNAL(textChanged(const QString&)), this, SLOT(updateKnownGaussMeasure()));
    connect(mKnownGaussError, SIGNAL(textChanged(const QString&)), this, SLOT(updateKnownGaussError()));
    
    setEvent(QJsonObject());
}

EventPropertiesView::~EventPropertiesView()
{
    
}

#pragma mark Event Managment
void EventPropertiesView::setEvent(const QJsonObject& event)
{
    mEvent = event;
    updateEvent();
}

void EventPropertiesView::updateEvent()
{
    mDatesList->setEvent(mEvent);
    
    bool empty = mEvent.isEmpty();
    
    mNameEdit->setEnabled(!empty);
    mColorPicker->setEnabled(!empty);
    mMethodCombo->setEnabled(!empty);
    mDeleteBut->setEnabled(!empty);
    mRecycleBut->setEnabled(!empty);
    
    for(int i=0; i<mPluginButs1.size(); ++i)
        mPluginButs1[i]->setEnabled(!empty);
    
    for(int i=0; i<mPluginButs2.size(); ++i)
        mPluginButs2[i]->setEnabled(!empty);
    
    if(empty)
    {
        mDefaultView->setVisible(true);
        mKnownView->setVisible(false);
    }
    else
    {
        Event::Type type = (Event::Type)mEvent[STATE_EVENT_TYPE].toInt();
        QString name = mEvent[STATE_EVENT_NAME].toString();
        QColor color(mEvent[STATE_EVENT_RED].toInt(),
                     mEvent[STATE_EVENT_GREEN].toInt(),
                     mEvent[STATE_EVENT_BLUE].toInt());
        
        mNameEdit->setText(name);
        mColorPicker->setColor(color);
        
        mDefaultView->setVisible(type == Event::eDefault);
        mKnownView->setVisible(type == Event::eKnown);
        
        if(type == Event::eDefault)
        {
            mMethodCombo->setCurrentIndex(mEvent[STATE_EVENT_METHOD].toInt());
        }
        else if(type == Event::eKnown)
        {
            EventKnown::KnownType knownType = (EventKnown::KnownType)mEvent[STATE_EVENT_KNOWN_TYPE].toInt();
            
            mKnownFixedRadio->setChecked(knownType == EventKnown::eFixed);
            mKnownUniformRadio->setChecked(knownType == EventKnown::eUniform);
            mKnownGaussRadio->setChecked(knownType == EventKnown::eGauss);
            
            mKnownFixedEdit->setText(QString::number(mEvent[STATE_EVENT_KNOWN_FIXED].toDouble()));
            mKnownStartEdit->setText(QString::number(mEvent[STATE_EVENT_KNOWN_START].toDouble()));
            mKnownEndEdit->setText(QString::number(mEvent[STATE_EVENT_KNOWN_END].toDouble()));
            mKnownGaussMeasure->setText(QString::number(mEvent[STATE_EVENT_KNOWN_MEASURE].toDouble()));
            mKnownGaussError->setText(QString::number(mEvent[STATE_EVENT_KNOWN_ERROR].toDouble()));
            
            updateKnownControls();
            updateKnownGraph();
        }
    }
    updateLayout();
    update();
}

const QJsonObject& EventPropertiesView::getEvent() const
{
    return mEvent;
}

#pragma mark Event Properties
void EventPropertiesView::updateEventName(const QString& name)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_NAME] = name;
    ProjectManager::getProject()->updateEvent(event, tr("Event name updated"));
}

void EventPropertiesView::updateEventColor(QColor color)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_RED] = color.red();
    event[STATE_EVENT_GREEN] = color.green();
    event[STATE_EVENT_BLUE] = color.blue();
    ProjectManager::getProject()->updateEvent(event, tr("Event color updated"));
}

void EventPropertiesView::updateEventMethod(int index)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_METHOD] = index;
    ProjectManager::getProject()->updateEvent(event, tr("Event method updated"));
}

#pragma mark Event Known Properties
void EventPropertiesView::updateKnownType()
{
    if(mEvent[STATE_EVENT_TYPE].toInt() == Event::eKnown)
    {
        EventKnown::KnownType type = EventKnown::eFixed;
        if(mKnownUniformRadio->isChecked())
            type = EventKnown::eUniform;
        else if(mKnownGaussRadio->isChecked())
            type = EventKnown::eGauss;
        
        if(mEvent[STATE_EVENT_KNOWN_TYPE].toInt() != type)
        {
            QJsonObject event = mEvent;
            event[STATE_EVENT_KNOWN_TYPE] = type;
            ProjectManager::getProject()->updateEvent(event, tr("Bound type updated"));
        }
    }
}

void EventPropertiesView::updateKnownFixed()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_FIXED] = mKnownFixedEdit->text().toFloat();
    ProjectManager::getProject()->updateEvent(event, tr("Bound fixed value updated"));
}
void EventPropertiesView::updateKnownUnifStart()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_START] = mKnownStartEdit->text().toFloat();
    ProjectManager::getProject()->updateEvent(event, tr("Bound min updated"));
}

void EventPropertiesView::updateKnownUnifEnd()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_END] = mKnownEndEdit->text().toFloat();
    ProjectManager::getProject()->updateEvent(event, tr("Bound max updated"));
}

void EventPropertiesView::updateKnownGaussMeasure()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_MEASURE] = mKnownGaussMeasure->text().toFloat();
    ProjectManager::getProject()->updateEvent(event, tr("Bound average updated"));
}

void EventPropertiesView::updateKnownGaussError()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_ERROR] = mKnownGaussError->text().toFloat();
    ProjectManager::getProject()->updateEvent(event, tr("Bound error updated"));
}

void EventPropertiesView::loadKnownCsv()
{
    /*if(mEvent)
     {
     QString currentDir = ProjectManager::getCurrentPath();
     QString path = QFileDialog::getOpenFileName(qApp->activeWindow(), tr("Open CSV File"), currentDir, tr("CSV File (*.csv)"));
     
     if(!path.isEmpty())
     {
     QFileInfo info(path);
     QString dirPath = info.absolutePath();
     ProjectManager::setCurrentPath(dirPath);
     
     QList<QStringList> csv = readCSV(path, ";");
     qDebug() << csv;
     qDebug() << path;
     qDebug() << csv.size();
     
     EventKnown* e = dynamic_cast<EventKnown*>(mEvent);
     if(e)
     {
     for(int i=0; i<csv.size(); ++i)
     {
     float x = csv[i][0].replace(",", ".").toFloat();
     float y = csv[i][1].replace(",", ".").toFloat();
     e->mValues[x] = y;
     }
     e->mValues = normalize_map(e->mValues);
     
     Project* project = ProjectManager::getProject();
     e->updateValues(project->mSettings.mTmin, project->mSettings.mTmax, project->mSettings.mStep);
     updateKnownGraph();
     }
     }
     }*/
}

void EventPropertiesView::updateKnownGraph()
{
    mKnownGraph->removeAllCurves();
    
    if(mEvent[STATE_EVENT_TYPE].toInt() == Event::eKnown)
    {
        Project* project = ProjectManager::getProject();
        QJsonObject state = project->state();
        QJsonObject settings = state[STATE_SETTINGS].toObject();
        
        EventKnown event = EventKnown::fromJson(mEvent);
        event.updateValues(settings[STATE_SETTINGS_TMIN].toDouble(),
                           settings[STATE_SETTINGS_TMAX].toDouble(),
                           settings[STATE_SETTINGS_STEP].toDouble());
        
        mKnownGraph->setRangeX(settings[STATE_SETTINGS_TMIN].toDouble(),
                               settings[STATE_SETTINGS_TMAX].toDouble());
        
        mKnownGraph->setRangeY(0, map_max_value(event.mValues));
        
        GraphCurve curve;
        curve.mName = "Known";
        curve.mData = event.mValues;
        curve.mPen.setColor(Qt::blue);
        curve.mFillUnder = true;
        mKnownGraph->addCurve(curve);
    }
}

void EventPropertiesView::updateKnownControls()
{
    if(mKnownFixedRadio->isChecked())
    {
        mKnownFixedEdit->setEnabled(true);
        mKnownStartEdit->setEnabled(false);
        mKnownEndEdit->setEnabled(false);
        mKnownGaussMeasure->setEnabled(false);
        mKnownGaussError->setEnabled(false);
    }
    else if(mKnownUniformRadio->isChecked())
    {
        mKnownFixedEdit->setEnabled(false);
        mKnownStartEdit->setEnabled(true);
        mKnownEndEdit->setEnabled(true);
        mKnownGaussMeasure->setEnabled(false);
        mKnownGaussError->setEnabled(false);
    }
    else if(mKnownGaussRadio->isChecked())
    {
        mKnownFixedEdit->setEnabled(false);
        mKnownStartEdit->setEnabled(false);
        mKnownEndEdit->setEnabled(false);
        mKnownGaussMeasure->setEnabled(true);
        mKnownGaussError->setEnabled(true);
    }
}

#pragma mark Event Data
void EventPropertiesView::createDate()
{
    if(!mEvent.isEmpty())
    {
        Button* but = dynamic_cast<Button*>(sender());
        if(but)
        {
            Project* project = ProjectManager::getProject();
            const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
            
            for(int i=0; i<plugins.size(); ++i)
            {
                if(plugins[i]->getName() == but->text())
                {
                    Date date = project->createDateFromPlugin(plugins[i]);
                    project->addDate(mEvent[STATE_EVENT_ID].toInt(), date.toJson());
                }
            }
        }
    }
}

void EventPropertiesView::deleteSelectedDates()
{
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> indexes;
    for(int i=0; i<items.size(); ++i)
        indexes.push_back(mDatesList->row(items[i]));
    
    ProjectManager::getProject()->deleteDates(mEvent[STATE_EVENT_ID].toInt(), indexes);
}

void EventPropertiesView::recycleDates()
{
    ProjectManager::getProject()->recycleDates(mEvent[STATE_EVENT_ID].toInt());
}

#pragma mark Merge / Split
void EventPropertiesView::updateDatesSelection()
{
    /*QList<Date*> selectedDates;
    QString pluginName;
    bool mergeable = true;
    bool splittable = false;
    for(int i=0; i<mEvent->mDates.size(); ++i)
    {
        if(mEvent->mDates[i]->mIsSelected)
        {
            selectedDates.append(mEvent->mDates[i]);
            
            QString plgName = mEvent->mDates[i]->mPlugin->getName();
            if(pluginName.isEmpty())
                pluginName = plgName;
            if(plgName != pluginName)
                mergeable = false;
            
            if(mEvent->mDates[i]->mSubDates.size() > 0)
            {
                mergeable = false;
                splittable = true;
            }
        }
    }
    mDeleteBut->setEnabled(selectedDates.size() > 0);
    mMergeBut->setEnabled(selectedDates.size() >= 2 && mergeable);
    mSplitBut->setEnabled(selectedDates.size() == 1 && splittable);*/
}

void EventPropertiesView::sendMergeSelectedDates()
{
    //emit mergeSelectedDates(mEvent);
}

void EventPropertiesView::sendSplitDate()
{
    /*for(int i=0; i<mEvent->mDates.size(); ++i)
    {
        if(mEvent->mDates[i]->mIsSelected)
        {
            emit splitDate(mEvent->mDates[i]);
            return;
        }
    }*/
}



#pragma mark Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(200, 200, 200));
}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void EventPropertiesView::updateLayout()
{
    QRect r = rect().adjusted(0, 0, 0, 0);
    int m = 5;
    int w1 = 80;
    int w2 = r.width() - w1 - 3*m;
    int lineH = 20;
    int comboH = mMethodCombo->height();
    int butW = 80;
    int butH = 50;
    
    if(width() < 100)
    {
        butW = 0;
    }
    
    mNameLab->setGeometry(m, m, w1, lineH);
    mColorLab->setGeometry(m, 2*m + lineH, w1, lineH);
    
    mNameEdit->setGeometry(2*m + w1, m, w2, lineH);
    mColorPicker->setGeometry(2*m + w1, 2*m + lineH, w2, lineH);
    
    QRect typeRect = r.adjusted(0, 3*m + 2*lineH, 0, 0);
    mDefaultView->setGeometry(typeRect);
    mKnownView->setGeometry(typeRect);
    
    
    // Default view :
    
    mMethodLab->setGeometry(0, 0, w1, comboH);
    mMethodCombo->move(w1 + m, 0);
    mMethodCombo->setFixedWidth(w2);
    
    QRect listRect(0, comboH + m, typeRect.width() - butW, typeRect.height() - comboH - m);
    mDatesList->setGeometry(listRect);
    
    int x = listRect.width();
    int y = comboH + m;
    
    for(int i=0; i<mPluginButs1.size(); ++i)
    {
        mPluginButs1[i]->setGeometry(x, y, butW, butH);
        y += butH;
    }
    
    //y += butGap;
    
    for(int i=0; i<mPluginButs2.size(); ++i)
    {
        mPluginButs2[i]->setGeometry(x, y, butW, butH);
        y += butH;
    }
    
    mMergeBut->setGeometry(x, comboH + m + listRect.height() - 4*butH, butW, butH);
    mSplitBut->setGeometry(x, comboH + m + listRect.height() - 3*butH, butW, butH);
    mDeleteBut->setGeometry(x, comboH + m + listRect.height() - 2*butH, butW, butH);
    mRecycleBut->setGeometry(x, comboH + m + listRect.height() - butH, butW, butH);
    
    
    // Known view :
    
    y = 0;
        
    mKnownFixedRadio->setGeometry(m, y, r.width() - 2*m, lineH);
    mKnownFixedLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownFixedEdit->setGeometry(w1 + 2*m, y, w2, lineH);
    
    mKnownUniformRadio->setGeometry(m, y += (lineH + m), r.width() - 2*m, lineH);
    mKnownStartLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownStartEdit->setGeometry(w1 + 2*m, y, w2, lineH);
    mKnownEndLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownEndEdit->setGeometry(w1 + 2*m, y, w2, lineH);
    
    mKnownGaussRadio->setGeometry(m, y += (lineH + m), r.width() - 2*m, lineH);
    mKnownGaussMeasureLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownGaussMeasure->setGeometry(w1 + 2*m, y, w2, lineH);
    mKnownGaussErrorLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownGaussError->setGeometry(w1 + 2*m, y, w2, lineH);
    
    mKnownGraph->setGeometry(m, y += (lineH + m), r.width() - 2*m, 100);
}

