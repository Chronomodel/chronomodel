/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE
	Komlan NOUKPOAPE

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

#ifndef PLUGINMAGFORM_H
#define PLUGINMAGFORM_H

#if USE_PLUGIN_AM
#include "PluginFormAbstract.h"

#include <QValidator>
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

class PluginMag;



class PluginMagForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginMagForm(PluginMag* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginMagForm();

    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();

    bool isValid();

protected slots:
    void updateOptions();

    void allIsValid();

private:
    QRadioButton* mIncRadio;
    QRadioButton* mDecRadio;
    QRadioButton* mFieldRadio;
    QRadioButton* mIDRadio;
    QRadioButton* mIFRadio;
    QRadioButton* mIDFRadio;

    QLabel* mIncLab;
    QLabel* mDecLab;
    QLabel* mAlpha95Lab;
    QLabel* mFieldLab;
    QLabel* mFieldErrorLab;
    QLabel* mMCMCIterationLab;

    QLabel* mRefILab;
    QLabel* mRefDLab;
    QLabel* mRefFLab;

    QLineEdit* mIncEdit;
    QLineEdit* mDecEdit;
    QLineEdit* mAlpha95Edit;
    QLineEdit* mFieldEdit;
    QLineEdit* mFieldErrorEdit;
    QLineEdit* mMCMCIterationEdit;

    QComboBox* mRefICombo;
    QComboBox* mRefDCombo;
    QComboBox* mRefFCombo;

   // QDoubleValidator *m9090Validator, *m90270Validator, *mRplusValidator;
    //QIntValidator* mZplusValidator;


};

#endif
#endif
