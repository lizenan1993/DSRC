/**
 * @addtogroup mk2mac_test MK2 MLME_IF test application(s)
 * @{
 *
 * @file
 * test-tx: Channel configuration & Transmit testing
 */

//------------------------------------------------------------------------------
// Copyright (c) 2010 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifndef __QNX__
#include <getopt.h>
#include <linux/if_ether.h>
#endif

#include "test-common.h"
#include "mkxtest.h"
#include "RxOpts.h"

// identifier this module for debugging
#define D_SUBMODULE RXAPP_Module
#include "debug-levels.h"

void RxOpts_printusage()
{
  int i;
  static tUsageOption UsageOptionList[] =
  {
    {'f', "LogFile",       "Packet Log File Name",  "string"},
    {'r', "ReportFile",    "Report File Name",      "string"},
    {'h', "Help|Usage",    "Print this Usage",      ""},
    {'u', "LogUnMatched",  "Log Unmatched Packets", ""},
    {'p', "ReportPeriod",  "Report to Standard output after this many packets","uint16"},
    {'y', "DumpPayload",   "Dump Packet Payload in addition to Descriptor",    ""},
    {'o', "DumpToStdout",  "Dump packets to Standard output",                  ""},
    {'s', "StatSA",        "Statistics on this MAC Source Address only",       "aa:bb:cc:dd:ee:ff"},
    {'i', "Interface",     "Network interface to receive on",  "wave-raw | wave-mgmt"},
    {'c', "ChannelNumber", "Rx on this Channel Number",        "uint8"},
    {'l', "LatencyResults","Give latency calculation results",        "uint8"},
    {0, 0, 0, 0}
  };

  printf("usage: test-rx <Options>\n");
  i = 0;
  while (UsageOptionList[i].pName != NULL)
  {
    printf("-%c, ", UsageOptionList[i].Letter);
    printf("--%-s\n", UsageOptionList[i].pName);
    printf("        %s\n", UsageOptionList[i].pDescription);
    // Display any type info
    if (strlen(UsageOptionList[i].pValues) > 0)
      printf("        %s\n", UsageOptionList[i].pValues);
    i++;
  }

}

/**
 * @brief Print a Rx Options Object. Makes use of inherited type printers.
 * @param Rx Options Object
 */
void RxOpts_Print (tRxOpts * pRxOpts)
{

  // Packets logged here
  printf("Packet Log File:   %s\n", pRxOpts->pPacketLogFileName);
  // Report saved here
  printf("Report File:       %s\n", pRxOpts->pReportFileName);
  // Log Unmatched packets in addition
  printf("LogUnMatched:      %d\n", pRxOpts->LogUnMatched);
  // Dump entire packet payload to file
  printf("DumpPayload:       %d\n", pRxOpts->DumpPayload);
  // Dump packet to stadout
  printf("DumpToStdout:      %d\n", pRxOpts->DumpToStdout);
  // Report Period (Packets)
  printf("ReportPeriod:      %d\n", pRxOpts->ReportPeriod);
  // Is Source Address Filtering On?
  printf("Filter SA: ");
  if (pRxOpts->StatSAFilt)
  {
    MACAddr_fprintf(stdout, pRxOpts->StatSA);
    printf("\n");
  }
  else
  {
    printf("None\n");
  }
  // Network Interface
  printf("Interface:         %s\n", pRxOpts->pInterfaceName);
  // Channel Numbers
  printf("ChannelNumber:     %d\n", pRxOpts->ChannelNumber);
}

/**
 * @brief Process the Rx command line arguments
 * @param argc the number of command line arguments
 * @param argv pointer to the command line options
 * @param pRxOpts pointer to the tRxOpts object to be filled
 */
int RxOpts_New (int argc, char **argv, tRxOpts *pRxOpts)
{
  char short_options[100];

  tRxOptsErrCode ErrCode = RXOPTS_ERR_NONE;
  int c = 0; // returned from getopt (argument index)

#ifndef __QNX__
  int option_index = 0;

  // Long Option specification
  // {Long Name, has arg?, Flag Pointer, Val to set if found}
  static struct option long_options[] =
  {
    { "Help", no_argument, 0, 'h' }, // Usage
    { "help", no_argument, 0, 'h' }, // Usage
    { "Usage", no_argument, 0, 'h' }, // Usage
    { "LogFile", required_argument, 0, 'f' }, // Log file name
    { "LogUnMatched", no_argument, 0, 'u' },
    { "DumpPayload", no_argument, 0, 'y' },
    { "DumpToStdout", no_argument, 0, 'o' },
    { "ReportPeriod", required_argument, 0, 'p' }, // Unblocks between reports
    { "ReportFile", required_argument, 0, 'r' }, // Report file name
    { "StatSA", required_argument, 0, 's' }, // Statistics on these pkts
    { "StatEtherType", required_argument, 0, 'e' },
    { "Interface", required_argument, 0, 'i' }, // wave-raw or wave-mgmt
    { "ChannelNumber", required_argument, 0, 'c' },
    { "LatencyResults", no_argument, 0, 'l' },
    { 0, 0, 0, 0 }
  };
#endif

  //---------------------------------------------
  // Default settings
  // Packet Log FileName
  //strcpy(pRxOpts->pPacketLogFileName, "RxLog.txt");
  pRxOpts->pPacketLogFileName[0] = 0;
  // Report FileName
#if defined(__QNX__)
  // TODO: for now always log to /tmp since in general / is not writable on MQ5
  strcpy(pRxOpts->pReportFileName, "/tmp/RxReport.txt");
#else
  strcpy(pRxOpts->pReportFileName, "RxReport.txt");
#endif
  // Log Unmatched packets in addition
  pRxOpts->LogUnMatched = false; // true if option present
  // Dump entire packet payload to file
  pRxOpts->DumpPayload = false; // true if option present
  // Dump entire packet payload to file
  pRxOpts->DumpToStdout = false; // true if option present
  // Unblocks between reports
  pRxOpts->ReportPeriod = 100;
  // Statistics Filtering is OFF. Only turned on if any of the Stat args are set.
  pRxOpts->StatSAFilt = false;
  strcpy(pRxOpts->pInterfaceName, "wave-raw");
  pRxOpts->ChannelNumber = 178;//TEST_DEFAULT_CHANNELNUMBER;
  pRxOpts->LatencyResults = 0;

#if defined(__QNX__)
  strcpy(short_options, "hf:uyop:r:s:e:i:c:l");
#else
  // Build the short options from the Long
  copyopts(long_options, (char *) (&short_options));
#endif

  while (1)
  {
    ErrCode = RXOPTS_ERR_NONE; // set the error for this arg to none
#if defined(__QNX__)
    c = getopt(argc, argv, short_options);
#else
    c
      = getopt_long_only(argc, argv, short_options, long_options,
                         &option_index);
#endif
    //printf("c:%c, option_index: %d, option arg: %s\n", c, option_index, optarg);

    // finshed
    if (c == -1)
      return 0;

    // unrecognised option
    //if (c == '?')
    //  printf("Unrecognised option.\n");
    //  return -1;

    // unrecognised option
    //if (c == '.')
    //  fprintf(stdout, "Option Argument missing.\n");
    //  return -1;

    switch (c)
    {

      case 'h': // usage
        return -1;
        break;

      case 'f': // Packet Log File Name
        strcpy(pRxOpts->pPacketLogFileName, optarg);
        break;

      case 'u': // Log Unmatched packets in addition
        pRxOpts->LogUnMatched = true;
        break;

      case 'y': // Dump entire packet payload to file
        pRxOpts->DumpPayload = true;
        break;

      case 'o': // Dump to stdout
        pRxOpts->DumpToStdout = true;
        break;

      case 'p': // Unblocks between reports
        pRxOpts->ReportPeriod = (uint16_t) strtoul(optarg, NULL, 0);
        if (pRxOpts->ReportPeriod == 0)
          pRxOpts->ReportPeriod = 1;

        break;
      case 'r': // Report Log File Name
        strcpy(pRxOpts->pReportFileName, optarg);
        break;

      case 's': // Filtering on Ethernet Header Source address
        if (!(GetMacFromString(pRxOpts->StatSA, optarg)))
        {
          ErrCode = RXOPTS_ERR_INVALIDOPTIONARG;
          goto error;
        }
        pRxOpts->StatSAFilt = true;
        break;

      case 'e': // Filtering on Ethernet Type
        fprintf(stderr,
                "Warning: EtherType filtering no longer supported. Ignoring.\n");
        break;
      case 'i': // Interface (raw of mgmt, could add data)
        if (strcmp("wave-raw", optarg) == 0)
          strcpy(pRxOpts->pInterfaceName, optarg);
        else if (strcmp("wave-mgmt", optarg) == 0)
          strcpy(pRxOpts->pInterfaceName, optarg);
        else
        {
          ErrCode = RXOPTS_ERR_INVALIDOPTIONARG;
          goto error;
        }
        break;
      case 'c': // Channel Number to listen on
        pRxOpts->ChannelNumber = strtoul(optarg, NULL, 0);
        break;

      case 'l': // Give latency results
        pRxOpts->LatencyResults = 1;
        break;

      default:
        ErrCode = RXOPTS_ERR_INVALIDOPTION;
        goto error;
        break;
    } // switch on argument

    if (ErrCode != RXOPTS_ERR_NONE)
    {
      goto error;
    }

  } // while getopt is not finished


error:
  if (ErrCode == RXOPTS_ERR_INVALIDOPTIONARG)
    fprintf(stderr, "invalid argument %s of option %s\n", optarg, argv[optind - 2]);
  if (ErrCode == RXOPTS_ERR_INVALIDOPTION)
    fprintf(stderr, "invalid option %s\n", argv[optind - 2]);

  return -1;
}

/**
 * @}
 */
