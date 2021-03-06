/***********************************************************************
**
**   polardialog.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by Eggert Ehmke
**                   2008-2010 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/**
 * \class PolarDialog
 *
 * \author Eggert Ehmke, Axel Pauli
 *
 * \brief Class to handle glider polar settings.
 *
 * \date 2002-2010
 *
 */

#ifndef POLAR_DIALOG_H
#define POLAR_DIALOG_H

#include <QWidget>

#include "speed.h"
#include "polar.h"

class PolarDialog : public QWidget
{
  Q_OBJECT

private:

  Q_DISABLE_COPY ( PolarDialog )

public:

  PolarDialog( Polar&, QWidget* );

  virtual ~PolarDialog();

public slots:

  void slot_keyup();
  void slot_keydown();
  void slot_shiftkeyup();
  void slot_shiftkeydown();
  void slot_keyleft();
  void slot_keyright();
  void slot_keyhome();
  void slot_keyreturn();

protected:

  virtual void paintEvent (QPaintEvent*);
  virtual void mousePressEvent( QMouseEvent* );

private:

  Polar _polar;
  Speed wind;
  Speed lift;
  Speed mc;
};

#endif
