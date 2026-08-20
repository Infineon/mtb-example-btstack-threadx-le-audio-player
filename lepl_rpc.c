/*
 * $ Copyright Cypress Semiconductor $
 */

/* Application includes */
#include "lepl.h"
#include "lepl_bis.h"
#ifdef HS_SPK_ENABLED
#include "bt_hs_spk_control.h"
#endif

lepl_audio_state_info_t *p_audio_state_info = &g_lepl_gatt_cb.audio_state;

#ifdef SIMULATED_NVRAM
void lepl_rpc_send_identity_resolving_key(wiced_bt_local_identity_keys_t *p_id_keys)
{
    uint8_t tx_buff[75] = {0};
    uint8_t *p_buff = tx_buff;
    UINT16_TO_STREAM(p_buff, UNICAST_APP_NVRAM_ID_LOCAL_IRK);
    WICED_MEMCPY(p_buff, p_id_keys, sizeof(wiced_bt_local_identity_keys_t));
    p_buff += sizeof(wiced_bt_local_identity_keys_t);
    app_rpc_send_data(HCI_CONTROL_EVENT_IDENTITY_KEYS, tx_buff, (int)(p_buff - tx_buff));
}
void lepl_rpc_send_link_keys(uint16_t nvram_id, lepl_nvram_paired_device_key_t *p_key_data)
{
    uint8_t tx_buff[600] = {0};
    uint8_t *p_buff = tx_buff;
    UINT16_TO_STREAM(p_buff, nvram_id);
    WICED_MEMCPY(p_buff, p_key_data, sizeof(lepl_nvram_paired_device_key_t));
    p_buff += sizeof(lepl_nvram_paired_device_key_t);
    app_rpc_send_data(HCI_CONTROL_EVENT_NVRAM_DATA, tx_buff, (int)(p_buff - tx_buff));
}
#endif

void lepl_rpc_send_pacs_records(uint16_t conn_id,
    uint8_t *p_data,
    uint16_t val_len,
    uint8_t is_sink)
{
    uint8_t tx_buff[512] = {0};
    uint8_t *p_buff = tx_buff;
    UINT16_TO_STREAM(p_buff, conn_id);
    UINT8_TO_STREAM(p_buff, is_sink);
    ARRAY_TO_STREAM(p_buff, p_data, val_len);

    app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_PACS_RECORD, tx_buff, (int)(p_buff - tx_buff));
}

void lepl_rpc_send_audio_started_event()
{
    uint8_t tx_buff[5] = {0};
    uint8_t *p_buff = tx_buff;

    lepl_audio_mode_t audio_mode = lepl_cap_get_final_audio_mode();
    UINT32_TO_STREAM(p_buff, audio_mode);
    app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_STARTED, tx_buff, (int)(p_buff - tx_buff));
}

void lepl_rpc_send_csis_lock_state(uint16_t conn_id, uint8_t lock_state)
{
    uint8_t tx_buff[3] = {0};
    uint8_t *p_buff = (uint8_t *)tx_buff;

    UINT16_TO_STREAM(p_buff, conn_id);
    UINT8_TO_STREAM(p_buff, lock_state);
    app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_CSIS_LOCK_STATE, tx_buff, (int)(p_buff - tx_buff));
}

static void lepl_rpc_handle_scan(uint8_t *p_data, uint32_t data_len)
{
    uint8_t scan;
    uint8_t enable_uuid_filter;

    STREAM_TO_UINT8(scan, p_data);
    STREAM_TO_UINT8(enable_uuid_filter, p_data);

    WICED_BT_TRACE("[%s] adv %d len %d\n", __FUNCTION__, scan, data_len);

    lepl_gatt_start_stop_scan(scan, enable_uuid_filter);
}

static void lepl_rpc_handle_adv(uint8_t *p_data, uint8_t payload_len)
{
    uint8_t adv;

    STREAM_TO_UINT8(adv, p_data);

    WICED_BT_TRACE("[%s] adv %d len %d\n", __FUNCTION__, adv, payload_len);

    if (adv && (lepl_get_current_ble_activity() != LEPL_BLE_ACTIVITY_NONE))
    {
        return;
    }

    lepl_gatt_start_stop_adv(adv);
}

static void lepl_rpc_handle_connect(uint8_t *p_data, uint32_t data_len, int cancel)
{
    wiced_bt_device_address_t bd_addr;
    uint8_t addr_type;
    wiced_result_t status;

    STREAM_TO_UINT8(addr_type, p_data);
    STREAM_TO_BDADDR(bd_addr, p_data);

    if (cancel == 0)
    {
        lepl_ble_activity_state_t ble_activity = lepl_get_current_ble_activity();
        if (ble_activity == LEPL_BLE_ACTIVITY_SCANNING)
        {
            lepl_start_stop_scan(0, NULL);
        }
        else if (ble_activity == LEPL_BLE_ACTIVITY_CONNECTING)
        {
            WICED_BT_TRACE_CRIT("[%s] Already connecting to a device", __FUNCTION__);
            return;
        }
        status = app_create_connection(addr_type, bd_addr);
    }
    else
    {
        status = wiced_bt_gatt_cancel_connect(bd_addr, 1);
    }

    WICED_BT_TRACE("[%s] %sconnect type %d address %B len %d status %d\n", __FUNCTION__,
                   cancel ? "cancel" : "", addr_type, bd_addr, data_len, status);
}

wiced_result_t lepl_rpc_stop_streaming(uint16_t conn_id, lepl_audio_mode_t mode, lepl_audio_mode_t next_mode)
{
    mode = (mode == LEPL_AUDIO_MODE_IN_TRANSIT) ? lepl_cap_get_final_audio_mode() : mode;
    if ((next_mode != LEPL_AUDIO_MODE_NONE) && (mode > next_mode))
    {
        WICED_BT_TRACE_CRIT("[%s] Invalid state transition! current state: %d next state: %d",
                            __FUNCTION__,
                            mode,
                            next_mode);
        return WICED_ABORTED;
    }
    switch (mode)
    {
    case LEPL_AUDIO_MODE_MEDIA:
        lepl_mcs_pause(conn_id);
        break;
    case LEPL_AUDIO_MODE_MIC:
    case LEPL_AUDIO_MODE_CALL:
        lepl_cap_stop_streaming(conn_id);
        break;
    case LEPL_AUDIO_MODE_BIDIRMIC:
        lepl_cap_stop_streaming(conn_id);
        lepl_rpc_send_convo_stream_state(0);
        break;
    default:
        WICED_BT_TRACE_CRIT("[%s] Can't proceed mode:%d", __FUNCTION__, mode);
        return WICED_ABORTED;
    }
    if (next_mode == LEPL_AUDIO_MODE_NONE)
    {
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
    }
    return WICED_SUCCESS;
}

wiced_result_t lepl_disconnect_device(uint16_t conn_id)
{
    wiced_result_t res;

    lepl_audio_mode_t audio_mode = lepl_cap_get_audio_mode();

    for (uint8_t i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[i];
        if (p_clcb->in_use)
        {
            app_rpc_send_app_status(p_clcb->conn_id,
                                    p_clcb->identity_bd_address,
                                    HCI_CONTROL_MISC_APP_STATE_DISCONNECTING,
                                    0);
            p_clcb->disconnecting = 1;
        }
    }

    if (audio_mode > LEPL_AUDIO_MODE_BROADCAST)
    {
        res = lepl_rpc_stop_streaming(conn_id, audio_mode, LEPL_AUDIO_MODE_NONE);
    }
    else
    {
        res = lepl_gatt_disconnect(conn_id);
    }
    return res;
}

static void lepl_rpc_handle_disconnect(uint8_t *p_data, uint32_t data_len)
{
    uint16_t conn_id;
    STREAM_TO_UINT16(conn_id, p_data);
    wiced_result_t res = lepl_disconnect_device(conn_id);
    WICED_BT_TRACE("[%s] conn %d status 0x%x\n", __FUNCTION__, conn_id, res);
}

void lepl_rpc_handle_play(uint8_t *p_data, uint32_t data_len)
{
    uint16_t conn_id;
    lepl_unicast_stream_config_t stream_config = {0};
    lepl_audio_config_t *p_audio_config = &stream_config.in_cfg;

    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
    STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
    STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
    STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
    STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);

    if (p_audio_config->sampling_frequency == 44100)
        stream_config.framing = WICED_BLE_ISOC_FRAMED;

    WICED_BT_TRACE("[%s] conn_id %d sampling freq %d frame duration %d sdu interval %d ocpf %d len %d\n",
                   __FUNCTION__,
                   conn_id,
                   p_audio_config->sampling_frequency,
                   p_audio_config->frame_duration,
                   p_audio_config->sdu_interval,
                   p_audio_config->octets_per_codec_frame,
                   data_len);

#ifdef ENABLE_LC3_PLUS
    // check if the codec config is lc3plus
    if ((p_audio_config->sampling_frequency == 96000) ||
        (p_audio_config->sampling_frequency == 48000 && p_audio_config->octets_per_codec_frame >= 160))
    {
        lc3_hrmode = true;
        WICED_BT_TRACE("[%s] currently using lc3plus codec config\n", __FUNCTION__);
    }
    else
    {
        lc3_hrmode = false;
    }
#endif /* ENABLE_LC3_PLUS */

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    if (current_mode != LEPL_AUDIO_MODE_NONE)
    {
        WICED_BT_TRACE_CRIT("[%s] Not ready to play! current mode: %d", __FUNCTION__, current_mode);
        return;
    }
    if (lepl_mcs_play(conn_id, &stream_config) == WICED_SUCCESS)
    {
#if defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
        le_pl_transcoding_data.conn_id = conn_id;
        le_pl_transcoding_data.is_unicast = 1;
        le_pl_transcoding_data.in_use = 1;
#endif // defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_MEDIA, &stream_config);
    }
}

void lepl_rpc_handle_pause(uint8_t *p_data, uint32_t data_len)
{
    uint16_t conn_id;

    STREAM_TO_UINT16(conn_id, p_data);

    WICED_BT_TRACE("[%s] conn_id %d len %d\n", __FUNCTION__, conn_id, data_len);

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    lepl_audio_mode_t final_mode = lepl_cap_get_final_audio_mode();

    if ((current_mode != LEPL_AUDIO_MODE_MEDIA) && (final_mode != LEPL_AUDIO_MODE_MEDIA))
    {
        WICED_BT_TRACE_CRIT("[%s] current mode: %d final mode %d ", __FUNCTION__, current_mode, final_mode);
        return;
    }

    if (lepl_mcs_pause(conn_id) == WICED_SUCCESS)
    {
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
    }
}

static void lepl_rpc_vcs_set_vol(uint8_t *p_data, uint32_t data_len, uint16_t opcode)
{
    uint16_t conn_id;
    uint8_t abs_vol = 0;
    ga_lib_vcs_volume_control_opcodes_t vcs_opcode;
    STREAM_TO_UINT16(conn_id, p_data);

    WICED_BT_TRACE("[%s] conn_id %d opcode %d len %d\n", __FUNCTION__, conn_id, opcode, data_len);

    switch (opcode)
    {
    case HCI_CONTROL_LE_AUDIO_COMMAND_VOL_UP:
        vcs_opcode = VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_UP;
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_VOL_DOWN:
        vcs_opcode = VOLUME_CONTROL_OPCODE_RELATIVE_VOLUME_DOWN;
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_UNMUTE_VOL_UP:
        vcs_opcode = VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_UP;
       break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_UNMUTE_VOL_DOWN:
        vcs_opcode = VOLUME_CONTROL_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN;
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_ABS_VOL:
        vcs_opcode = VOLUME_CONTROL_OPCODE_SET_ABSOLUTE_VOLUME;
        STREAM_TO_UINT8(abs_vol, p_data);
        break;
    default:
        return;
    }
    lepl_cap_vcp_set_volume(conn_id, vcs_opcode, abs_vol);
}

static void lepl_rpc_handle_set_mute_state(uint8_t *p_data, uint32_t payload_len, uint16_t opcode)
{
    uint16_t conn_id;
    uint8_t mute_state = 0;
    STREAM_TO_UINT16(conn_id, p_data);
    if (opcode == HCI_CONTROL_LE_AUDIO_COMMAND_MUTE) mute_state = 1;

    WICED_BT_TRACE("[%s] conn_id %d mute state %d\n", __FUNCTION__, conn_id, mute_state);
    lepl_cap_vcp_set_mute_state(conn_id, mute_state);
}

#define MAX_BROADCAST_CODE_LEN 16

void lepl_rpc_broadcast_src_handle_start_streaming(uint8_t *p_data, uint32_t data_len)
{
    ga_lib_bap_stream_config_t stream_config;
    uint8_t start;
    uint32_t codec_config;
    uint8_t enable_encryption;
    uint32_t channel_counts;
    uint32_t broadcast_id;
    uint8_t broadcast_code[MAX_BROADCAST_CODE_LEN];
    uint8_t bis_count;

    wiced_result_t ret_sts = WICED_ERROR;
#if defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
    // Store the configuration
    lepl_transcoding_save_broadcast_config(p_data, data_len);
#endif // defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
    STREAM_TO_UINT8(start, p_data);

    lepl_audio_mode_t mode = lepl_cap_get_audio_mode();

    if (start)
    {
        if(mode != LEPL_AUDIO_MODE_NONE)
        {
            WICED_BT_TRACE_CRIT("[%s] Not ready to play! mode: %d", __FUNCTION__, mode);
            return;
        }

        lepl_broadcast_source_cb_t *p_big = lehs_get_broadcast_source_cb();
        STREAM_TO_UINT32(codec_config, p_data);
        STREAM_TO_UINT8(enable_encryption, p_data);
        STREAM_TO_UINT32(channel_counts, p_data);
        STREAM_TO_UINT32(broadcast_id, p_data);
        STREAM_TO_ARRAY(broadcast_code, p_data, MAX_BROADCAST_CODE_LEN);
        STREAM_TO_UINT8(bis_count, p_data);

        WICED_BT_TRACE("[%s] Broadcast ID: %x", __FUNCTION__, broadcast_id);

        ga_lib_bap_get_broadcast_stream_config(codec_config, &stream_config);

#ifdef ENABLE_LC3_PLUS
        // check if codec config is lc3plus based on stream parameters
        if ((stream_config.sampling_frequency == 96000) ||
            (stream_config.sampling_frequency == 48000 && stream_config.octets_per_codec_frame >= 160))
        {
            lc3_hrmode = true;
            WICED_BT_TRACE("[%s] currently using lc3plus codec config\n", __FUNCTION__);
            // For lc3plus codec, inserting stereo data into one BIS will cause
            // the ISO data size to exceed the maximum SDU size (251 bytes).
            // Therefore, force adjustment to mono (1-channel) data.
            // TODO: may need to implement ISO data fragmentation in the app layer
            channel_counts = 1;
        }
        else
        {
            lc3_hrmode = false;
        }
#endif /* ENABLE_LC3_PLUS */

        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_BROADCAST, NULL);

        if (p_big->base.state == BAP_BROADCAST_STATE_IDLE)
        {
           ret_sts = lepl_bis_configure_stream(broadcast_id,
                                                broadcast_code,
                                                bis_count,
                                                channel_counts,
                                                stream_config.sampling_frequency,
                                                stream_config.frame_duration,
                                                stream_config.octets_per_codec_frame,
                                                enable_encryption);
            WICED_BT_TRACE("[%s] Cfg stream SF:%d FD:%d OPF:%d res:%d\n",
                           __FUNCTION__,
                           stream_config.sampling_frequency,
                           stream_config.frame_duration,
                           stream_config.octets_per_codec_frame,
                           ret_sts);
        }

        lepl_bis_start_stream(&stream_config);
        g_lepl_gatt_cb.audio_state.current_mode = LEPL_AUDIO_MODE_BROADCAST;
    }
    else
    {
        lepl_broadcast_source_cb_t *p_big = lehs_get_broadcast_source_cb();
        if (p_big->base.state != BAP_BROADCAST_STATE_IDLE)
        {
            lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
            ret_sts = lepl_bis_disable_stream();
            WICED_BT_TRACE_CRIT("[%s] disable_stream 0x%x\n", __FUNCTION__, ret_sts);

            ret_sts = lepl_bis_release_stream();
            WICED_BT_TRACE_CRIT("[%s] release_stream 0x%x\n", __FUNCTION__, ret_sts);
            g_lepl_gatt_cb.audio_state.current_mode = LEPL_AUDIO_MODE_NONE;
        }
    }
}

static void lepl_rpc_ccs_handle_generate_call_uri(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t uri_len, f_name_len;
    char call_uri[LEPL_TBS_BEARER_URI_MAX_SIZE] = {'\0'};
    char friendly_name[LEPL_TBS_FRIENDLY_NAME_MAX_SIZE] = {'\0'};

    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(uri_len, p_data);
    if (uri_len >= LEPL_TBS_BEARER_URI_MAX_SIZE)
        uri_len = LEPL_TBS_BEARER_URI_MAX_SIZE - 1;
    STREAM_TO_ARRAY(call_uri, p_data, uri_len);

    STREAM_TO_UINT8(f_name_len, p_data);
    if (f_name_len >= LEPL_TBS_FRIENDLY_NAME_MAX_SIZE)
        f_name_len = LEPL_TBS_FRIENDLY_NAME_MAX_SIZE - 1;
    STREAM_TO_ARRAY(friendly_name, p_data, f_name_len);

    WICED_BT_TRACE("[%s] conn_id %d call URI %s friendly_name %s  \n", __FUNCTION__, conn_id, call_uri, friendly_name);

    lepl_unicast_stream_config_t stream_config = {0};
    lepl_audio_config_t *p_audio_config = &stream_config.in_cfg;
    STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
    STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
    STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
    STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
    STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);

    p_audio_config = &stream_config.out_cfg;
    STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
    STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
    STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
    STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
    STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    if (current_mode == LEPL_AUDIO_MODE_NONE)
    {
        lepl_tbs_set_incoming_remote_call(&g_lepl_gatt_cb.local_service_data.gtbs, call_uri, friendly_name);
        if (lepl_rpc_ccs_set_incoming_remote_call(conn_id, &stream_config) != WICED_SUCCESS)
        {
            return;
        }
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_CALL, &stream_config);
        return;
    }

    if (lepl_rpc_stop_streaming(conn_id, current_mode, LEPL_AUDIO_MODE_CALL) == WICED_SUCCESS)
    {
        WICED_BT_TRACE("[%s] Not ready! current mode: %d ", __FUNCTION__, current_mode);
        lepl_tbs_set_incoming_remote_call(&g_lepl_gatt_cb.local_service_data.gtbs, call_uri, friendly_name);
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_CALL, &stream_config);
    }
}

static void lepl_rpc_ccs_simulate_remote_hold_retrieve_call(uint8_t *p_data, uint8_t data_len, wiced_bool_t hold)
{
    if (lepl_cap_get_audio_mode() != LEPL_AUDIO_MODE_CALL)
    {
        return;
    }
    uint8_t call_id;
    STREAM_TO_UINT8(call_id, p_data);
    WICED_BT_TRACE("[%s] call_id %d \n", __FUNCTION__, call_id);
    if (hold)
        lepl_rpc_ccs_set_remote_hold_call(call_id);
    else
        lepl_rpc_ccs_set_retrieve_remote_hold_call(call_id);
}

static void lepl_rpc_ccs_handle_terminate_call(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t call_id;

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    lepl_audio_mode_t final_mode = lepl_cap_get_final_audio_mode();

    if ((current_mode != LEPL_AUDIO_MODE_CALL) && (final_mode != LEPL_AUDIO_MODE_CALL))
    {
        WICED_BT_TRACE_CRIT("[%s] current mode: %d final mode %d ", __FUNCTION__, current_mode, final_mode);
        return;
    }

    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(call_id, p_data);
    lepl_tbs_terminate_call(conn_id, call_id, GA_LIB_TBS_SERVER_CALL_END);
}

void lepl_rpc_send_mic_state(uint8_t state)
{
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (g_lepl_gatt_cb.clcb[i].in_use)
        {
            le_audio_rpc_send_mic_state_update(g_lepl_gatt_cb.clcb[i].conn_id, state);
        }
    }
}

void lepl_rpc_send_convo_stream_state(uint8_t state)
{
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (g_lepl_gatt_cb.clcb[i].in_use)
        {
            le_audio_rpc_send_convo_stream_state_update(g_lepl_gatt_cb.clcb[i].conn_id, state);
        }
    }
}

void lepl_start_voice_capture(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] start voice assistant %x\n",__FUNCTION__, conn_id);

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("No clcb");
        return;
    }

    p_stream_config->context_type = BAP_CONTEXT_TYPE_VOICE_ASSISTANTS;
    lepl_cap_start_streaming(conn_id, p_stream_config);
    lepl_rpc_send_mic_state(1);
}

static void lepl_rpc_handle_start_stop_capture_voice(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    lepl_unicast_stream_config_t stream_config = {0};
    lepl_audio_config_t *p_audio_config = &stream_config.out_cfg;
    uint8_t start;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(start, p_data);

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    lepl_audio_mode_t final_mode = lepl_cap_get_final_audio_mode();
    WICED_BT_TRACE("[%s] current mode: %d final mode %d", __FUNCTION__, current_mode, final_mode);

    if (start)
    {
        STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
        STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
        STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
        STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
        STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);

        if (current_mode == LEPL_AUDIO_MODE_NONE)
        {
            lepl_start_voice_capture(conn_id, &stream_config);
        }
        else if (lepl_rpc_stop_streaming(conn_id, current_mode, LEPL_AUDIO_MODE_MIC))
        {
            WICED_BT_TRACE_CRIT("[%s] Not ready to start voice capture! current mode: %d ",
                                __FUNCTION__,
                                current_mode);
            return;
        }
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_MIC, &stream_config);
    }
    else
    {
        if ((current_mode != LEPL_AUDIO_MODE_MIC) && (final_mode != LEPL_AUDIO_MODE_MIC))
        {
            WICED_BT_TRACE_CRIT("[%s] current mode: %d final mode %d", __FUNCTION__, current_mode, final_mode);
            return;
        }
        lepl_cap_stop_streaming(conn_id);
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
    }
}

static void lepl_rpc_mics_mute(uint8_t* p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t mute;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(mute, p_data);

    lepl_cap_micp_mute(conn_id, mute);
}

static void lepl_rpc_mics_aics_mute(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t mute;
    uint32_t instance;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(mute, p_data);
    STREAM_TO_UINT32(instance, p_data);

    lepl_cap_micp_aics_mute(conn_id, instance, mute);
}

static void lepl_rpc_mics_aics_set_gain(uint8_t* p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t opcode;
    uint32_t instance;
    int8_t gain = 0;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(opcode, p_data);
    STREAM_TO_UINT32(instance, p_data);
    if (data_len - 7) STREAM_TO_INT8(gain, p_data);

    lepl_cap_micp_aics_set_gain(conn_id, instance, opcode, gain);
}

static void lepl_rpc_hap_read_preset(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    STREAM_TO_UINT16(conn_id, p_data);
    lepl_rpc_has_cp_read_preset_records(conn_id);
}

static void lepl_rpc_hap_write_preset_name(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t preset_index;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(preset_index, p_data);
    lepl_rpc_has_cp_set_preset_name(conn_id, preset_index, (char *)p_data);
}

static void lepl_rpc_hap_set_active_preset(uint8_t* p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t opcode;
    uint8_t preset_index;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(opcode, p_data);
    STREAM_TO_UINT8(preset_index, p_data);
    lepl_rpc_has_cp_set_active_preset(conn_id, opcode, preset_index);
}

void lepl_start_stop_bidir_mic(uint16_t conn_id, uint8_t start, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] start voice assistant %x\n", __FUNCTION__, conn_id);

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("No clcb");
        return;
    }

    if (start)
    {
        if ((p_clcb->pacs.available.snk_contexts & BAP_CONTEXT_TYPE_GAME) &&
            (p_clcb->pacs.available.src_contexts & BAP_CONTEXT_TYPE_GAME))
        {
            p_stream_config->context_type = BAP_CONTEXT_TYPE_GAME;
        }
        else if ((p_clcb->pacs.available.snk_contexts & BAP_CONTEXT_TYPE_CONVERSATIONAL) &&
                 (p_clcb->pacs.available.src_contexts & BAP_CONTEXT_TYPE_CONVERSATIONAL))
        {
            p_stream_config->context_type = BAP_CONTEXT_TYPE_CONVERSATIONAL;
        }
        else
        {
            p_stream_config->context_type = BAP_CONTEXT_TYPE_UNSPECIFIED;
        }
        lepl_cap_start_streaming(conn_id, p_stream_config);
    }
    else
    {
        lepl_cap_stop_streaming(conn_id);
    }
    lepl_rpc_send_convo_stream_state(start);
}

static void lepl_rpc_handle_start_stop_conv_streaming(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t start;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(start, p_data);

    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();
    lepl_audio_mode_t final_mode = lepl_cap_get_final_audio_mode();
    WICED_BT_TRACE("[%s] current mode: %d final mode %d", __FUNCTION__, current_mode, final_mode);

    if (start)
    {
        lepl_unicast_stream_config_t stream_config = {0};
        lepl_audio_config_t *p_audio_config = &stream_config.in_cfg;
        STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
        STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
        STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
        STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
        STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);

        p_audio_config = &stream_config.out_cfg;
        STREAM_TO_UINT8(p_audio_config->channel_count, p_data);
        STREAM_TO_UINT32(p_audio_config->sampling_frequency, p_data);
        STREAM_TO_UINT32(p_audio_config->frame_duration, p_data);
        STREAM_TO_UINT32(p_audio_config->sdu_interval, p_data);
        STREAM_TO_UINT16(p_audio_config->octets_per_codec_frame, p_data);
        stream_config.context_type = BAP_CONTEXT_TYPE_GAME;

        if (current_mode == LEPL_AUDIO_MODE_NONE)
        {
            lepl_start_stop_bidir_mic(conn_id, start, &stream_config);
        }
        else if (lepl_rpc_stop_streaming(conn_id, current_mode, LEPL_AUDIO_MODE_BIDIRMIC))
        {
            WICED_BT_TRACE_CRIT("[%s] Not ready to start conversation stream! current mode: %d ",
                                __FUNCTION__,
                                current_mode);

            return;
        }
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_BIDIRMIC, &stream_config);
    }
    else
    {
        if ((current_mode != LEPL_AUDIO_MODE_BIDIRMIC) && (final_mode != LEPL_AUDIO_MODE_BIDIRMIC))
        {
            return;
        }

        lepl_start_stop_bidir_mic(conn_id, start, NULL);
        lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
    }
}

static void lepl_rpc_csis_set_lock(uint8_t *p_data, uint8_t data_len)
{
    uint16_t conn_id;
    uint8_t lock;
    STREAM_TO_UINT16(conn_id, p_data);
    STREAM_TO_UINT8(lock, p_data);
    lepl_csis_set_lock(conn_id, lock);
}

#ifdef SIMULATED_NVRAM
static void lepl_rpc_handle_nvram_data(uint8_t *p_data, uint32_t data_len, uint8_t is_long)
{
    uint16_t nvram_id;
    uint16_t offset = 0;
    uint32_t len_to_write = data_len - 2;
    STREAM_TO_UINT16(nvram_id, p_data);
    if (is_long)
    {
        STREAM_TO_UINT16(offset, p_data);
        len_to_write -= 2;
    }
    WICED_BT_TRACE("[%s] nvram id 0x%x len %d\n", __FUNCTION__, nvram_id, len_to_write);
    lepl_nvram_write_data(nvram_id, p_data, len_to_write, offset);
}

static void lepl_rpc_handle_delete_nvram_data(uint8_t *p_data, uint32_t data_len)
{
    uint16_t nvram_id;
    STREAM_TO_UINT16(nvram_id, p_data);
    WICED_BT_TRACE("[%s] nvram id 0x%x \n", __FUNCTION__, nvram_id);
    lepl_nvram_delete_keys(nvram_id);
}

#endif //

#ifdef HS_SPK_ENABLED
static wiced_timer_t hci_control_update_role_timer;
static wiced_bt_device_address_t update_role_bda;
void hci_control_update_role_timer_cb(wiced_timer_callback_arg_t data)
{
    wiced_result_t status;
    wiced_bt_dev_role_t curr_role;
    status = wiced_bt_dev_get_role(update_role_bda, &curr_role, BT_TRANSPORT_BR_EDR);
    if (status == WICED_BT_SUCCESS)
    {
        // Switch role to master and disable role switch so we can be in master role.
        uint16_t new_link_policy = HCI_ENABLE_SNIFF_MODE;
        // Comment below two lines to avoid the role switch
        if (curr_role == HCI_ROLE_PERIPHERAL)
        {
            wiced_bt_dev_switch_role(update_role_bda, HCI_ROLE_CENTRAL, NULL);
        }
        wiced_bt_dev_set_link_policy(update_role_bda, &new_link_policy);
#ifdef DISABLE_3M_PKT
        wiced_bt_dev_setAclPacketTypes(update_role_bda,
                HCI_PKT_TYPES_MASK_DM5 | HCI_PKT_TYPES_MASK_DH5 | /* Use 1 mbps 5 slot packets */
                HCI_PKT_TYPES_MASK_DH3 | HCI_PKT_TYPES_MASK_DM3 | /* Use 1 mbps 3 slot packets */
                HCI_PKT_TYPES_MASK_DH1 | HCI_PKT_TYPES_MASK_DM1 | /* Use 1 mbps 1 slot packets */
                HCI_PKT_TYPES_MASK_NO_3_DH1 |               /* Don't use 3 mbps 1 slot packets */
                HCI_PKT_TYPES_MASK_NO_3_DH3 |               /* Don't use 3 mbps 3 slot packets */
                HCI_PKT_TYPES_MASK_NO_3_DH5);               /* Don't use 3 mbps 5 slot packets */
#endif // DISABLE_3M_PKT
    }
}

wiced_bool_t hci_control_connection_status_callback (wiced_bt_device_address_t bd_addr, uint8_t *p_features, wiced_bool_t is_connected, uint16_t handle, wiced_bt_transport_t transport, uint8_t reason)
{
    uint8_t event_data[2];

    //Build event payload
    event_data[0] = is_connected;
    event_data[1] = reason;
#ifdef HS_SPK_ENABLED
    if ( (transport == BT_TRANSPORT_BR_EDR) && (is_connected == WICED_TRUE))
    {
        wiced_init_timer(&hci_control_update_role_timer, hci_control_update_role_timer_cb, NULL, WICED_MILLI_SECONDS_TIMER);
        memcpy(update_role_bda, bd_addr, BD_ADDR_LEN);
        wiced_start_timer(&hci_control_update_role_timer, 50);
    }
#endif

    app_rpc_send_data( HCI_CONTROL_EVENT_CONNECTION_STATUS, event_data, 2 );

    WICED_BT_TRACE("%s  is_connected:%d reason:%x\n", __FUNCTION__, is_connected, reason );
    return WICED_TRUE;
}

void hci_control_audio_send_connect_complete( wiced_bt_device_address_t bd_addr, uint8_t status, uint32_t handle )
{
    int i;
    uint8_t event_data[BD_ADDR_LEN + sizeof(handle) + sizeof(uint8_t)];

    WICED_BT_TRACE( "[%s] %B status %x handle %x\n", __FUNCTION__, bd_addr, status, handle );

    //Build event payload
    if ( status == WICED_SUCCESS )
    {
        for ( i = 0; i < BD_ADDR_LEN; i++ )                     // bd address
            event_data[i] = bd_addr[BD_ADDR_LEN - 1 - i];

        event_data[i++] = handle & 0xff;                        //handle
        event_data[i++]   = ( handle >> 8 ) & 0xff;

        //event_data[i] = wiced_bt_rc_target_is_peer_absolute_volume_capable( );
        app_rpc_send_data( HCI_CONTROL_AUDIO_SINK_EVENT_CONNECTED, event_data, sizeof(event_data));
    }
    else
    {
        app_rpc_send_data( HCI_CONTROL_AUDIO_SINK_EVENT_CONNECTION_FAILED, NULL, 0 );
    }
}

/*
 *  send audio disconnect complete event to UART
 */
void hci_control_audio_send_disconnect_complete( uint32_t handle, uint8_t status, uint8_t reason )
{
    uint8_t event_data[4];

    WICED_BT_TRACE( "[%s] %04x status %d reason %d\n", __FUNCTION__, handle, status, reason );

    //Build event payload
    event_data[0] = handle & 0xff;                          //handle
    event_data[1] = ( handle >> 8 ) & 0xff;
    event_data[2] = status;                                 // status
    event_data[3] = reason;                                 // reason(1 byte)

    app_rpc_send_data( HCI_CONTROL_AUDIO_SINK_EVENT_DISCONNECTED, event_data, 4 );
}

/*
 *  send audio connect complete event to UART
 */
void hci_control_audio_send_started_stopped( uint32_t handle, wiced_bool_t started )
{
    uint8_t event_data[2];

    WICED_BT_TRACE( "[%s] handle %04x\n", __FUNCTION__, handle );

    //Build event payload
    event_data[0] = handle & 0xff;                          //handle
    event_data[1] = ( handle >> 8 ) & 0xff;

    app_rpc_send_data(started ? HCI_CONTROL_AUDIO_SINK_EVENT_STARTED : HCI_CONTROL_AUDIO_SINK_EVENT_STOPPED, event_data, 2);
}

void hci_control_avrc_send_connect_complete( wiced_bt_device_address_t bd_addr, uint8_t status, uint16_t handle )
{
    int i = 0;
    uint8_t event_data[BD_ADDR_LEN + sizeof(handle) + sizeof(uint8_t)];

    WICED_BT_TRACE( "[%s] %B status %x handle %x\n", __FUNCTION__, bd_addr, status, handle );

    //Build event payload
    if ( status == WICED_SUCCESS )
    {
        for ( ; i < BD_ADDR_LEN; i++ )                     // bd address
            event_data[i] = bd_addr[BD_ADDR_LEN - 1 - i];

        event_data[i++] = status;

        event_data[i++] = handle & 0xff;                        //handle
        event_data[i++] = ( handle >> 8 ) & 0xff;

    }
    else
    {
        event_data[i++] = status;
    }

    app_rpc_send_data( HCI_CONTROL_AVRC_CONTROLLER_EVENT_CONNECTED, event_data, i );
}

/*
 *  send avrcp controller disconnect complete event to UART
 */
void hci_control_avrc_send_disconnect_complete( uint16_t handle )
{
    uint8_t event_data[4];

    WICED_BT_TRACE( "[%s] handle: %04x\n", __FUNCTION__, handle );

    //Build event payload
    event_data[0] = handle & 0xff;                          //handle
    event_data[1] = ( handle >> 8 ) & 0xff;

    app_rpc_send_data( HCI_CONTROL_AVRC_CONTROLLER_EVENT_DISCONNECTED, event_data, 2 );
}

void hci_control_send_encryption_changed_evt( uint8_t encrypted ,  wiced_bt_device_address_t bdaddr )
{
    int i;
    uint8_t event_data[BD_ADDR_LEN + sizeof(uint8_t)];
    int     cmd_bytes = 0;

    event_data[cmd_bytes++] = encrypted;

    for ( i = 0 ; i < BD_ADDR_LEN; i++ )                     // bd address
        event_data[cmd_bytes++] = bdaddr[BD_ADDR_LEN - 1 - i];

    app_rpc_send_data( HCI_CONTROL_EVENT_ENCRYPTION_CHANGED, event_data, cmd_bytes );
}

void hci_control_send_pairing_completed_evt( uint8_t status , wiced_bt_device_address_t bdaddr )
{
    int i;

    uint8_t event_data[BD_ADDR_LEN + sizeof(uint8_t)];
    int     cmd_bytes = 0;

    event_data[cmd_bytes++] = status;

    for ( i = 0 ; i < BD_ADDR_LEN; i++ )                     // bd address
        event_data[cmd_bytes++] = bdaddr[BD_ADDR_LEN - 1 - i];

    WICED_BT_TRACE( "pairing complete evt: %B as %B status %d\n", bdaddr, &event_data[1], status );

    app_rpc_send_data( HCI_CONTROL_EVENT_PAIRING_COMPLETE, event_data, sizeof(event_data));
}

void _le_audio_rpc_handle_get_version(void)
{
    uint8_t tx_buf[15];
    uint8_t idx = 0;
    uint32_t chip = 55571;

    tx_buf[idx++] = 0;
    tx_buf[idx++] = 0;
    tx_buf[idx++] = 0;
    tx_buf[idx++] = 0;
    tx_buf[idx++] = 0;
    tx_buf[idx++] = chip & 0xFF;
    tx_buf[idx++] = (chip >> 8) & 0xFF;
    tx_buf[idx++] = (chip >> 24) & 0xFF;
    tx_buf[idx++] = 0; // not used

    /* Send MCU app the supported features */
    tx_buf[idx++] = HCI_CONTROL_GROUP_AVRC_CONTROLLER;
    tx_buf[idx++] = HCI_CONTROL_GROUP_HF;
    tx_buf[idx++] = HCI_CONTROL_GROUP_LE_AUDIO;

    app_rpc_send_data(HCI_CONTROL_MISC_EVENT_VERSION, tx_buf, idx);
}
#endif // HS_SPK_ENABLED

wiced_bool_t lepl_rpc_rx_cback(uint16_t opcode, uint8_t *p_data, uint32_t payload_len)
{
    wiced_bool_t b_response_sent = TRUE;

    WICED_BT_TRACE("[%s] [opcode 0x%04x] (%d bytes)\n", __FUNCTION__, opcode, payload_len);

    switch (opcode)
    {
    case HCI_CONTROL_MISC_COMMAND_GET_VERSION:
#ifdef HS_SPK_ENABLED
    {
        uint8_t tx_buf[8];
        uint8_t *p = tx_buf;
        uint32_t dev_role = HCI_CONTROL_LE_AUDIO_DEV_ROLE_UNICAST_SOURCE |
                HCI_CONTROL_LE_AUDIO_DEV_ROLE_BROADCAST_SOURCE |
                HCI_CONTROL_LE_AUDIO_DEV_ROLE_CALL_CONTROL_SERVER;
        UINT32_TO_STREAM(p, dev_role);
        _le_audio_rpc_handle_get_version();
        app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_DEVICE_ROLE, tx_buf, p - tx_buf);
    }
#else

        app_rpc_send_dev_role(HCI_CONTROL_LE_AUDIO_DEV_ROLE_UNICAST_SOURCE |
                                   HCI_CONTROL_LE_AUDIO_DEV_ROLE_BROADCAST_SOURCE |
                                   HCI_CONTROL_LE_AUDIO_DEV_ROLE_CALL_CONTROL_SERVER
                                   );
#endif
        extern wiced_bt_cfg_ble_t lepl_ble_cfg;
        app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_DEVICE_APPEARANCE, (uint8_t *)&lepl_ble_cfg.appearance, 2);
        break;

    case HCI_CONTROL_LE_COMMAND_ADVERTISE:
        lepl_rpc_handle_adv(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_COMMAND_SCAN:
        lepl_rpc_handle_scan(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_COMMAND_CONNECT:
        lepl_rpc_handle_connect(p_data, payload_len, 0);
        break;
    case HCI_CONTROL_LE_COMMAND_CANCEL_CONNECT:
        lepl_rpc_handle_connect(p_data, payload_len, 1);
        break;
    case HCI_CONTROL_LE_COMMAND_DISCONNECT:
        lepl_rpc_handle_disconnect(p_data, payload_len);
        break;
    case HCI_CONTROL_MISC_COMMAND_PING:
        app_rpc_send_data(HCI_CONTROL_MISC_EVENT_PING_REPLY, p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_PLAY:
        lepl_rpc_handle_play(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_PAUSE:
        lepl_rpc_handle_pause(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_VOL_UP:
    case HCI_CONTROL_LE_AUDIO_COMMAND_VOL_DOWN:
    case HCI_CONTROL_LE_AUDIO_COMMAND_ABS_VOL:
    case HCI_CONTROL_LE_AUDIO_COMMAND_UNMUTE_VOL_UP:
    case HCI_CONTROL_LE_AUDIO_COMMAND_UNMUTE_VOL_DOWN:
        lepl_rpc_vcs_set_vol(p_data, payload_len, opcode);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_MUTE:
    case HCI_CONTROL_LE_AUDIO_COMMAND_UNMUTE:
        lepl_rpc_handle_set_mute_state(p_data, payload_len, opcode);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_BROADCAST_SOURCE_START_STREAMIMG:
        lepl_rpc_broadcast_src_handle_start_streaming(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_PLACE_CALL:
        lepl_rpc_ccs_handle_generate_call_uri(p_data, payload_len);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_REM_HOLD_CALL:
        lepl_rpc_ccs_simulate_remote_hold_retrieve_call(p_data, payload_len, TRUE);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_REM_HOLD_RETRIEVE:
        lepl_rpc_ccs_simulate_remote_hold_retrieve_call(p_data, payload_len, FALSE);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_TERMINATE_CALL:
        lepl_rpc_ccs_handle_terminate_call(p_data, payload_len);
        break;

    case HCI_CONTROL_LE_AUDIO_COMMAND_START_STOP_MIC:
        lepl_rpc_handle_start_stop_capture_voice(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_MICP_MUTE:
        lepl_rpc_mics_mute(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_MICP_AICS_MUTE:
        lepl_rpc_mics_aics_mute(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_MICP_AICS_SET_GAIN:
        lepl_rpc_mics_aics_set_gain(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_HAS_READ_PRESET:
        lepl_rpc_hap_read_preset(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_HAS_WRITE_PRESET_NAME:
        lepl_rpc_hap_write_preset_name(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_HAS_SET_PRESET:
        lepl_rpc_hap_set_active_preset(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_CONVO_STREAM_START_STOP:
        lepl_rpc_handle_start_stop_conv_streaming(p_data, payload_len);
        break;
    case HCI_CONTROL_LE_AUDIO_COMMAND_SET_LOCK_STATE:
        lepl_rpc_csis_set_lock(p_data, payload_len);
        break;
#ifdef SIMULATED_NVRAM
    case HCI_CONTROL_COMMAND_PUSH_NVRAM_DATA:
        lepl_rpc_handle_nvram_data(p_data, payload_len, FALSE);
        break;
    case HCI_CONTROL_COMMAND_DELETE_NVRAM_DATA:
        lepl_rpc_handle_delete_nvram_data(p_data, payload_len);
        break;
    case HCI_CONTROL_COMMAND_PUSH_NVRAM_DATA_LONG:
        lepl_rpc_handle_nvram_data(p_data, payload_len, TRUE);
        break;
#endif // SIMULATED_NVRAM
#ifdef HS_SPK_ENABLED
   case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_VOLUME_UP:
       WICED_BT_TRACE("[%s] VOLUME_UP_NEXT_TRACK_BUTTON - BUTTON_CLICK_EVENT - BUTTON_STATE_RELEASED\n", __FUNCTION__);
       bt_hs_spk_app_service_action_run(ACTION_VOLUME_UP);
       break;
   case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_VOLUME_DOWN:
       WICED_BT_TRACE("[%s] VOLUME_DOWN_PREVIOUS_TRACK_BUTTON - BUTTON_CLICK_EVENT - BUTTON_STATE_RELEASED\n", __FUNCTION__);
       bt_hs_spk_app_service_action_run(ACTION_VOLUME_DOWN);
       break;
   case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_PLAY:
   case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_PAUSE:
       extern wiced_result_t service_pause_play(void);
       WICED_BT_TRACE("[%s] PLAY_PAUSE_BUTTON - BUTTON_CLICK_EVENT - BUTTON_STATE_RELEASED\n", __FUNCTION__);
       bt_hs_spk_app_service_action_run(ACTION_PAUSE_PLAY);
       break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_NEXT_TRACK:
        bt_hs_spk_app_service_action_run(ACTION_FORWARD);
        WICED_BT_TRACE("[%s] NEXT TRACK\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_PREVIOUS_TRACK:
        bt_hs_spk_app_service_action_run(ACTION_BACKWARD);
        WICED_BT_TRACE("[%s] PREVIOUS TRACK\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_STOP:
        bt_hs_spk_app_service_action_run(ACTION_STOP);
        WICED_BT_TRACE("[%s] STOP\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_BEGIN_FAST_FORWARD:
        bt_hs_spk_app_service_action_run(ACTION_FAST_FORWARD_HELD);
        WICED_BT_TRACE("[%s] BEGIN FAST FORWARD\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_END_FAST_FORWARD:
        bt_hs_spk_app_service_action_run(ACTION_FAST_FORWARD_RELEASE);
        WICED_BT_TRACE("[%s] END FAST FORWARD\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_BEGIN_REWIND:
        bt_hs_spk_app_service_action_run(ACTION_FAST_REWIND_HELD);
        WICED_BT_TRACE("[%s] BEGIN REWIND\n", __FUNCTION__);
        break;

    case HCI_CONTROL_AVRC_CONTROLLER_COMMAND_END_REWIND:
        bt_hs_spk_app_service_action_run(ACTION_FAST_REWIND_RELEASE);
        WICED_BT_TRACE("[%s] END REWIND\n", __FUNCTION__);
        break;
#endif // HS_SPK_ENABLED
    default:
        b_response_sent = FALSE;
        WICED_BT_TRACE("[%s] Unknown Function code [%d] \n", __FUNCTION__, opcode);
        break;
    }

    return b_response_sent;
}

void lepl_rpc_init(uint8_t app_instance)
{
    /* RPC to work with LE Audio Client Control */
    app_rpc_init(app_instance, lepl_rpc_rx_cback, TRUE);
}
