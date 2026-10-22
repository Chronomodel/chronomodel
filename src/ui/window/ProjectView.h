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

#ifndef PROJECTVIEW_H
#define PROJECTVIEW_H

#include "ModelCurve.h"
#include "ModelView.h"
#include "Tabs.h"

#include <QWidget>

class QStackedWidget;
class QTextEdit;
class QTabWidget;
class QVBoxLayout;

class ResultsView;
class Event;

class ProjectView: public QWidget
{
    Q_OBJECT

public:
    ModelView* mModelView;
    ResultsView* mResultsView;

    ProjectView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ProjectView();

    void setScreenDefinition();
    void resizeEvent(QResizeEvent* e);

    void setProject();
    
    void clearInterface();
    void resetInterface();

    void readSettings();
    void writeSettings();

    void newPeriod();

    void updateMultiCalibrationAndEventProperties();
    void eventsAreSelected();

    void setShowAllThumbs(bool show) {mModelView->setShowAllThumbs(show);}

public slots:
    void initResults(bool densitiesReady = false);
    void updateResults();
    void updateProject();
    void showModel();
    void showResults();
    void showLogTab(const int &i);
    
    void showLog();
    void showHelp(bool show);

    void applySettings(std::shared_ptr<ModelCurve> &model);
    void applyFilesSettings(std::shared_ptr<ModelCurve> &model);
    void updateResultsLog();
    
    void toggleCurve(bool toggle);

private:
    QStackedWidget* mStack;

    QWidget* mLogView;
    QVBoxLayout* mLogLayout;
    Tabs* mLogTabs;
    QTextEdit* mLogModelEdit;
    QTextEdit* mLogInitEdit;
    QTextEdit* mLogAdaptEdit;
    QTextEdit* mLogResultsEdit;


};

#endif
