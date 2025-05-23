/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

/* Analog Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/config.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/services.h"

#ifndef MAX_ANALOG_INPUTS
#define MAX_ANALOG_INPUTS 2
#endif

static float Present_Value[MAX_ANALOG_INPUTS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Analog_Input_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_UNITS, -1 };

static const int Analog_Input_Properties_Optional[] = { -1 };

static const int Analog_Input_Properties_Proprietary[] = { -1 };

void Analog_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired)
        *pRequired = Analog_Input_Properties_Required;
    if (pOptional)
        *pOptional = Analog_Input_Properties_Optional;
    if (pProprietary)
        *pProprietary = Analog_Input_Properties_Proprietary;

    return;
}

void Analog_Input_Init(void)
{
    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Input_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_ANALOG_INPUTS)
        return true;

    return false;
}

/* we simply have 0-n object instances. */
unsigned Analog_Input_Count(void)
{
    return MAX_ANALOG_INPUTS;
}

/* we simply have 0-n object instances. */
uint32_t Analog_Input_Index_To_Instance(unsigned index)
{
    return index;
}

bool Analog_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text[32]; /* okay for single thread */
    bool status = false;

    if (object_instance < MAX_ANALOG_INPUTS) {
        snprintf(text, sizeof(text), "AI-%lu", (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

float Analog_Input_Present_Value(uint32_t object_instance)
{
    float value = 0.0;

    if (object_instance < MAX_ANALOG_INPUTS) {
        value = Present_Value[object_instance];
    }

    return value;
}

void Analog_Input_Present_Value_Set(uint32_t object_instance, float value)
{
    if (object_instance < MAX_ANALOG_INPUTS) {
        Present_Value[object_instance] = value;
    }
}

/* return apdu length, or -1 on error */
/* assumption - object already exists */
int Analog_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_BIT_STRING bit_string = { 0 };
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Analog_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_real(
                &apdu[0], Analog_Input_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], UNITS_PERCENT);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}
