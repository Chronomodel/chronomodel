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

#ifndef PLUGINF14CFORM_H
#define PLUGINF14CFORM_H


#if USE_PLUGIN_F14C
#include <QValidator>
#include "../PluginFormAbstract.h"

class PluginF14C;
class QLineEdit;
class QComboBox;
class QLabel;


class PluginF14CForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginF14CForm(PluginF14C* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginF14CForm();

    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();

    bool isValid();
signals:
    //void OkEnabled(bool enabled) ;

protected slots:
    void errorIsValid(QString str);
    void valueIsValid(QString str);

private:
    QLabel* mAverageLab;
    QLabel* mErrorLab;

    QLabel* mRefLab;

    QLineEdit* mAverageEdit;
    QLineEdit* mErrorEdit;
    QDoubleValidator* mRValidator;
    QDoubleValidator* mRplusValidator;

    QComboBox* mRefCombo;

    static QString mSelectedRefCurve;
};

#endif

#endif
