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

#ifndef CurveSETTINGSVIEW_H
#define CurveSETTINGSVIEW_H

#include <QWidget>
#include "CurveSettings.h"

class QLabel;
class QComboBox;
class QCheckBox;
class QLineEdit;
class Project;


class CurveSettingsView: public QWidget
{
    Q_OBJECT
public:
    CurveSettingsView(QWidget* parent = nullptr);
    virtual ~CurveSettingsView();

    void setSettings(const CurveSettings& settings);
    CurveSettings getSettings() const;
    
    void setProject();
    void setConnections(const bool doConnections);

signals:
    void newProcess(QString str);
    
protected slots:
    void updateVisibilities();
    void reset();
    void save();
    void saveDeferred();

private:

    bool mEnabled;
    bool mConnectionsActive = false; // vrai entre setConnections(true) et setConnections(false)
    unsigned mConnectionsGeneration = 0; // invalide les save() différés en attente
    
    QLabel* mTitleLabel;
    QLabel* mDescriptionLabel;
    
    QLabel* mProcessTypeLabel;
    QComboBox* mProcessTypeInput;

    QLabel* mThresholdLabel;
    QLineEdit* mThresholdInput;

    QLabel* mUseErrMesureLabel;
    QCheckBox* mUseErrMesureInput;
    
    QLabel* mTimeTypeLabel;
    QComboBox* mTimeTypeInput;
    
    QLabel* mVarianceTypeLabel;
    QComboBox* mVarianceTypeInput;
    
    QLabel* mVarianceValueLabel;
    QLineEdit* mVarianceValueInput;
    
    QLabel* mLambdaSplineTypeLabel;
    QComboBox* mLambdaSplineTypeInput;
    
    QLabel* mLambdaSplineLabel;
    QLineEdit* mLambdaSplineInput;

    bool validateAndConvertDouble(const QString& text, double& result, const QString& fieldName) const;
};

#endif
