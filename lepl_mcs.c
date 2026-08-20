/*
 * $ Copyright Cypress Semiconductor $
 */
#include "lepl.h"

#define MEDIA_PLAYER_NAME "CYPRESS_PLAYER"
#define MEDIA_TRACK_TITLE "DEFAULT_TRACK"

ga_lib_mcs_result_t lepl_media_control_service_update_state(ga_lib_mcs_state_t state)
{
    lepl_mcs_t *p_media_app_data = &g_lepl_gatt_cb.local_service_data.gmcs;

    if (p_media_app_data->media_state == state)
    {
        return GA_LIB_MCS_SUCCESS;
    }

    p_media_app_data->media_state = state;
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (g_lepl_gatt_cb.clcb[i].in_use)
        {
            uint16_t conn_id = g_lepl_gatt_cb.clcb[i].conn_id;
            le_audio_rpc_send_mcs_state_update(conn_id, p_media_app_data->media_state);

            ga_lib_mcs_notify_media_state(conn_id, HDLC_GMCS_MEDIA_STATE_VALUE, p_media_app_data->media_state);
        }
    }

    return GA_LIB_MCS_SUCCESS;
}

wiced_bool_t lepl_mcs_is_streaming()
{
    ga_lib_mcs_state_t media_state = g_lepl_gatt_cb.local_service_data.gmcs.media_state;
    return (media_state == GA_LIB_MCS_MEDIA_PLAYING) ? WICED_TRUE : WICED_FALSE;
}

static ga_lib_mcs_result_t lepl_handle_streaming_operation(ga_lib_mcs_media_control_operation_t opcode,
                                                                uint16_t conn_id,
                                                                lepl_unicast_stream_config_t *p_stream_config)
{
    ga_lib_mcs_state_t media_state = g_lepl_gatt_cb.local_service_data.gmcs.media_state;
    WICED_BT_TRACE("[%s] media_app_data->media_state %d \n", __FUNCTION__, media_state);

    // Start/Stop actual streaming
    if (opcode == GA_LIB_MCS_PLAY)
    {
        // start streaming
        if (media_state != GA_LIB_MCS_MEDIA_PLAYING)
        {
            p_stream_config->context_type = BAP_CONTEXT_TYPE_MEDIA;
            if (lepl_cap_start_streaming(conn_id, p_stream_config))
                return GA_LIB_MCS_MEDIA_PLAYER_INACTIVE;
        }
        return lepl_media_control_service_update_state(GA_LIB_MCS_MEDIA_PLAYING);
    }
    else if ((opcode == GA_LIB_MCS_PAUSE) || (opcode == GA_LIB_MCS_STOP))
    {
        // stop streaming
        if (media_state != GA_LIB_MCS_MEDIA_PAUSED)
        {
            lepl_cap_stop_streaming(conn_id);
        }
        return lepl_media_control_service_update_state(GA_LIB_MCS_MEDIA_PAUSED);
    }
    return GA_LIB_MCS_SUCCESS;
}

void lepl_mcs_initialize_data(void)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);

    lepl_mcs_t  *p_mcs = &g_lepl_gatt_cb.local_service_data.gmcs;

    // Set default playing order single repeat
    p_mcs->playing_order = GA_LIB_MCS_SINGLE_REPEAT;
    // Playing order supported
    p_mcs->playing_order_supported =
        (MCS_SINGLE_ONCE_PLAYING_ORDER_MASK | MCS_SINGLE_REPEAT_PLAYING_ORDER_MASK |
         MCS_IN_ORDER_ONCE_PLAYING_ORDER_MASK | MCS_IN_ORDER_REPEAT_PLAYING_ORDER_MASK);

    p_mcs->media_state = GA_LIB_MCS_MEDIA_INACTIVE;
    memcpy(p_mcs->player_name, MEDIA_PLAYER_NAME, strlen(MEDIA_PLAYER_NAME));
    p_mcs->player_name[strlen(MEDIA_PLAYER_NAME)] = '\0';
    memcpy(p_mcs->track_title, MEDIA_TRACK_TITLE, strlen(MEDIA_TRACK_TITLE));
    p_mcs->track_title[strlen(MEDIA_TRACK_TITLE)] = '\0';
    p_mcs->track_position = 0;
    p_mcs->media_control_opcodes_supported = (MCS_BASIC_OPCODES_SUPPORTED | MCS_TRACK_OPCODES_SUPPORTED);

    // select the track and update media state
    p_mcs->track_selected = 0;

}

wiced_bt_gatt_status_t lepl_gmcs_handle_write_cp(uint16_t conn_id, uint8_t *p_data, uint16_t len_to_write)

{
    ga_lib_mcs_result_t result = GA_LIB_MCS_SUCCESS;
    ga_lib_mcs_media_control_operation_t opcode = GA_LIB_MCS_INVALID;
    lepl_mcs_t *p_mcs = &g_lepl_gatt_cb.local_service_data.gmcs;
    lepl_audio_mode_t audio_mode = lepl_cap_get_audio_mode();

    // Format
    // Byte 0 ==> Opcode
    if (len_to_write < 1)
    {
        result = GA_LIB_MCS_COMMAND_CANNOT_BE_COMPLETED;
    }
    else
    {
        STREAM_TO_UINT8(opcode, p_data);

        switch (opcode)
        {
        case GA_LIB_MCS_PLAY:
            if (p_mcs->media_state == GA_LIB_MCS_MEDIA_INACTIVE || audio_mode != LEPL_AUDIO_MODE_NONE)
            {
                WICED_BT_TRACE_CRIT("[%s] Not ready to play! state: %d", __FUNCTION__, audio_mode);
                return GA_LIB_MCS_MEDIA_PLAYER_INACTIVE;
            }
            else
            {
                lepl_handle_streaming_operation(opcode, conn_id, &g_lepl_gatt_cb.audio_state.paused_strm_config);
                lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_MEDIA, &g_lepl_gatt_cb.audio_state.paused_strm_config);
            }
            break;
        case GA_LIB_MCS_PAUSE:
        case GA_LIB_MCS_STOP:
            if (audio_mode == LEPL_AUDIO_MODE_IN_TRANSIT)
                audio_mode = lepl_cap_get_final_audio_mode();

            if (p_mcs->media_state == GA_LIB_MCS_MEDIA_INACTIVE || audio_mode != LEPL_AUDIO_MODE_MEDIA)
            {
                WICED_BT_TRACE_CRIT("[%s] state: %d", __FUNCTION__, audio_mode);
                return GA_LIB_MCS_MEDIA_PLAYER_INACTIVE;
            }
            lepl_handle_streaming_operation(opcode, conn_id, NULL);
            lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
            break;
        case GA_LIB_MCS_MOVE_RELATIVE:
        {
            uint32_t move_relative_offset = 0;
            if (len_to_write != 5)
            {
                result = GA_LIB_MCS_COMMAND_CANNOT_BE_COMPLETED;
            }
            else
            {
                STREAM_TO_UINT32(move_relative_offset, p_data);
            }
            WICED_BT_TRACE("[%s] move_relative_offset: %d", __FUNCTION__, move_relative_offset);
        }
        break;
        case GA_LIB_MCS_GOTO_TRACK:
        {
            uint32_t track_number = 0;
            if (len_to_write != 5)
            {
                result = GA_LIB_MCS_COMMAND_CANNOT_BE_COMPLETED;
            }
            else
            {
                STREAM_TO_UINT32(track_number, p_data);
            }
            WICED_BT_TRACE("[%s] track_number: %d", __FUNCTION__, track_number);
        }
        break;
        case GA_LIB_MCS_FAST_REWIND:
        case GA_LIB_MCS_FAST_FORWARD:
        case GA_LIB_MCS_PREVIOUS_TRACK:
        case GA_LIB_MCS_NEXT_TRACK:
        case GA_LIB_MCS_FIRST_TRACK:
        case GA_LIB_MCS_LAST_TRACK:
        case GA_LIB_MCS_PREVIOUS_SEGMENT:
        case GA_LIB_MCS_NEXT_SEGMENT:
        case GA_LIB_MCS_FIRST_SEGMENT:
        case GA_LIB_MCS_LAST_SEGMENT:
        case GA_LIB_MCS_GOTO_SEGMENT:
            WICED_BT_TRACE("[%s] opcode: %d not implemented \n", __FUNCTION__, opcode);
            break;
        default:
            result = GA_LIB_MCS_OPCODE_NOT_SUPPORTED;
            break;
        }
    }
    return ga_lib_mcs_notify_cp_result(conn_id, HDLC_GMCS_CONTROL_POINT_VALUE, opcode, result);
}

wiced_result_t lepl_mcs_play(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] conn_id 0x%x  \n", __FUNCTION__, conn_id);
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb || !p_clcb->in_use)
    {
        return WICED_ERROR;
    }

    ga_lib_mcs_result_t status = GA_LIB_MCS_SUCCESS;
    status = lepl_handle_streaming_operation(GA_LIB_MCS_PLAY, conn_id, p_stream_config);
    if (status != GA_LIB_MCS_SUCCESS)
    {
        WICED_BT_TRACE_CRIT("[%s] MCS play failed %d", __FUNCTION__, status);
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t lepl_mcs_pause(uint16_t conn_id)
{
    WICED_BT_TRACE("[%s] conn_id 0x%x \n", __FUNCTION__, conn_id);

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb || !p_clcb->in_use)
    {
        return WICED_ERROR;
    }

    ga_lib_mcs_result_t status = GA_LIB_MCS_SUCCESS;
    status = lepl_handle_streaming_operation(GA_LIB_MCS_PAUSE, conn_id, NULL);
    if (status != GA_LIB_MCS_SUCCESS)
    {
        WICED_BT_TRACE_CRIT("[%s] MCS pause failed %d", __FUNCTION__, status);
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_bt_gatt_status_t lepl_mcs_handle_read_request(lepl_clcb_t *p_clcb,
                                                  uint16_t handle,
                                                  uint8_t *p_data,
                                                  uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    lepl_local_service_data_t *p_local = &g_lepl_gatt_cb.local_service_data;
    uint8_t *p_data_start = p_data;

    switch (handle)
    {
    case HDLC_GMCS_PLAYER_NAME_VALUE:
    {
        int len = strlen((char *)p_local->gmcs.player_name);
        memcpy(p_data, p_local->gmcs.player_name, len);
        p_data += len;
    }
    break;
    case HDLC_GMCS_TRACK_TITLE_VALUE:
    {
        int len = strlen((char *)p_local->gmcs.track_title);
        memcpy(p_data, p_local->gmcs.track_title, len);
        p_data += len;
    }
    break;
    case HDLC_GMCS_TRACK_DURATION_VALUE:
        UINT32_TO_STREAM(p_data, p_local->gmcs.track_duration);
        break;
    case HDLC_GMCS_TRACK_POSITION_VALUE:
        UINT32_TO_STREAM(p_data, p_local->gmcs.track_position);
        break;
    case HDLC_GMCS_PLAYBACK_SPEED_VALUE:
        UINT16_TO_STREAM(p_data, p_local->gmcs.playback_speed);
        break;
    case HDLC_GMCS_SEEKING_SPEED_VALUE:
        UINT16_TO_STREAM(p_data, p_local->gmcs.seeking_speed);
        break;
    case HDLC_GMCS_PLAYING_ORDER_VALUE:
        UINT8_TO_STREAM(p_data, p_local->gmcs.playing_order);
        break;
    case HDLC_GMCS_PLAYING_ORDER_SUPPORTED_VALUE:
        UINT16_TO_STREAM(p_data, p_local->gmcs.playing_order_supported);
        break;
    case HDLC_GMCS_MEDIA_STATE_VALUE:
        UINT8_TO_STREAM(p_data, p_local->gmcs.media_state);
        break;
    case HDLC_GMCS_CONTROL_POINT_OPCODE_SUPPORTED_VALUE:
        UINT32_TO_STREAM(p_data, p_local->gmcs.media_control_opcodes_supported);
        break;
    case HDLC_GMCS_CONTENT_CONTROL_ID_VALUE:
        UINT8_TO_STREAM(p_data, p_local->gmcs.content_control_id);
        break;
    default:
        status = WICED_BT_GATT_READ_NOT_PERMIT;
        break;
    }
    *p_len = p_data - p_data_start;
    WICED_BT_TRACE("[%s] hdl %d len %d status 0x%x", __FUNCTION__, handle, *p_len, status);

    return status;
}
