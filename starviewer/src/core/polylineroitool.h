/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGPOLYLINEROITOOL_H
#define UDGPOLYLINEROITOOL_H

#include "roitool.h"

namespace udg {

/**
    Tool per calcular l'àrea i la mitjana de grisos d'un polígon fet amb ROITool

	@author Grup de Gràfics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class PolylineROITool : public ROITool {
Q_OBJECT
public:
    PolylineROITool( QViewer *viewer, QObject *parent = 0 );
    ~PolylineROITool();

private slots:
    /// Fa els càlculs pertinents de la tool després d'haver rebut un signal de figura acabada
    void start();

private:
    /// Mètode per escriure a pantalla les dades calculades.
    void printData();

    /// Mètode per buscar els valors de gris que serviran per calcular la mitjana i la desviació estàndard
    void computeGrayValues();

    /// Mètode per calcular la mitjana de gris de la regió del polyline
    double computeGrayMean();

    /// Mètode per calcular la desviació estàndard de gris de la regió del polyline
    double computeStandardDeviation();

    /// Llista amb els valors de gris per calcular la mitjana i la desviació etàndard
    QList< double > grayValues;
};

}

#endif
