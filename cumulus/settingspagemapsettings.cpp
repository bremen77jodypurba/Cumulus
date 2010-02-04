/***********************************************************************
**
**   settingspagemapsettings.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by André Somers
**                   2008-2010 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
************************************************************************
**
** Contains the map projection related settings
**
** @author André Somers
**
***********************************************************************/

#include <QtCore>
#include <QtGui>

#include "settingspagemapsettings.h"
#include "generalconfig.h"
#include "mapcontents.h"
#include "distance.h"

/***********************************************************/
/*  map setting page                                       */
/***********************************************************/

SettingsPageMapSettings::SettingsPageMapSettings(QWidget *parent) :
  QWidget(parent)
{
  setObjectName("SettingsPageMapSettings");
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  currentProjType = ProjectionBase::Unknown;
  GeneralConfig *conf = GeneralConfig::instance();
  QGridLayout *topLayout = new QGridLayout(this);

  int row=0;

  mapSelection = new QPushButton( tr("Maps"), this );
  mapSelection->setToolTip(tr("Select your personal map directory"));
  topLayout->addWidget(mapSelection, row, 0 );

  connect(mapSelection, SIGNAL( clicked()), this, SLOT(slot_openFileDialog()) );

  mapDirectory = new QLineEdit( this );
  topLayout->addWidget(mapDirectory, row++, 1, 1, 2 );

  topLayout->addWidget(new QLabel(tr("Projection:"), this), row, 0 );
  cmbProjection = new QComboBox(this);
  topLayout->addWidget(cmbProjection, row++, 1);
  cmbProjection->addItem(tr("Lambert"));
  cmbProjection->addItem(tr("Plate Carrée")); // Qt::Key_Eacute

  connect(cmbProjection, SIGNAL(activated(int)),
          this, SLOT(slot_selectProjection(int)));

  topLayout->addWidget(new QLabel(tr("1. St. Parallel:"), this), row, 0);
  edtLat1=new LatEdit(this, conf->getHomeLat());
  topLayout->addWidget(edtLat1, row++, 1, 1, 2);

  edtLat2Label = new QLabel(tr("2. St. Parallel:"), this);
  topLayout->addWidget(edtLat2Label, row, 0);
  edtLat2 = new LatEdit(this, conf->getHomeLat());
  topLayout->addWidget(edtLat2, row++, 1, 1, 2);

  edtLonLabel = new QLabel(tr("Origin Longitude:"), this);
  topLayout->addWidget(edtLonLabel, row, 0);
  edtLon = new LongEdit(this, conf->getHomeLon());
  topLayout->addWidget(edtLon, row++, 1, 1, 2);

  //------------------------------------------------------------------------------

  topLayout->setRowMinimumHeight(row++,15);

  chkProjectionFollowHome = new QCheckBox(tr("Projection follows Home Position"), this );
  topLayout->addWidget(chkProjectionFollowHome, row, 0, 1, 2);
  row++;

  chkUnloadUnneeded = new QCheckBox(tr("Unload unused maps from RAM"), this );
  topLayout->addWidget(chkUnloadUnneeded, row, 0, 1, 2);
  row++;

  topLayout->setRowMinimumHeight(row++,15);

  QLabel *label = new QLabel(tr("Proxy : Port"), this);
  topLayout->addWidget(label, row, 0);
  proxy = new QLineEdit(this);
  proxy->setToolTip(tr("Enter Proxy data if needed"));
  topLayout->addWidget(proxy, row, 1, 1, 2);
  row++;

  label = new QLabel(tr("Center Latitude:"), this);
  topLayout->addWidget(label, row, 0);
  edtCenterLat = new LatEdit(this, conf->getHomeLat());
  topLayout->addWidget(edtCenterLat, row++, 1, 1, 2);

  label = new QLabel(tr("Center Longitude:"), this);
  topLayout->addWidget(label, row, 0);
  edtCenterLon = new LongEdit(this, conf->getHomeLon());
  topLayout->addWidget(edtCenterLon, row++, 1, 1, 2);

  installMaps = new QPushButton( tr("Install Maps"), this );
  installMaps->setToolTip(tr("Install maps around center point"));
  topLayout->addWidget(installMaps, row, 0 );

  connect(installMaps, SIGNAL( clicked()), this, SLOT(slot_installMaps()) );

  installRadius = new QSpinBox( this );
  installRadius->setToolTip( tr("Radius around center point") );
  installRadius->setButtonSymbols(QSpinBox::PlusMinus);
  installRadius->setRange( 0, 20000 );
  installRadius->setWrapping(true);
  installRadius->setSingleStep( 100 );
  installRadius->setValue( GeneralConfig::instance()->getMapInstallRadius() );
  installRadius->setSuffix( Distance::getUnitText() );

  topLayout->addWidget(installRadius, row++, 1 );

  topLayout->setColumnStretch( 2, 10 );
  topLayout->setRowStretch( row, 10 );
}

SettingsPageMapSettings::~SettingsPageMapSettings()
{}

void SettingsPageMapSettings::slot_load()
{
  GeneralConfig *conf = GeneralConfig::instance();

  // Take the first directory from the search order
  mapDirectory->setText( conf->getMapDirectories()[0] );

  chkUnloadUnneeded->setChecked( conf->getMapUnload() );
  chkProjectionFollowHome->setChecked( conf->getMapProjectionFollowsHome() );
  proxy->setText( conf->getProxy() );
  edtCenterLat->setKFLogDegree(conf->getHomeLat());
  edtCenterLon->setKFLogDegree(conf->getHomeLon());

  currentProjType = conf->getMapProjectionType();
  lambertV1 =       conf->getLambertParallel1();
  lambertV2 =       conf->getLambertParallel2();
  lambertOrigin =   conf->getLambertOrign();
  cylinPar =        conf->getCylinderParallel();

  // @AP: Note, that the index of the list starts with 0 but the
  // ProjectionType uses zero for unknown. So we must subtract 1
  // to get the right value.
  int projIndex = currentProjType - 1;
  cmbProjection->setCurrentIndex(projIndex);
  slot_selectProjection(projIndex);
}


void SettingsPageMapSettings::slot_save()
{
  // @AP: here we must overtake the new user values at first. After that
  // we can store them.
  // Check, if input string values have been changed. If not, no
  // overtake of values to avoid rounding errors. They can appear if
  // the position formats will be changed between DMS <-> DDM vice
  // versa.

  switch(cmbProjection->currentIndex())
    {

    case 0:

      if( edtLat1->isInputChanged() )
        {
          lambertV1 = edtLat1->KFLogDegree();
        }

      if( edtLat2->isInputChanged() )
        {
          lambertV2 = edtLat2->KFLogDegree();
        }

      if( edtLon->isInputChanged() )
        {
          lambertOrigin = edtLon->KFLogDegree();
        }

      break;

    case 1:

      if( edtLat1->isInputChanged() )
        {
          cylinPar = edtLat1->KFLogDegree();
        }

      break;
    }

  GeneralConfig *conf = GeneralConfig::instance();

  conf->setMapRootDir( mapDirectory->text() );
  conf->setMapUnload( chkUnloadUnneeded->isChecked() );
  conf->setMapProjectionFollowsHome( chkProjectionFollowHome->isChecked() );
  conf->setProxy( proxy->text() );
  conf->setMapInstallRadius( installRadius->value() );
  conf->setMapProjectionType( currentProjType );
  conf->setLambertParallel1( lambertV1 );
  conf->setLambertParallel2( lambertV2 );
  conf->setLambertOrign( lambertOrigin );
  conf->setCylinderParallel( cylinPar );
}

/**
 * Called if the install maps button is pressed
 */
void SettingsPageMapSettings::slot_installMaps()
{
  if( installRadius->value() == 0 )
    {
      // Radius distance is zero, ignore request.
      return;
    }

  int answer = QMessageBox::question( this, tr("Download Maps?"),
      tr("Active Internet connection is needed!") +
      QString("<p>") + tr("Start download now?"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

  if( answer == QMessageBox::No )
    {
      return;
    }

  QString hostName;
  quint16 port;

  // Check proxy settings
  if( ! proxy->text().trimmed().isEmpty() &&
      HttpClient::parseProxy( proxy->text(), hostName, port ) == false )
    {
      QMessageBox::information ( this,
                                 tr("Proxy settings invalid!"),
                                 tr("Please correct your Proxy settings!") +
                                 "<p>" + tr("Expected format: <b>Host:Port</b>") );
      return;
    }

  // Store the proxy settings, can be changed in the meantime
  GeneralConfig::instance()->setProxy( proxy->text().trimmed() );
  Distance distance( Distance::convertToMeters( installRadius->value() ));
  QPoint center;
  center.setX( edtCenterLat->KFLogDegree() );
  center.setY( edtCenterLon->KFLogDegree() );

  emit downloadMapArea( center, distance );
}

/**
 * Called if the map selection button is pressed
 */
void SettingsPageMapSettings::slot_openFileDialog()
{
  QString mapDirCurrent = GeneralConfig::instance()->getMapDirectories()[0];
  QDir mapDir;

  if( ! mapDir.exists( mapDirCurrent ) )
      {
        // Fall back to default if not existing
        mapDirCurrent = QDir::homePath();
      }

  QString mapDirNew = QFileDialog::getExistingDirectory( this,
                                                         tr("Please select your map directory"),
                                                         mapDirCurrent,
                                                         QFileDialog::ShowDirsOnly );
  if( mapDirNew.isEmpty() )
    {
      return; // cancel was selected by the user
    }

  mapDirectory->setText( mapDirNew );

  // Now check, if all needed map subdirectories are exist under the new map root
  QStringList missingDirs;

  if( ! mapDir.exists( mapDirNew + "/airfields" ))
    {
      missingDirs << "airfields";
    }

  if( ! mapDir.exists( mapDirNew + "/airspaces" ))
    {
      missingDirs << "airspaces";
    }

  if( ! mapDir.exists( mapDirNew + "/landscape" ))
    {
      missingDirs << "landscape";
    }

  if( missingDirs.size() != 0 )
    {
      // map subdirectories are missing, ask user for creation
        int answer = QMessageBox::question( this, tr("Map Subdirectories?"),
            tr("Missing Map subdirectories:") + QString("<p>") +
            missingDirs.join(", ") +
            QString("<p>") + tr("Shall they be created now?"),
            QMessageBox::Yes | QMessageBox::No );

      if( answer == QMessageBox::Yes )
        {
          for( int i = 0; i < missingDirs.size(); i++ )
            {
              mapDir.mkpath( mapDirNew + "/" + missingDirs.at(i) );
            }
        }
    }
}

// selection in the combo box has been changed. index is a reference
// to the current entry. initialize widgets with the internal values
// normally read from the configuration file.
void SettingsPageMapSettings::slot_selectProjection(int index)
{

  switch (index)
    {
      case 0: // Lambert

        edtLat2Label->setVisible(true);
        edtLat2->setVisible(true);
        edtLonLabel->setVisible(true);
        edtLon->setVisible(true);
        chkProjectionFollowHome->setVisible(false);
        edtLat1->setKFLogDegree(lambertV1);
        edtLat2->setKFLogDegree(lambertV2);
        edtLon->setKFLogDegree(lambertOrigin);
        currentProjType = ProjectionBase::Lambert;
        break;

      case 1: // Plate Carreé
      default: // take this if index is unknown

        edtLat2Label->setVisible(false);
        edtLat2->setVisible(false);
        edtLonLabel->setVisible(false);
        edtLon->setVisible(false);
        chkProjectionFollowHome->setVisible(true);
        edtLat1->setKFLogDegree(cylinPar);
        edtLat2->setKFLogDegree(0);
        edtLon->setKFLogDegree(0);
        currentProjType = ProjectionBase::Cylindric;
        break;
    }
}

/* Called to ask is confirmation on the close is needed. */
void SettingsPageMapSettings::slot_query_close(bool& warn, QStringList& warnings)
{
  /* set warn to 'true' if the data has changed. Note that we can NOT
    just set warn equal to _changed, because that way we might erase a
    warning flag set by another page! */

  bool changed = false;
  GeneralConfig *conf = GeneralConfig::instance();

  changed |= ( mapDirectory->text() != conf->getMapRootDir() );
  changed |= ( chkUnloadUnneeded->isChecked() != conf->getMapUnload() );
  changed |= ( chkProjectionFollowHome->isChecked() != conf->getMapProjectionFollowsHome() );
  changed |= ( installRadius->value() != conf->getMapInstallRadius() );
  changed |= (proxy->text() != conf->getProxy());
  changed |= checkIsProjectionChanged();

  if (changed)
    {
      warn = true;
      warnings.append(tr("The Map Settings"));
    }
}

/**
 * Checks, if the configuration of the projection has been changed
 */
bool SettingsPageMapSettings::checkIsProjectionChanged()
{
  bool changed = false;
  GeneralConfig *conf = GeneralConfig::instance();

  switch( cmbProjection->currentIndex() )
    {
      case 0:
        changed |= ( edtLat1->isInputChanged() );
        changed |= ( edtLat2->isInputChanged() );
        changed |= ( edtLon->isInputChanged() );
        break;

      case 1:
        changed |= ( edtLat1->isInputChanged() );
        break;
    }

  changed |= ( conf->getMapProjectionType() != currentProjType );

  // qDebug( "SettingsPageMapSettings::()checkIsProjectionChanged: %d", changed );
  return changed;
}
