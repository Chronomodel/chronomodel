/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <QWidget>
#include <QLineEdit>

class QStackedWidget;
class QSplitter;
class QGraphicsItem;
class QGraphicsView;
class QPushButton;
class LineEdit;
class QPropertyAnimation;
class QGraphicsScene;

class Project;
class ProjectSettings;
class ModelToolsView;
class EventsScene;
class PhasesScene;
class ChronocurveSettingsView;
class Event;
class PhasesList;
class ImportDataView;
class EventPropertiesView;
class Button;
class SceneGlobalView;
class ScrollCompressor;
class CalibrationView;
class MultiCalibrationView;
class Label;
class LineEdit;
class SwitchWidget;

class ModelView: public QWidget
{
    Q_OBJECT
public:
    ModelView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ModelView();

    void setProject(Project* project);
    Project* getProject() const;

    void calibrateAll(ProjectSettings newS);
    bool findCalibrateMissing();

    void resetInterface();
    void showHelp(bool show);

    void readSettings();
    void writeSettings();


public slots:
    void updateProject();
    void modifyPeriod();

    void updateMultiCalibration();

    void eventsAreSelected(); //connect with EventAreSelected
    //void phasesAreSelected();
    void noEventSelected();
    void applyAppSettings();

    void toggleChronocurve(bool toggle);
    void updateRightPanelTitle();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void keyPressEvent(QKeyEvent* event);

    void adaptStudyPeriodButton(const double& min, const double& max);
    void connectScenes();
    void disconnectScenes();

private slots:
    void togglePropeties();
    void showProperties();
    void hideProperties();
    void showImport();
    void showPhases();
    void slideRightPanel();
    void prepareNextSlide();

    void updateEventsZoom(const double prop);
    void exportEventsScene();

    void updatePhasesZoom(const double prop);
    void exportPhasesScene();

    void updateCalibration(const QJsonObject& date);
    void showCalibration(bool show);

    void showMultiCalib();

    void setSettingsValid(bool valid);

    void searchEvent();
    void createEventInPlace();
    void createEventKnownInPlace();
    void createPhaseInPlace();

private:
    void exportSceneImage(QGraphicsScene* scene);

private:
    QWidget* mTopWrapper;
    QWidget* mLeftWrapper;
    QWidget* mRightWrapper;

    // ------

    EventsScene* mEventsScene;
    QGraphicsView* mEventsView;

    SceneGlobalView* mEventsGlobalView;
    ScrollCompressor* mEventsGlobalZoom;

    QLineEdit* mEventsSearchEdit;
    QString mLastSearch;
    QVector<int> mSearchIds;
    int mCurSearchIdx;

    Button* mButNewEvent;
    Button* mButNewEventKnown;
    Button* mButDeleteEvent;
    Button* mButRecycleEvent;
    Button* mButExportEvents;
    Button* mButEventsOverview;
    Button* mButEventsGrid;
    Button* mButProperties;
    Button* mButMultiCalib;
    Button* mButImport;

    // ------

    ImportDataView* mImportDataView;
    EventPropertiesView* mEventPropertiesView;

    ChronocurveSettingsView* mChronocurveSettingsView;

    PhasesScene* mPhasesScene;
    QGraphicsView* mPhasesView;

    SceneGlobalView* mPhasesGlobalView;
    ScrollCompressor* mPhasesGlobalZoom;

    Button* mButNewPhase;
    Button* mButDeletePhase;
    Button* mButExportPhases;
    Button* mButPhasesOverview;
    Button* mButPhasesGrid;

    QPropertyAnimation* mAnimationHide;
    QPropertyAnimation* mAnimationShow;

    QWidget* mCurrentRightWidget;

    // ------

    CalibrationView* mCalibrationView;
    MultiCalibrationView* mMultiCalibrationView;

    QPropertyAnimation* mAnimationCalib;

    // ------
    double mTmin;
    double mTmax;

    //Button* mButModifyPeriod;
    QPushButton* mButModifyPeriod;
    SwitchWidget* mChronocurveWidget;

    Label* mLeftPanelTitle;
    Label* mRightPanelTitle;

    QRect mTopRect;
    QRect mHandlerRect;

    QRect mLeftRect;
    QRect mLeftHiddenRect;

    QRect mRightRect;
    QRect mRightHiddenRect;



private:
    Project* mProject;

    int mMargin;
    int mToolbarH;
    int mButtonWidth;
    int mButtonHeigth;
    double mSplitProp;
    int mHandlerW;
    bool mIsSplitting;
    bool mCalibVisible;
    bool mIsChronocurve;
};

#endif
