/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"

#define AICS_AUDIO_INPUT_DESCRIPTION "LEPL MICP AICS"

wiced_result_t lepl_mics_set_mute(uint16_t conn_id, uint8_t mute)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    WICED_BT_TRACE("[%s] conn_id 0x%x mute state %d", __FUNCTION__, conn_id, mute);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No p_clcb", __FUNCTION__);
        return WICED_ERROR;
    }

    if (ga_lib_mics_write_mute_state(conn_id,
                                     &p_clcb->peer_profiles.mics[GA_LIB_MICS_CHARACTERISTIC_MUTE_STATE],
                                     mute) == WICED_BT_GATT_SUCCESS)
    {
        p_clcb->waiting_for_mics_notif = 1;
        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

wiced_result_t lepl_mics_aics_set_mute(uint16_t conn_id, uint32_t instance, uint8_t mute)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);

    WICED_BT_TRACE("[%s] conn_id 0x%x mute state %d", __FUNCTION__, conn_id, mute);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No p_clcb", __FUNCTION__);
        return WICED_ERROR;
    }

    if (p_clcb->mics_aics[instance].input_state.mute_mode == mute)
    {
        WICED_BT_TRACE_CRIT("[%s] No change in mute state %d", __FUNCTION__, mute);
        return WICED_ERROR;
    }

    gatt_intf_characteristic_handles_t *p_char =
        &p_clcb->peer_profiles.mics_aics[instance][GA_LIB_AICS_CHARACTERISTIC_INPUT_CONTROL_POINT];
    uint8_t opcode = (mute) ? GA_LIB_AICS_OPCODE_SET_MUTE : GA_LIB_AICS_OPCODE_SET_UNMUTE;
    ga_lib_aics_input_state_t aics_input_state = {.change_counter = p_clcb->mics_aics[instance].input_state.change_counter,
                                                  .mute_mode = mute};

    if (ga_lib_aics_write_cp(conn_id, p_char, opcode, &aics_input_state) == WICED_BT_GATT_SUCCESS)
    {
        p_clcb->waiting_for_aics_notif[instance] = 1;
        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

wiced_result_t lepl_mics_aics_set_gain(uint16_t conn_id, uint32_t instance, uint8_t opcode, int8_t input_gain)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No p_clcb", __FUNCTION__);
        return WICED_ERROR;
    }
    lepl_aics_data_t *p_aics = &p_clcb->mics_aics[instance];

    if (opcode == HCI_CONTROL_LEA_MICS_AICS_GAIN_INCREMENT)
    {
        int gain = p_aics->input_state.gain_setting + (p_aics->gain_setting.gain_setting_units);
        input_gain = (gain > 127) ? 127 : (int8_t)gain;
    }
    else if (opcode == HCI_CONTROL_LEA_MICS_AICS_GAIN_DECREMENT)
    {
        int gain = p_aics->input_state.gain_setting - (p_aics->gain_setting.gain_setting_units);
        input_gain = (gain < -128) ? -128 : (int8_t)gain;
    }

    ga_lib_aics_gain_settings_params_t *p_gain = &p_aics->gain_setting;

    if ((p_gain->max_gain_setting < input_gain) || (input_gain < p_gain->min_gain_setting) ||
        (p_aics->input_state.gain_setting == input_gain))
    {
        return WICED_ERROR;
    }

    WICED_BT_TRACE("[%s] gain : input %d units %d", __FUNCTION__, input_gain, p_gain->gain_setting_units);

    gatt_intf_characteristic_handles_t *p_char =
        &p_clcb->peer_profiles.mics_aics[instance][GA_LIB_AICS_CHARACTERISTIC_INPUT_CONTROL_POINT];
    ga_lib_aics_input_state_t aics_input_state = {.change_counter =
                                                      p_clcb->mics_aics[instance].input_state.change_counter,
                                                  .gain_setting = input_gain};

    if (ga_lib_aics_write_cp(conn_id, p_char, GA_LIB_AICS_OPCODE_SET_GAIN_SETTINGS, &aics_input_state) == WICED_BT_GATT_SUCCESS)
    {
        p_clcb->waiting_for_aics_notif[instance] = 1;
        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

void lepl_mics_handle_received_characteristics_value(uint16_t conn_id,
                                    lepl_clcb_t *p_clcb,
                                    wiced_bt_gatt_optype_t op,
                                    wiced_bt_gatt_status_t status,
                                    wiced_bt_gatt_data_t *p_gatt_data)
{
    lepl_mics_data_t *p_mics = &p_clcb->mics;
    uint8_t *p_data = p_gatt_data->p_data;
    int mics_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.mics,
                                                                        GA_LIB_MICS_CHARACTERISTIC_MAX,
                                                                        p_gatt_data->handle);

    if (mics_index == GA_LIB_MICS_CHARACTERISTIC_MUTE_STATE)
    {
        STREAM_TO_UINT8(p_mics->mute_state, p_data);
        WICED_BT_TRACE("[%s] char %d  mute_state:%x \n", __FUNCTION__, mics_index, p_mics->mute_state);
        le_audio_rpc_send_mics_mute_state(conn_id, p_mics->mute_state);

        if (op == GATTC_OPTYPE_NOTIFICATION)
        {
           lepl_cap_handle_mics_notification(p_clcb, &p_clcb->mics);
        }
    }
    return ;
}

void aics_process_op_cmpl_input_state(wiced_bt_gatt_data_t *p_op, lepl_aics_data_t *p_aics)
{
    uint8_t *p = p_op->p_data;
    ga_lib_aics_input_state_t *p_state = &p_aics->input_state;

    if (p_op->len == 4)
    {
        STREAM_TO_UINT8(p_state->gain_setting, p);   // byte 0 ==> Gain setting
        STREAM_TO_UINT8(p_state->mute_mode, p);      // byte 1 ==> Mute mode
        STREAM_TO_UINT8(p_state->gain_mode, p);      // byte 2 ==> Gain Mode
        STREAM_TO_UINT8(p_state->change_counter, p); // byte 3 ==> Change counter
    }

    return;
}

void aics_process_op_cmpl_gain_settings(wiced_bt_gatt_data_t *p_data, lepl_aics_data_t *p_aics)
{
    ga_lib_aics_gain_settings_params_t *p_gain = &p_aics->gain_setting;
    uint8_t *p = p_data->p_data;

    if (p_data->len == 3)
    {
        STREAM_TO_UINT8(p_gain->gain_setting_units, p); // byte 0 ==> Gain setting units
        STREAM_TO_UINT8(p_gain->min_gain_setting, p);   // byte 1 ==> Minimum gain setting
        STREAM_TO_UINT8(p_gain->max_gain_setting, p);   // byte 2 ==> Maximum gain setting
    }
    return;
}

void aics_process_op_cmpl_status(wiced_bt_gatt_data_t *p_data, lepl_aics_data_t *p_aics)
{
    uint8_t *p = p_data->p_data;

    // byte 0 ==> Input status
    if (p_data->len == 1)
    {
        STREAM_TO_UINT8(p_aics->input_status, p); // byte 0 ==> Input Status
    }
    return;
}

static void aics_process_op_cmpl_input_type(wiced_bt_gatt_data_t *p_data, lepl_aics_data_t *p_aics)
{
    uint8_t *p = p_data->p_data;

    if (p_data->len == 1)
    {
        STREAM_TO_UINT8(p_aics->input_type, p); // byte 0 ==> Input Type
    }

    return;
}

void aics_process_op_cmpl_description(wiced_bt_gatt_data_t *p_data, lepl_aics_data_t *p_aics)
{
    uint8_t *p = p_data->p_data;
    int len = p_data->len;

    if (len > MAX_DESCRIPTION)
    {
        len = MAX_DESCRIPTION;
    }

    WICED_MEMCPY(p_aics->description, p, len);
    return;
}

void aics_read_rsp_notification_handler(
    uint16_t conn_id, lepl_clcb_t *p_clcb, int aics_instance, wiced_bt_gatt_optype_t op, wiced_bt_gatt_data_t *p_data)
{
    int aics_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.mics_aics[aics_instance],
                                                                        GA_LIB_AICS_CHARACTERISTIC_MAX,
                                                                        p_data->handle);
    lepl_aics_data_t *p_aics = &p_clcb->mics_aics[aics_instance];

    //check for AICS handles
    switch (aics_index)
    {
    case GA_LIB_AICS_CHARACTERISTIC_INPUT_STATE: // 0
        aics_process_op_cmpl_input_state(p_data, p_aics);
        le_audio_rpc_send_mics_aics_input_state(p_clcb->conn_id, aics_instance, &p_aics->input_state);
        break;
    case GA_LIB_AICS_CHARACTERISTIC_GAIN_SETTING_PROPERTIES: //1
        aics_process_op_cmpl_gain_settings(p_data, p_aics);
        break;
    case GA_LIB_AICS_CHARACTERISTIC_INPUT_TYPE: //2
        aics_process_op_cmpl_input_type(p_data, p_aics);
        break;
    case GA_LIB_AICS_CHARACTERISTIC_INPUT_STATUS: //3
        aics_process_op_cmpl_status(p_data, p_aics);
        break;
    case GA_LIB_AICS_CHARACTERISTIC_INPUT_CONTROL_POINT: //4
        break;
    case GA_LIB_AICS_CHARACTERISTIC_INPUT_DESCRIPTION: //5
        aics_process_op_cmpl_description(p_data, p_aics);
        le_audio_rpc_send_mics_aics_description(p_clcb->conn_id, aics_instance, p_aics->description);
        break;
    default:
        WICED_BT_TRACE_CRIT("[%s] Invalid AICS char index %d for handle 0x%x",
                            __FUNCTION__,
                            aics_index,
                            p_data->handle);
        break;
    }

    if ((op == GATTC_OPTYPE_NOTIFICATION) && (aics_index == GA_LIB_AICS_CHARACTERISTIC_INPUT_STATE))
    {
        lepl_cap_handle_mics_aics_notification(p_clcb, aics_instance, &p_aics->input_state);
    }

    return;
}

void lepl_mics_aics_handle_received_characteristics_value(uint16_t conn_id,
                                                          lepl_clcb_t *p_clcb,
                                                          wiced_bt_gatt_optype_t op,
                                                          wiced_bt_gatt_status_t status,
                                                          wiced_bt_gatt_data_t *p_gatt_data)

{
    for (int i = 0; i < LEPL_MAX_MICS_AICS; i++)
    {
        gatt_intf_service_range_t *p_range = &p_clcb->peer_profiles.service_handles[LEPL_MICS_AICS + i];
        if ((p_gatt_data->handle >= p_range->start_handle) && (p_gatt_data->handle <= p_range->end_handle))
        {
            aics_read_rsp_notification_handler(conn_id, p_clcb, i, op, p_gatt_data);
        }
    }
}
