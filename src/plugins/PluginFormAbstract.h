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

#ifndef PLUGINFORMABSTRACT_H
#define PLUGINFORMABSTRACT_H

#include <QString>
#include <QGroupBox>

class PluginAbstract;


class PluginFormAbstract: public QGroupBox
{
    Q_OBJECT
public:
    PluginAbstract* mPlugin;
    QString mError;


    PluginFormAbstract(PluginAbstract* plugin, const QString& title, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget):QGroupBox(/*title, */parent),
        mPlugin(plugin)
    {
        (void) title;
        (void) flags;
    }

    virtual ~PluginFormAbstract(){}

    virtual void setData(const QJsonObject& data, bool isCombined) = 0;
    virtual QJsonObject getData() = 0;

    virtual bool isValid() = 0;

signals:
   void OkEnabled(bool enabled) ;
   void sizeChanged();


};

#endif
