##################################################################
# Cumulus Maemo 5.x project file for qmake
#
# Copyright (c): 2010-2013 Axel Pauli
#
# This file is distributed under the terms of the General Public
# License. See the file COPYING for more information.
#
# $Id$
#
##################################################################

TEMPLATE = app

# Put all generated objects into an extra directory
OBJECTS_DIR = .obj
MOC_DIR     = .obj

QT += xml

CONFIG = qt \
    warn_on \
    release

# CONFIG = debug qt warn_on

# The next 3 lines shall force a compilation of the date stamp file
rm_build_date.commands = rm -f $(OBJECTS_DIR)/builddate.o

QMAKE_EXTRA_TARGETS += rm_build_date

PRE_TARGETDEPS += rm_build_date

# Enable Flarm feature, if not wanted comment out the next line with a hash
CONFIG += flarm

# Enable Internet features, if not wanted comment out the next line with a hash
CONFIG +=internet

# Enable bluetooth feature, if not wanted comment out the next line with a hash
CONFIG += bluetooth

# Enable Welt2000 reloading via an extra thread, if not wanted comment out the
# next line with a hash
CONFIG += welt2000thread

# Enable classical menu bar, if define is set. Otherwise a context menu is used.
# DEFINES += USE_MENUBAR

# Activate this define, if Qt class QScroller is available.
# DEFINES += QSCROLLER

# Activate this feature, if class FlickCharm shall be used for kinetic finger scrolling.
# CONFIG += flickcharm

# Enable this feature, if the own number key pad shall be used for number input.
CONFIG += numberpad

HEADERS = \
    aboutwidget.h \
    airfieldlistview.h \
    airfieldlistwidget.h \
    airfield.h \
    airregion.h \
    airspace.h \
    airspacewarningdistance.h \
    altimetermodedialog.h \
    altitude.h \
    authdialog.h \
    basemapelement.h \
    configwidget.h \
    calculator.h \
    datatypes.h \
    distance.h \
    elevationcolorimage.h \
    filetools.h \
    flighttask.h \
    fontdialog.h \
    generalconfig.h \
    glider.h \
    glidereditor.h \
    gliderflightdialog.h \
    gliderlistwidget.h \
    gpscon.h \
    gpsnmea.h \
    gpsstatusdialog.h \
    coordedit.h \
    helpbrowser.h \
    hwinfo.h \
    igclogger.h \
    interfaceelements.h \
    ipc.h \
    isohypse.h \
    isolist.h \
    layout.h \
    limitedlist.h \
    lineelement.h \
    listviewfilter.h \
    logbook.h \
    maemostyle.h \
    mainwindow.h \
    mapcalc.h \
    mapconfig.h \
    mapcontents.h \
    mapdefaults.h \
    map.h \
    mapinfobox.h \
    mapmatrix.h \
    mapview.h \
    messagehandler.h \
    messagewidget.h \
    multilayout.h \
    openairparser.h \
    polardialog.h \
    polar.h \
    preflightwidget.h \
    preflightgliderpage.h \
    preflightmiscpage.h \
    preflightwaypointpage.h \
    preflighttasklist.h \
    projectionbase.h \
    projectioncylindric.h \
    projectionlambert.h \
    protocol.h \
    radiopoint.h \
    reachablelist.h \
    reachablepoint.h \
    reachpointlistview.h \
    resource.h \
    rowdelegate.h \
    runway.h \
    settingspageairfields.h \
    settingspageairspace.h \
    settingspageairspacefilling.h \
    settingspageairspaceloading.h \
    settingspageairspacewarnings.h \
    settingspagegps.h \
    settingspageglider.h \
    settingspageinformation.h \
    settingspagelooknfeel.h \
    settingspagemapobjects.h \
    settingspagemapsettings.h \
    settingspagepersonal.h \
    settingspagetask.h \
    settingspageterraincolors.h \
    settingspageunits.h \
    signalhandler.h \
    singlepoint.h \
    sonne.h \
    sound.h \
    speed.h \
    splash.h \
    target.h \
    taskeditor.h \
    tasklistview.h \
    taskpoint.h \
    time_cu.h \
    tpinfowidget.h \
    vario.h \
    variomodedialog.h \
    varspinbox.h \    
    vector.h \
    waitscreen.h \
    waypointcatalog.h \
    waypointlistview.h \
    waypointlistwidget.h \
    welt2000.h \
    wgspoint.h \
    whatsthat.h \
    windanalyser.h \
    windmeasurementlist.h \
    windstore.h \
    wpeditdialog.h \
    wpeditdialogpageaero.h \
    wpeditdialogpagegeneral.h \
    waypoint.h \
    wpinfowidget.h \
    listwidgetparent.h

SOURCES = \
    aboutwidget.cpp \
    airfieldlistview.cpp \
    airfieldlistwidget.cpp \
    airfield.cpp \
    airregion.cpp \
    airspace.cpp \
    altimetermodedialog.cpp \
    altitude.cpp \
    authdialog.cpp \
    basemapelement.cpp \
    builddate.cpp \
    configwidget.cpp \
    calculator.cpp \
    distance.cpp \
    elevationcolorimage.cpp \
    filetools.cpp \
    flighttask.cpp \
    fontdialog.cpp \
    generalconfig.cpp \
    glider.cpp \
    glidereditor.cpp \
    gliderflightdialog.cpp \
    gliderlistwidget.cpp \
    gpscon.cpp \
    gpsnmea.cpp \
    gpsstatusdialog.cpp \
    coordedit.cpp \
    helpbrowser.cpp \
    hwinfo.cpp \
    igclogger.cpp \
    ipc.cpp \
    isohypse.cpp \
    isolist.cpp \
    layout.cpp \
    lineelement.cpp \
    listviewfilter.cpp \
    logbook.cpp \
    maemostyle.cpp \
    main.cpp \
    mainwindow.cpp \
    mapcalc.cpp \
    mapconfig.cpp \
    mapcontents.cpp \
    map.cpp \
    mapinfobox.cpp \
    mapmatrix.cpp \
    mapview.cpp \
    messagehandler.cpp \
    messagewidget.cpp \
    openairparser.cpp \
    polar.cpp \
    polardialog.cpp \
    preflightwidget.cpp \
    preflightgliderpage.cpp \
    preflightmiscpage.cpp \
    preflightwaypointpage.cpp \
    preflighttasklist.cpp \
    projectionbase.cpp \
    projectioncylindric.cpp \
    projectionlambert.cpp \
    radiopoint.cpp \
    reachablelist.cpp \
    reachablepoint.cpp \
    reachpointlistview.cpp \
    rowdelegate.cpp \
    runway.cpp \
    settingspageairfields.cpp \
    settingspageairspace.cpp \
    settingspageairspacefilling.cpp \
    settingspageairspaceloading.cpp \
    settingspageairspacewarnings.cpp \
    settingspagegps.cpp \
    settingspageglider.cpp \
    settingspageinformation.cpp \
    settingspagelooknfeel.cpp \
    settingspagemapobjects.cpp \
    settingspagemapsettings.cpp \
    settingspagepersonal.cpp \
    settingspagetask.cpp \
    settingspageterraincolors.cpp \
    settingspageunits.cpp \
    signalhandler.cpp \
    singlepoint.cpp \
    sonne.cpp \
    sound.cpp \
    speed.cpp \
    splash.cpp \
    target.h \
    taskeditor.cpp \
    tasklistview.cpp \
    taskpoint.cpp \
    time_cu.cpp \
    tpinfowidget.cpp \
    vario.cpp \
    variomodedialog.cpp \
    varspinbox.cpp \
    vector.cpp \
    waitscreen.cpp \
    waypointcatalog.cpp \
    waypointlistview.cpp \
    waypointlistwidget.cpp \
    welt2000.cpp \
    wgspoint.cpp \
    whatsthat.cpp \
    windanalyser.cpp \
    windmeasurementlist.cpp \
    windstore.cpp \
    waypoint.cpp \
    wpeditdialog.cpp \
    wpeditdialogpageaero.cpp \
    wpeditdialogpagegeneral.cpp \
    wpinfowidget.cpp \
    listwidgetparent.cpp

flarm {
    HEADERS += flarm.h \
               flarmaliaslist.h \
               flarmbase.h \
               flarmdisplay.h \
               flarmlistview.h \
               flarmlogbook.h \
               flarmradarview.h \
               flarmwidget.h \
               preflightflarmpage.h
               
               
    SOURCES += flarm.cpp \
               flarmaliaslist.cpp \
               flarmbase.cpp \
               flarmdisplay.cpp \
               flarmlistview.cpp \
               flarmlogbook.cpp \
               flarmradarview.cpp \
               flarmwidget.cpp \
               preflightflarmpage.cpp               
               
    DEFINES += FLARM
}

internet {
    QT += network
    
    HEADERS += airspacedownloaddialog.h \
               downloadmanager.h \
               httpclient.h \
               proxydialog.h
                              
    SOURCES += airspacedownloaddialog.cpp \
               downloadmanager.cpp \
               httpclient.cpp \
               proxydialog.cpp
               
    DEFINES += INTERNET
}

bluetooth {
    DEFINES += BLUEZ
  
    HEADERS += bluetoothdevices.h
    
    SOURCES += bluetoothdevices.cpp
    
    LIBS += -lbluetooth
}

welt2000thread {
    DEFINES += WELT2000_THREAD
}

flickcharm {
    DEFINES += FLICK_CHARM
    
    HEADERS += flickcharm.h
    
    SOURCES += flickcharm.cpp
}

numberpad {
    DEFINES += USE_NUM_PAD

    HEADERS += coordeditnumpad.h \
    					 doubleNumberEditor.h \
    					 glidereditornumpad.h \
               numberEditor.h \
               numberInputPad.h \
               preflighttaskpage.h \
               settingspageairspacefillingnumpad.h \
               settingspageairspacewarningsnumpad.h
    
    SOURCES += coordeditnumpad.cpp \
    					 doubleNumberEditor.cpp \
    					 glidereditornumpad.cpp \
               numberEditor.cpp \
               numberInputPad.cpp \
               preflighttaskpage.cpp \
               settingspageairspacefillingnumpad.cpp \
               settingspageairspacewarningsnumpad.cpp
}

TARGET = cumulus

DESTDIR = .

INCLUDEPATH += ../ \
    /usr/lib/glib-2.0/include \
    /usr/include/glib-2.0 \
    /usr/include/dbus-1.0 \
    /usr/lib/dbus-1.0/include
    
DEFINES += MAEMO MAEMO5

LIBS += -lstdc++ \
    -losso \
    -llocation
    
TRANSLATIONS = cumulus_de.ts
