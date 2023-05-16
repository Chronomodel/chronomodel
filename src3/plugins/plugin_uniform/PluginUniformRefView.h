/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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

#ifndef PLUGINUNIFORMREFVIEW_H
#define PLUGINUNIFORMREFVIEW_H
#if USE_PLUGIN_TL
#include "../GraphViewRefAbstract.h"

class PluginUniform;
class GraphView;
class QVBoxLayout;

class PluginUniformRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginUniformRefView(QWidget* parent = nullptr);
    virtual ~PluginUniformRefView();

    void setDate(const Date& date, const ProjectSettings& settings);

public slots:
    void zoomX(const double min, const double max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent* e);

  
};

#endif
#endif
