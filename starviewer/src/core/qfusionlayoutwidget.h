/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#ifndef UDG_QFUSIONLAYOUTWIDGET_H
#define UDG_QFUSIONLAYOUTWIDGET_H

#include <QWidget>
#include "ui_qfusionlayoutWidgetbase.h"

namespace udg {

class AnatomicalPlane;

/**
 * @brief The QFusionLayoutWidget class allows to request one of a few different standard layouts for fusion visualization.
 */
class QFusionLayoutWidget : public QWidget, private ::Ui::QFusionLayoutWidgetBase
{
    Q_OBJECT

public:
    explicit QFusionLayoutWidget(QWidget *parent = 0);
    virtual ~QFusionLayoutWidget();

public slots:
    /// Updates the icons of the 3x1 layout to match the given anatomical plane.
    void setCurrentAnatomicalPlane(const AnatomicalPlane &anatomicalPlane);

signals:
    /// Emitted when a 3x1 layout is requested.
    void layout3x1Requested();
    /// Emitted when a 3x3 layout is requested.
    void layout3x3Requested();

};

} // namespace udg

#endif // UDG_QFUSIONLAYOUTWIDGET_H
