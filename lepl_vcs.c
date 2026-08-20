/*
 * $ Copyright Cypress Semiconductor $
 */
#include "lepl.h"

static wiced_result_t lepl_check_volume_state_validity(lepl_clcb_t *p_clcb, ga_lib_vcs_volume_control_opcodes_t opcode, uint8_t abs_vol)
{
    ga_lib_vcs_volume_state_t *p_vol = &p_clcb->vcs.volume_state;
    wiced_result_t res = WICED_SUCCESS;
    switch (opcode)
    {
    case VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_DOWN:
        if (p_vol->volume_setting == GA_LIB_VCS_MINIMUM_VOLUME)
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_UP:
        if (p_vol->volume_setting == GA_LIB_VCS_MAXIMUM_VOLUME)
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN:
        if ((p_vol->volume_setting == GA_LIB_VCS_MINIMUM_VOLUME) && (p_vol->mute_state == GA_LIB_MUTE_STATE_NOT_MUTED))
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_UP:
        if ((p_vol->volume_setting == GA_LIB_VCS_MAXIMUM_VOLUME) && (p_vol->mute_state == GA_LIB_MUTE_STATE_NOT_MUTED))
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_SET_ABSOLUTE_VOLUME:
        if (p_vol->volume_setting == abs_vol)
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_MUTE:
        if (p_vol->mute_state == GA_LIB_MUTE_STATE_MUTED)
        {
            res = WICED_ERROR;
        }
        break;
    case VOLUME_CONTROL_OPCODE_UNMUTE:
        if (p_vol->mute_state == GA_LIB_MUTE_STATE_NOT_MUTED)
        {
            res = WICED_ERROR;
        }
        break;
    default:
        WICED_BT_TRACE("[%s] unsupported opcode %d", __FUNCTION__, opcode);
        res = WICED_ERROR;
        break;
    }
    return res;
}

wiced_result_t lepl_vcs_set_volume(uint16_t conn_id, ga_lib_vcs_volume_control_opcodes_t opcode, uint8_t abs_vol)
{
    WICED_BT_TRACE("[%s] opcode %d vol %d\n", __FUNCTION__, opcode, abs_vol);
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);

    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No p_clcb conn id %d", __FUNCTION__, conn_id);
        return WICED_ERROR;
    }

    if (lepl_check_volume_state_validity(p_clcb, opcode, abs_vol) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    ga_lib_vcs_volume_state_t vcs_state = {.change_counter = p_clcb->vcs.volume_state.change_counter,
                                           .volume_setting = abs_vol};

    if (ga_lib_vcs_write_control_point(conn_id,
                                       &p_clcb->peer_profiles.vcs[GA_LIB_VCS_CHARACTERISTIC_CONTROL_POINT],
                                       opcode,
                                       &vcs_state) == WICED_SUCCESS)
    {
        p_clcb->waiting_for_vcs_notif = 1;
        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

wiced_result_t lepl_vcs_set_mute_state(uint16_t conn_id, ga_lib_mute_val_t mute_state)
{
    WICED_BT_TRACE("[%s] mute state %d \n", __FUNCTION__, mute_state);
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No p_clcb conn id %d", __FUNCTION__, conn_id);
        return WICED_ERROR;
    }
    ga_lib_vcs_volume_control_opcodes_t opcode =
        (mute_state) ? VOLUME_CONTROL_OPCODE_MUTE : VOLUME_CONTROL_OPCODE_UNMUTE;
    if (lepl_check_volume_state_validity(p_clcb, opcode, 0) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    ga_lib_vcs_volume_state_t vcs_state = {.change_counter = p_clcb->vcs.volume_state.change_counter,
                                           .mute_state = mute_state};
    if (ga_lib_vcs_write_control_point(conn_id,
                                       &p_clcb->peer_profiles.vcs[GA_LIB_VCS_CHARACTERISTIC_CONTROL_POINT],
                                       opcode,
                                       &vcs_state) == WICED_SUCCESS)
    {
        p_clcb->waiting_for_vcs_notif = 1;

        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

void lepl_vcs_handle_received_characteristics_value(uint16_t conn_id,
                                                    lepl_clcb_t *p_clcb,
                                                    wiced_bt_gatt_optype_t op,
                                                    wiced_bt_gatt_status_t status,
                                                    wiced_bt_gatt_data_t *p_gatt_data)
{
    int vcs_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.vcs,
                                                                       GA_LIB_VCS_CHARACTERISTIC_MAX,
                                                                       p_gatt_data->handle);
    lepl_vcs_data_t *p_vcs = &p_clcb->vcs;
    uint8_t *p_data = p_gatt_data->p_data;

    switch (vcs_index)
    {
    case GA_LIB_VCS_CHARACTERISTIC_VOLUME_STATE:
    {
        ga_lib_vcs_volume_state_t *p_vol = &p_vcs->volume_state;
        STREAM_TO_UINT8(p_vol->volume_setting, p_data);
        STREAM_TO_UINT8(p_vol->mute_state, p_data);
        STREAM_TO_UINT8(p_vol->change_counter, p_data);

        WICED_BT_TRACE("[%s] vol %d mute %d change %d",
                       __FUNCTION__,
                       p_vol->volume_setting,
                       p_vol->mute_state,
                       p_vol->change_counter);
        le_audio_rpc_send_vcs_state_update(conn_id,
                                           p_vol->volume_setting,
                                           p_vol->mute_state,
                                           HCI_CONTROL_LEA_MUTE_AND_VOLUME_STATUS);
        if (op == GATTC_OPTYPE_NOTIFICATION)
        {
            lepl_cap_handle_vcs_notification(p_clcb, &p_clcb->vcs.volume_state);
        }
    }
    break;
    case GA_LIB_VCS_CHARACTERISTIC_VOLUME_FLAG:
    {
        STREAM_TO_UINT8(p_vcs->volume_flag, p_data);
        WICED_BT_TRACE("[%s] volume_flag:%d", __FUNCTION__, p_vcs->volume_flag);
    }
    break;
    default:
        break;
    }
}
