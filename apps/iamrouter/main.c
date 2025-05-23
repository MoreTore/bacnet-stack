/**
 * @file
 * @brief command line tool that sends a BACnet I-Am-Router-To-Network message
 * for one or more networks.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

/* global variables used in this file */
#define MAX_ROUTER_DNETS 64
static int Target_Router_Networks[MAX_ROUTER_DNETS] = { -1 };

static bool Error_Detected = false;

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    (void)server;
    printf("BACnet Abort: %s\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf("BACnet Reject: %s\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s DNET [DNET] [DNET] [...]\n", filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf(
        "Send BACnet I-Am-Router-To-Network message for \n"
        "one or more networks.\n"
        "\nDNET:\n"
        "BACnet destination network number 0-65534\n"
        "To send a I-Am-Router-To-Network message for DNET 86:\n"
        "%s 86\n"
        "To send a I-Am-Router-To-Network message for multiple DNETs\n"
        "use the following command:\n"
        "%s 86 42 24 14\n",
        filename, filename);
}

int main(int argc, char *argv[])
{
    unsigned arg_count = 0;
    int argi = 0;
    const char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            exit(0);
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2014 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            exit(0);
        }
    }
    if (argc < 2) {
        print_usage(filename);
        return 0;
    }
    /* decode the command line parameters */
    if (argc > 1) {
        for (arg_count = 1; arg_count < argc; arg_count++) {
            if (arg_count > MAX_ROUTER_DNETS) {
                fprintf(
                    stderr, "Limited to %u DNETS.  Sorry!\n", MAX_ROUTER_DNETS);
                break;
            }
            Target_Router_Networks[arg_count - 1] =
                strtol(argv[arg_count], NULL, 0);
            /* mark the end of list */
            Target_Router_Networks[arg_count] = -1;
            /* invalid DNET? */
            if (Target_Router_Networks[arg_count - 1] >= 65535) {
                fprintf(
                    stderr, "DNET=%u - it must be less than %u\n",
                    Target_Router_Networks[arg_count - 1], 65535);
                return 1;
            }
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    dlenv_init();
    atexit(datalink_cleanup);
    /* send the request */
    Send_I_Am_Router_To_Network(Target_Router_Networks);

    return 0;
}
