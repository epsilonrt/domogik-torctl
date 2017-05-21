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
 * @brief
 */
#include <math.h>
#include <string.h>
#include <sysio/string.h>
#include "dmg-torctl.h"
#include "config.h"

/* constants ================================================================ */
/*
 * Description des broches de sorties
 */
static const xDout xMyPins[DMG_TORCTL_PORTSIZE] = DMG_TORCTL_PORTPINS;

/* private functions ======================================================== */
// -----------------------------------------------------------------------------
static int
priSendCurrentValue (gxPLDevice * device, unsigned i, bool bSendMode, gxPLMessageType type) {
  gxPLMessage * m = xCtx.xMsg;
  bool bValue = (bSendMode) ? (xCtx.xOutput[i].eCurrentMode == eTorCtlAuto) :
                (xCtx.xOutput[i].iValue != false);

  gxPLMessageTypeSet (m, type);
  gxPLMessageSchemaSet (m, "sensor", "basic");
  gxPLMessageBodyClear (m);
  if (bSendMode) {

    gxPLMessagePairAddFormat (m, "device", "%s-auto", xCtx.xOutput[i].sName);
  }
  else {

    gxPLMessagePairAdd (m, "device", xCtx.xOutput[i].sName);
  }
  gxPLMessagePairAdd (m, "type", "output" );
  gxPLMessagePairAdd (m, "current", bValue ? "1" : "0");
  return gxPLDeviceMessageSend (device, m);
}

// -----------------------------------------------------------------------------
// copie de device avec détection du suffixe "-auto"
static char *
prvvGetOutputName (const char * strdev, bool * bModeRequested) {
  char * dev;
  char * dash = strrchr (strdev, '-');
  size_t len = strlen (strdev) + 1;
  *bModeRequested = false;

  if (dash) {
    if (strcmp (dash, "-auto") == 0) {

      *bModeRequested = true;
      len -= strlen (dash);
    }
  }

  dev = malloc (len);
  assert (dev);

  strncpy (dev, strdev, len);
  if (dash) {

    dev[len - 1] = '\0';
  }
  return dev;
}

// -----------------------------------------------------------------------------
// Demande de status
static void
prvvSensorRequestListener (gxPLDevice * device, gxPLMessage * msg, void * udata) {
  /*  --- Message reçu ---
        XPL-CMND
        sensor.request
        {
        request=current
        [device=<output_name/output_name-auto>] if not existing, all devices will be sent
        [type=<output>] if not existing, output state will be sent
        }
      --- Réponse ---
        XPL-STAT
        sensor.basic
        {
        device=<output_name/output_name-auto>
        type=<output>
        current=<0/1>
        }
   */
  if (gxPLMessagePairExist (msg, "request") ) {
    const char * request = gxPLMessagePairGet (msg, "request");

    if (strcmp (request, "current") == 0) {
      const char * type = NULL;
      char * output = NULL;
      bool bModeRequested = false;

      if (gxPLMessagePairExist (msg, "type") ) {

        type = gxPLMessagePairGet (msg, "type");
        if (strcmp (type, "output") != 0) {

          // Type non pris en charge
          return;
        }
      }

      if (gxPLMessagePairExist (msg, "device") ) {

        output = prvvGetOutputName (gxPLMessagePairGet (msg, "device"), &bModeRequested);
      }

      for (unsigned i = 0; i < iDoutPortSize (xCtx.xOutPort); i++) {
        struct xTorCtlOutput * out = &xCtx.xOutput[i];

        if (output) {

          if (strcmp (output, out->sName) == 0) {

            // on envoie l'état ou le mode de la sortie demandée
            if (priSendCurrentValue (device, i, bModeRequested, gxPLMessageStatus) < 0) {

              PWARNING ("Unable to send current value for %s", out->sName);
            }
          }
        }
        else {

          // On doit envoyé l'état et le mode de toutes les sorties
          if (priSendCurrentValue (device, i, false, gxPLMessageStatus) < 0) {

            PWARNING ("Unable to send current value for %s", out->sName);
          }
          if (priSendCurrentValue (device, i, true, gxPLMessageStatus) < 0) {

            PWARNING ("Unable to send current mode for %s", out->sName);
          }
        }
      }
      free (output);
    }
  }
}

// -----------------------------------------------------------------------------
// Ordre de changement d'état ou de mode
static void
prvvControlBasicListener (gxPLDevice * device, gxPLMessage * msg, void * udata) {
  /*  --- Message reçu ---
        XPL-CMND Structure
        control.basic
        {
        device=<output_name/output_name-auto>
        type=<output>
        current=<value to which device should be set>
        }
      --- Réponse ---
        Aucune, un message trig sensor.basic sera envoyé si changement d'état
   */
  if (gxPLMessagePairExist (msg, "device") &&
      gxPLMessagePairExist (msg, "type") &&
      gxPLMessagePairExist (msg, "current") ) {
    const char * output = gxPLMessagePairGet (msg, "device");
    const char * type = gxPLMessagePairGet (msg, "type");
    const char * current = gxPLMessagePairGet (msg, "current");


    if (strcmp (type, "output") == 0) {
      bool bModeRequested;
      char * dev = prvvGetOutputName (output, &bModeRequested);

      for (unsigned i = 0; i < iDoutPortSize (xCtx.xOutPort); i++) {
        struct xTorCtlOutput * out = &xCtx.xOutput[i];


        if (strcmp (dev, out->sName) == 0) {


          if ( (strcasecmp (current, "high") == 0) ||
               (strcasecmp (current, "on") == 0) ||
               (strcasecmp (current, "true") == 0) ||
               (strcasecmp (current, "enable") == 0) ||
               (strcmp (current, "1") == 0) )  {

            if (bModeRequested) {

              out->eCurrentMode = eTorCtlAuto;
            }
            else {

              if (iDoutSet (i, xCtx.xOutPort) != 0) {

                PWARNING ("Unable to set %s output", out->sName);
              }
              out->bOutputStatusRequested = 1;
            }
          }
          else if ( (strcasecmp (current, "low") == 0) ||
                    (strcasecmp (current, "off") == 0) ||
                    (strcasecmp (current, "false") == 0) ||
                    (strcasecmp (current, "disable") == 0) ||
                    (strcmp (current, "0") == 0) )  {

            if (bModeRequested) {

              out->eCurrentMode = eTorCtlManual;
            }
            else {

              if (iDoutClear (i, xCtx.xOutPort) != 0) {

                PWARNING ("Unable to clear %s output", out->sName);
              }
              out->bOutputStatusRequested = 1;
            }
          }
          break;
        }
      }
      free (dev);
    }
  }
}

// -----------------------------------------------------------------------------
// Message trig teleinfo
static void
prvvTeleinfoBasicListener (gxPLDevice * device, gxPLMessage * msg, void * udata) {
  /*  --- Message reçu ---
        XPL-TRIG
        teleinfo.basic
        {
        adco=<ADCO>
        type=<ptec, demain, adps, motdetat>
        current=<PTEC, DEMAIN, ADPS, MOTDETAT>
        }
   */
  if (gxPLMessagePairExist (msg, "adco") &&
      gxPLMessagePairExist (msg, "type") &&
      gxPLMessagePairExist (msg, "current") ) {
    unsigned long ulAdco;

    if (iStrToLong (gxPLMessagePairGet (msg, "adco"), (long *) &ulAdco, 0) == 0) {

      for (unsigned i = 0; i < iDoutPortSize (xCtx.xOutPort); i++) {
        struct xTorCtlOutput * out = &xCtx.xOutput[i];

        if (out->eCurrentMode == eTorCtlAuto) {
          // sortie en auto

          if ( (out->ulAdco == 0) || (out->ulAdco == ulAdco) ) {
            // sortie correspondant au compteur
            const char * type = gxPLMessagePairGet (msg, "type");

            if (strcmp (type, out->sType) == 0) {
              // sortie du type du message reçu
              const char * current = gxPLMessagePairGet (msg, "current");
              char * value;
              bool bIsTrue;

              value = out->sOnValue;
              bIsTrue = true;
              if (strlen (value) > 1) {
                if (value[0] == '!') {
                  // valeur complémentée
                  value++;
                  bIsTrue = false;
                }
              }

              if ( (strcmp (current, value) == 0) == bIsTrue) {
                // la valeur reçue correspond au ON

                if (iDoutSet (i, xCtx.xOutPort) != 0) {

                  PWARNING ("Unable to set %s output", out->sName);
                }
              }
              else {

                value = out->sOffValue;
                bIsTrue = true;

                if (strlen (value) > 1) {
                  if (value[0] == '!') {
                    // valeur complémentée
                    value++;
                    bIsTrue = false;
                  }
                }

                if ( (strcmp (current, value) == 0) == bIsTrue) {
                  // la valeur reçue correspond au OFF

                  if (iDoutClear (i, xCtx.xOutPort) != 0) {

                    PWARNING ("Unable to clear %s output", out->sName);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

/* internal public functions ================================================ */
// -----------------------------------------------------------------------------
int
iTorCtlOpen (gxPLDevice * device) {

  // Setting up hardware
  xCtx.xOutPort = xDoutOpen (xMyPins, DMG_TORCTL_PORTSIZE);

  if (xCtx.xOutPort == NULL) {

    return -1;
  }

  iDoutClearAll (xCtx.xOutPort);

  gxPLDeviceListenerAdd (device, prvvSensorRequestListener,
                         gxPLMessageCommand, "sensor", "request", NULL);
  gxPLDeviceListenerAdd (device, prvvControlBasicListener,
                         gxPLMessageCommand, "control", "basic", NULL);
  gxPLDeviceListenerAdd (device, prvvTeleinfoBasicListener,
                         gxPLMessageTrigger, "teleinfo", "basic", NULL);
  return 0;
}

// -----------------------------------------------------------------------------
int
iTorCtlClose (gxPLDevice * device) {

  iDoutClearAll (xCtx.xOutPort);
  return iDoutClose (xCtx.xOutPort);
}

// -----------------------------------------------------------------------------
int
iTorCtlPoll (gxPLDevice * device) {
  int value;

  for (unsigned i = 0; i < iDoutPortSize (xCtx.xOutPort); i++) {
    gxPLMessageType mtype = gxPLMessageUnknown;
    
    // test du changement d'état des sorties
    value = iDoutRead (i, xCtx.xOutPort);
    if (value != xCtx.xOutput[i].iValue) {

      xCtx.xOutput[i].iValue = value;
      mtype = gxPLMessageTrigger;
    }
    else if (xCtx.xOutput[i].bOutputStatusRequested) {
      
      mtype = gxPLMessageStatus;
    }
    
    if (mtype != gxPLMessageUnknown) {
      
      xCtx.xOutput[i].bOutputStatusRequested = 0;
      if (priSendCurrentValue (device, i, false, mtype) < 0) {

        return -1;
      }
    }

    // test du changement de mode des sorties
    if (xCtx.xOutput[i].eCurrentMode != xCtx.xOutput[i].ePreviousMode) {

      xCtx.xOutput[i].ePreviousMode = xCtx.xOutput[i].eCurrentMode;
      if (priSendCurrentValue (device, i, true, gxPLMessageTrigger) < 0) {

        return -1;
      }
    }
  }

  return 0;
}

// -----------------------------------------------------------------------------
int
iTorCtlSendStat (gxPLDevice * device) {
  
  for (unsigned i = 0; i < iDoutPortSize (xCtx.xOutPort); i++) {
    struct xTorCtlOutput * out = &xCtx.xOutput[i];

    // On doit envoyé l'état et le mode de toutes les sorties
    if (priSendCurrentValue (device, i, false, gxPLMessageStatus) < 0) {

      PERROR ("Unable to send current value for %s", out->sName);
      return -1;
    }
    if (priSendCurrentValue (device, i, true, gxPLMessageStatus) < 0) {

      PERROR ("Unable to send current mode for %s", out->sName);
      return -1;
    }
  }
  return 0;
}

/* ========================================================================== */
