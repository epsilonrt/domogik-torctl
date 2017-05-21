/**
 * Copyright © 2016-2017 epsilonRT, All rights reserved.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * <http://www.cecill.info>.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 *
 * @file
 * @brief Configurable device side
 */
#include <string.h>
#include <sysio/string.h>
#include "dmg-torctl.h"
#include "config.h"

/* private functions ======================================================== */
// --------------------------------------------------------------------------
//  It's best to put the logic for reading the device configuration
//  and parsing it into your code in a seperate function so it can
//  be used by your prvDeviceConfigChanged and your startup code that
//  will want to parse the same data after a setting file is loaded
void
prvDeviceSetConfig (gxPLDevice * device) {
  const char * value;
  char * buffer = NULL;
  char * str, * p;

  for (int i = 0; i < gxPLDeviceConfigValueCount (device, DMG_TORCTL_CONFIG_NAME); i++) {

    value = gxPLDeviceConfigValueGetAt (device, DMG_TORCTL_CONFIG_NAME, i);
    if (value) {
      size_t s;
      struct xTorCtlOutput * out = &xCtx.xOutput[i];

      buffer = realloc (buffer, strlen (value) + 1);
      assert (buffer);

      p = buffer;
      strcpy (p, value);

      /*
       * découpage des strings de config:
       * name[:type:on_value:off_value[:adco]]
       * on_value et/ou off_value peuvent être complémentées avec !
       */
      str = strsep (&p, ":");
      s = sizeof (out->sName) - 1 ;
      strncpy (out->sName, str, s);
      out->sName[s] = '\0';

      out->eCurrentMode = eTorCtlManual;

      str = strsep (&p, ":");
      if ( (p) && strlen (str) ) {

        // type defined
        s = sizeof (out->sType) - 1 ;
        strncpy (out->sType, str, s);
        out->sType[s] = '\0';

        str = strsep (&p, ":");
        if ( (p) && strlen (str) ) {

          // on_value defined
          s = sizeof (out->sOnValue) - 1 ;
          strncpy (out->sOnValue, str, s);
          out->sOnValue[s] = '\0';

          str = strsep (&p, ":");
          if ( (p) && strlen (str) ) {

            // off_value defined
            s = sizeof (out->sOffValue) - 1 ;
            strncpy (out->sOffValue, str, s);
            out->sOffValue[s] = '\0';
            out->eCurrentMode = eTorCtlAuto;

            if (strlen (p) ) {
              long n;

              if (iStrToLong (p, &n, 0) == 0) {

                // adco defined
                out->ulAdco = n;
              }
            }
          }
          else {

            PWARNING ("Unable to setting up %s output, off_value not defined", out->sName);
          }
        }
        else {

          PWARNING ("Unable to setting up %s output, on_value not defined", out->sName);
        }
      }
    }
  }
  free (buffer);
}

// --------------------------------------------------------------------------
//  Handle a change to the device device configuration
static void
prvDeviceConfigChanged (gxPLDevice * device, void * udata) {

  gxPLMessageSourceInstanceIdSet (xCtx.xMsg, gxPLDeviceInstanceId (device) );

  // Read setting items for device and install
  prvDeviceSetConfig (device);
  // La config a été modifié, on transmet l'état et le mode des sorties
  iTorCtlSendStat (device);
}

/* internal public functions ================================================ */
// -----------------------------------------------------------------------------
// Create xPL application and device
gxPLDevice *
xDeviceCreate (gxPLSetting * setting) {
  gxPLApplication * app;
  gxPLDevice * device;

  // opens the xPL network
  app = gxPLAppOpen (setting);
  if (app == NULL) {

    return NULL;
  }

  // Initialize sensor device
  // Create a device for us
  // Create a configurable device and set our application version
  device = gxPLAppAddConfigurableDevice (app, DMG_TORCTL_VENDOR_ID,
                                         DMG_TORCTL_DEVICE_ID,
                                         gxPLConfigPath (DMG_TORCTL_CONFIG_FILENAME) );
  if (device == NULL) {

    return NULL;
  }

  gxPLDeviceVersionSet (device, DMG_TORCTL_DEVICE_VERSION);

  // If the configuration was not reloaded, then this is our first time and
  // we need to define what the configurables are and what the default values
  // should be.
  if (gxPLDeviceIsConfigured (device) == false) {
    const char * default_config[DMG_TORCTL_PORTSIZE] = DMG_TORCTL_CONFIG;

    // Define configurable items and give it a default
    gxPLDeviceConfigItemAdd (device, DMG_TORCTL_CONFIG_NAME, gxPLConfigReconf, DMG_TORCTL_PORTSIZE);

    for (int i = 0; i < DMG_TORCTL_PORTSIZE; i++) {

      gxPLDeviceConfigValueAdd (device, DMG_TORCTL_CONFIG_NAME, default_config[i]);
    }
  }

  // Create a sensor.basic message conforming to http://xplproject.org.uk/wiki/Schema_-_SENSOR.html
  xCtx.xMsg = gxPLDeviceMessageNew (device, gxPLMessageTrigger);
  assert (xCtx.xMsg);

  // Setting up the message
  gxPLMessageBroadcastSet (xCtx.xMsg, true);

  // Parse the device configurables into a form this program
  // can use (whether we read a setting or not)
  prvDeviceSetConfig (device);

  // Add a device change listener we'll use to pick up a new gap
  gxPLDeviceConfigListenerAdd (device, prvDeviceConfigChanged, NULL);

  return device;
}

/* ========================================================================== */
