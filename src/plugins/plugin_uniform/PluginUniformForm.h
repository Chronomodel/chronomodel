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

#ifndef PLUGINUNIFORMFORM_H
#define PLUGINUNIFORMFORM_H

#if USE_PLUGIN_UNIFORM

#include "PluginFormAbstract.h"

class PluginUniform;
class QLabel;
class QLineEdit;


class PluginUniformForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginUniformForm(PluginUniform* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginUniformForm();

    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();

    bool isValid();

protected slots:
    void valuesAreValid(QString str);

private:
    QLabel* mMinLab;
    QLabel* mMaxLab;

    QLineEdit* mMinEdit;
    QLineEdit* mMaxEdit;
};

#endif
#endif
