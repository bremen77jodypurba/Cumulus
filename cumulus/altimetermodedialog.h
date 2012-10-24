/***********************************************************************
**
**   altimetermodedialog.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2004      by Eckhard Voellm
**                   2008-2012 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/**
 * \class AltimeterModeDialog
 *
 * \author Eckhard Völlm, Axel Pauli
 *
 * \brief Dialog for altimeter user interaction.
 *
 * This dialog is the user interface for the altimeter settings.
 *
 * \date 2004-2012
 *
 * \version $Id$
 *
 */

#ifndef ALTIMETER_MODE_DIALOG_H
#define ALTIMETER_MODE_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QRadioButton>
#include <QSpinBox>

class Altitude;

class AltimeterModeDialog : public QDialog
{
  Q_OBJECT

private:

  Q_DISABLE_COPY ( AltimeterModeDialog )

public:

  AltimeterModeDialog( QWidget *parent );
  virtual ~AltimeterModeDialog();

  static QString mode2String();
  static int mode();

  /**
   * @return Returns the current number of instances.
   */
  static int getNrOfInstances()
  {
    return noOfInstances;
  };

protected:

  /** User has pressed the ok button. */
  void accept();

  /** User has pressed cancel button or timeout has occurred. */
  void reject();

private:

  /** Gets the initial data for all widgets which needs a start configuration. */
  void load();

  /** Starts resp. restarts the inactively timer. */
  void startTimer();

  /** Check for configuration changes. */
  bool changesDone();

  /** inactively timer control */
  QTimer* timeout;

  /** Altitude references */
  QRadioButton* _msl;
  QRadioButton* _agl;
  QRadioButton* _std;
  QRadioButton* _ahl;

  /** Altitude modes */
  int _mode; // 0: MSL,  1: STD,  2: AGL, 3: AHL

  /** Altitude units */
  int _unit;     // 0: Meter,  1: Feet,  2: FL
  QRadioButton* _meter;
  QRadioButton* _feet;

  /** Altitude reference. */
  int _ref; // 0: Gps, 1: Baro
  QRadioButton* _gps;
  QRadioButton* _baro;

  /** Altitude display */
  QLabel* _altitudeDisplay;

  /** Altitude gain display */
  QLineEdit* altitudeGainDisplay;

  /** Spin box for altitude leveling */
  QSpinBox* spinLeveling;

  /** Spin box for QNH setting */
  QSpinBox* spinQnh;

  /** Setup buttons */
  QPushButton *plus;
  QPushButton *pplus;
  QPushButton *minus;
  QPushButton *mminus;

  QPushButton *setAltitudeGain;

  /** Save the initial values here. They are needed in the reject case. */
  int m_saveMode;
  int m_saveUnit;
  int m_saveRef;
  int m_saveQnh;
  int m_saveLeveling;

  /** Auto sip flag storage. */
  bool m_autoSip;

  /** contains the current number of class instances */
  static int noOfInstances;

public slots:

  /** This slot is being called if the altitude value has been changed. */
  void slotAltitudeChanged(const Altitude& altitude );

  /** This slot is being called if the altitude gain value has been changed. */
  void slotAltitudeGain(const Altitude& altitudeGain );

private slots:

  /**
   * This slot is called if the altitude reference has been changed. It
   * restarts too the close timer.
   */
  void slotModeChanged( int mode );

  /**
   * This slot is called if the altitude unit has been changed. It
   * restarts too the close timer.
   */
  void slotUnitChanged( int unit );

  /**
   * This slot is called if the altitude reference has been changed. It
   * restarts too the close timer.
   */
  void slotReferenceChanged( int reference );

  /**
   * This slot is called if a value in a spin box has been changed
   * to restart the close timer.
   */
  void slotSpinValueChanged( const QString& text );

  /**
   * This slot is called if a button is pressed to change the content of the
   * related spin box which has the current focus.
   */
  void slotChangeSpinValue();

  /**
   * This slot is called if the S button is pressed. It resets the gained
   * altitude display and informs the calculator about that.
   */
  void slotResetGainedAltitude();

signals:

  /** Emitted, if the altimeter mode has been changed. */
  void newAltimeterMode();

  /** Emitted, if the altimeter QNH resp. leveling have been changed. */
  void newAltimeterSettings();

  /**
   * This signal is emitted, when the dialog is closed
   */
  void closingWidget();
};

#endif
