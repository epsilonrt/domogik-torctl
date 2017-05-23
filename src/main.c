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
 * @brief Main task
 */
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "dmg-torctl.h"
#include "config.h"

/* public variables ========================================================= */
xTorCtlContext xCtx;

/* private variables ======================================================== */
static bool bMainIsRun = true;

/* private functions ======================================================== */
static void
prvPrintUsage (void) {
  printf ("%s - xPL On/Off Devices controller\n", __progname);
  printf ("Copyright (c) 2016-2017 epsilonRT\n\n");
  printf ("Usage: %s [-i interface] [-n iolayer] [-W timeout] [-D] [-d] [-h]\n", __progname);
  printf ("  -i interface - use interface named interface (i.e. eth0) as network interface\n");
  printf ("  -n iolayer   - use hardware abstraction layer to access the network\n"
          "                 (i.e. udp, xbeezb... default: udp)\n");
  printf ("  -W timeout   - set the timeout at the opening of the io layer\n");
  printf ("  -D           - do not daemonize -- run from the console\n");
  printf ("  -d           - enable debugging, it can be doubled or tripled to\n"
          "                 increase the level of debug.\n");
  printf ("  -h           - print this message\n\n");
}

// -----------------------------------------------------------------------------
static void
prvSignalHandler (int sig) {

  if ( (sig == SIGTERM) || (sig == SIGINT) ) {

    bMainIsRun = false;
  }
}

/* internal public functions ================================================ */
// -----------------------------------------------------------------------------
void
vMain (gxPLSetting * setting) {
  int ret;
  size_t s;
  gxPLDevice * device;
  gxPLApplication * app;

  PNOTICE ("starting dmg-torctl... (%s log)", sLogPriorityStr (setting->log) );

  s = sizeof (struct xTorCtlOutput);
  xCtx.xOutput = calloc (DMG_TORCTL_PORTSIZE, s);
  assert (xCtx.xOutput);

  for (unsigned i = 0; i < DMG_TORCTL_PORTSIZE; i++) {
    
    xCtx.xOutput[i].iValue = -1;
    xCtx.xOutput[i].ePreviousMode = -1;
  }

  // Create xPL application and device
  device = xDeviceCreate (setting);
  if (device == NULL) {

    PERROR ("Unable to start xPL");
    free (xCtx.xOutput);
    exit (EXIT_FAILURE);
  }

  // take the application to be able to close
  app = gxPLDeviceParent (device);

  // TorCtl init.
  ret = iTorCtlOpen (device);
  if (ret != 0) {

    PERROR ("Unable to setting teleinfo switch, error %d", ret);
    gxPLAppClose (app);
    exit (EXIT_FAILURE);
  }

  // Enable the device to do the job
  gxPLDeviceEnable (device, true);

  // Install signal traps for proper shutdown
  signal (SIGTERM, prvSignalHandler);
  signal (SIGINT, prvSignalHandler);

  while (bMainIsRun) {

    // Main Loop
    ret = gxPLAppPoll (app, GXPL_POLL_RATE_MS);
    if (ret != 0) {

      PWARNING ("Unable to poll xPL network, error %d", ret);
    }

    if (gxPLDeviceIsHubConfirmed (device) ) {

      // if the hub is confirmed, performs xPL tasks...
      ret = iTorCtlPoll (device);
      if (ret != 0) {

        PWARNING ("Unable to poll teleinfo switch, error %d", ret);
      }
    }
  }

  ret = iTorCtlClose (device);
  if (ret != 0) {

    PWARNING ("Unable to close teleinfo switch, error %d", ret);
  }

  gxPLMessageDelete (xCtx.xMsg);

  // Sends heartbeat end messages to all devices
  ret = gxPLAppClose (app);
  if (ret != 0) {

    PWARNING ("Unable to close xPL network, error %d", ret);
  }
  free (xCtx.xOutput);

  PNOTICE ("dmg-torctl closed, Have a nice day !");
  exit (EXIT_SUCCESS);
}

// -----------------------------------------------------------------------------
void
vParseAdditionnalOptions (int argc, char *argv[]) {
  int c;

  static const char short_options[] = "h" GXPL_GETOPT;
  static struct option long_options[] = {
    {"help",     no_argument,        NULL, 'h' },
    {NULL, 0, NULL, 0} /* End of array need by getopt_long do not delete it*/
  };

  do  {

    c = getopt_long (argc, argv, short_options, long_options, NULL);

    switch (c) {

      case 'h':
        prvPrintUsage();
        exit (EXIT_SUCCESS);
        break;

      default:
        break;
    }
  }
  while (c != -1);
}

/* ========================================================================== */
