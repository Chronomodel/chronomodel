/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef MODELVIEW_H
#define MODELVIEW_H

#include "AbstractItem.h"
#include "EventsScene.h"
#include "PhasesScene.h"

#include <QWidget>
#include <QLineEdit>
#include <QShortcut>

class QStackedWidget;
class QSplitter;
class QGraphicsItem;
class QGraphicsView;
class QPushButton;
class LineEdit;
class QPropertyAnimation;
class QGraphicsScene;

class Project;
class StudyPeriodSettings;
class ModelToolsView;

class CurveSettingsView;
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
private:
    QWidget* mTopWrapper;
    QWidget* mLeftWrapper;
    QWidget* mRightWrapper;

    // ------

    EventsScene* mEventsScene;
    QGraphicsView* mEventsView;

    QLineEdit* mEventsSearchEdit;
    SceneGlobalView* mEventsGlobalView;

    ScrollCompressor* mEventsGlobalZoom;

    QString mLastSearch;
    QList<int> mSearchIds;
    int mCurSearchIdx;

    Button* mButNewEvent;
    Button* mButNewEventKnown;
    Button* mButDeleteEvent;
    QShortcut* mDelShortcut;

    Button* mButRecycleEvent;
    Button* mButImageOfEventScene;
    Button* mButEventsGlobalView;
    Button* mButEventsGrid;
    Button* mButProperties;
    Button* mButMultiCalib;
    Button* mButCsv;

    // ------

    ImportDataView* mImportDataView;
    EventPropertiesView* mEventPropertiesView;

    CurveSettingsView* mCurveSettingsView;

    PhasesScene* mPhasesScene;
    QGraphicsView* mPhasesView;

    SceneGlobalView* mPhasesGlobalView;
    ScrollCompressor* mPhasesGlobalZoom;

    Button* mButNewPhase;
    Button* mButDeletePhase;
    Button* mButExportPhases;
    Button* mButPhasesGlobaliew;
    Button* mButPhasesGrid;

    QPropertyAnimation* mAnimationHide;
    QPropertyAnimation* mAnimationShow;

    QWidget* mCurrentRightWidget;

    // ------

    CalibrationView* mCalibrationView;
    MultiCalibrationView* mMultiCalibrationView;


    //QPropertyAnimation* mAnimationCalib;

    // ------
    double mTmin;
    double mTmax;

    QPushButton* mButModifyPeriod;
    Button* mButCurve;

    Label* mLeftPanelTitle;
    Label* mRightPanelTitle;

    QRect mTopRect;
    QRect mHandlerRect;

    QRect mLeftRect;
    QRect mLeftHiddenRect;

    QRect mRightRect;
    QRect mRightHiddenRect;

    int mMargin;
    int mToolbarH;
    int mButtonWidth;
    int mButtonHeigth;
    double mSplitProp;
    int mHandlerW;
    bool mIsSplitting;
    bool mCalibVisible;
    bool mCurveSettingsVisible;

public:
    ModelView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ModelView();

    void setProject();

    void calibrateAll(StudyPeriodSettings newS);
    bool findCalibrateMissing();

    void clearInterface();
    void resetInterface();
    void showHelp(bool show);

    void readSettings();
    void writeSettings();

    void setShowAllThumbs(bool show) { mEventsScene->setShowAllThumbs(show);
        mPhasesScene->setShowAllEvents(show);};

public slots:
    void updateProject();
    void modifyPeriod();
    void updateCurveButton();

    void updateMultiCalibrationAndEventProperties();

    void eventsAreSelected(); //connect with EventAreSelected

    void noEventSelected();
    void applyAppSettings();

    void showCurveSettings(bool show);
    void updateRightPanelTitle();

    void togglePropeties(AbstractItem *item);
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

private:
    void exportSceneImage(QGraphicsScene* scene);


};

#endif
