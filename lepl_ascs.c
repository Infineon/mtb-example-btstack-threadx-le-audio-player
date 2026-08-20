/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"

void print_all_ases(char *from, lepl_clcb_t *p_clcb, ga_lib_ascs_characteristics_t ase_type)
{
    int limit = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? LEPL_ASCS_MAX_SNK_CHARACTERISTICS
                                                                 : LEPL_ASCS_MAX_SRC_CHARACTERISTICS;
    WICED_BT_TRACE("[%s] printing all ases for conn_id %d, limit %d", from, p_clcb->conn_id, limit);
    for (int i = 0; i < limit; i++)
    {
        lepl_ase_data_t *p_lepl_ase = &p_clcb->ases[i];
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        WICED_BT_TRACE("[%s] index %d, conn_id 0x%x 0x%x  ase_id %d type %d", from,
                       i,
                       p_lepl_ase->acl_conn_handle,
                       p_ase,
                       p_ase->ase_id,
                       p_ase->ase_type);
    }
}

 // find the first free slot and assign ASE ID
lepl_ase_data_t *lepl_assign_remote_ase_id(lepl_clcb_t *p_clcb, uint8_t ase_id,
                                           ga_lib_ascs_characteristics_t ase_type)
{
    int limit = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? LEPL_ASCS_MAX_SNK_CHARACTERISTICS
                                                          : LEPL_ASCS_MAX_SRC_CHARACTERISTICS;

    print_all_ases("before_assign", p_clcb, ase_type);

    for (int i = 0; i < limit; i++)
    {
        lepl_ase_data_t *p_lepl_ase = &p_clcb->ases[i];
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        WICED_BT_TRACE("[%s] index %d, 0x%x  ase_id %d type %d", __FUNCTION__, i, p_ase, p_ase->ase_id, p_ase->ase_type);

        // ASE ID value of 0, means unassigned
        if (0 == p_ase->ase_id)
        {
            WICED_BT_TRACE("[%s] index %d. ase_id %d type %d inst %d",
                           __FUNCTION__,
                           i,
                           ase_id,
                           ase_type,
                           i);
            p_ase->ase_id = ase_id;
            /* The ASE type is the local counterpart of the server characteristic */
            p_ase->ase_type =
                (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE : GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE;
            p_ase->data_path_dir =
                (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? WICED_BLE_ISOC_DPD_INPUT : WICED_BLE_ISOC_DPD_OUTPUT;

            p_lepl_ase->acl_conn_handle = p_clcb->acl_handle;

            print_all_ases("after_assign_0", p_clcb, ase_type);
            return p_lepl_ase;
        }
    }

    print_all_ases("after_assign_none", p_clcb, ase_type);
    return NULL;
}

// Get remote ASE based on type and index, start at *p_index if provided
lepl_ase_data_t *lepl_get_remote_ase(lepl_clcb_t *p_clcb, ga_lib_ascs_characteristics_t ase_type, uint8_t *p_index)
{
    int index = (p_index) ? *p_index : 0;

    for (int i = index; i < LEPL_MAX_ASES; i++)
    {
        lepl_ase_data_t *p_lepl_ase = &p_clcb->ases[i];
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

        if (p_ase->ase_id && (p_ase->ase_type == ase_type))
        {
            if (p_index)
            {
                *p_index = i + 1;
            }

            return p_lepl_ase;
        }
    }
    return NULL;
}

// Get remote ASE based on ASE ID for a given connection
lepl_ase_data_t *lepl_get_remote_ase_data_by_ase_id(lepl_clcb_t *p_clcb, uint8_t ase_id)
{
    for (int index = 0; index < LEPL_MAX_ASES; index++)
    {
        lepl_ase_data_t *p_lepl_ase = &p_clcb->ases[index];
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        if (p_ase->ase_id == ase_id)
        {
            return p_lepl_ase;
        }
    }
    return NULL;
}

// Handle ASE notification and perform actions based on ASE state
wiced_bool_t lepl_handle_ase_notification(uint16_t conn_id, lepl_clcb_t *p_clcb,lepl_ase_data_t *p_lepl_ase)
{
    wiced_result_t data_path_setup_sts = WICED_ERROR;

    if (!p_lepl_ase)
    {
        WICED_BT_TRACE_CRIT("[%s] p_lepl_ase is null", __FUNCTION__);
        return TRUE;
    }

    //TODO: validate state before using CIS/CIG id.. create separate fn for common use..
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    WICED_BT_TRACE("[%s] p_ase 0x%x 0x%x, 0x%x", __FUNCTION__, p_ase, p_ase->ase_state, p_lepl_ase->cis_conn_handle);

    switch (p_ase->ase_state) {
        case GA_LIB_ASCS_STATE_ENABLING:
            //if client is sink setup datapath and send rcr start ready
            if (GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE == p_ase->ase_type)
            {
                if (!p_lepl_ase->data_path_established && p_lepl_ase->cis_conn_handle)
                {
                    data_path_setup_sts = lepl_isoc_dhm_setup_cis_datapath(p_lepl_ase);

                    if (data_path_setup_sts) {
                        WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful..(err:%d)\n",
                                            __FUNCTION__,
                                            data_path_setup_sts);
                        return FALSE;
                    }
                }
            }
            break;

        case GA_LIB_ASCS_STATE_STREAMING:
		WICED_BT_TRACE("[%s] type %d",__FUNCTION__, p_ase->ase_type);
            if (!p_lepl_ase->data_path_established) {
                data_path_setup_sts = lepl_isoc_dhm_setup_cis_datapath(p_lepl_ase);

                if (data_path_setup_sts) {
                    WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful..(err:%d)\n",
                                        __FUNCTION__,
                                        data_path_setup_sts);
                    return FALSE;
                }
            }
            else
            {
                lepl_isoc_dhm_start_cis_stream(p_lepl_ase->cis_conn_handle, p_ase->data_path_dir);
            }
            break;

        case GA_LIB_ASCS_STATE_DISABLING:
        {
            // TODO: prepare for not receiving audio (ALSA config if any)
            // (source will not send data, datapath and CIS connection will be active..)
            ga_lib_ascs_send_receiver_stop_ready(conn_id,
                                              &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT],
                                              p_ase->ase_id);
        }
        break;

        case GA_LIB_ASCS_STATE_RELEASING:
            // Disconnect CIS after data path removal is successful
            if (!lepl_isoc_dhm_remove_cis_datapath(p_lepl_ase))
            {
                if (!p_lepl_ase->cis_conn_handle)
                {
                    // Get CIS connection handle if it's in connecting state
                    p_lepl_ase->cis_conn_handle =
                        wiced_ble_isoc_central_get_cis_conn_handle(p_ase->qos_configured.cig_id,
                                                                                 p_ase->qos_configured.cis_id);
                }
                if (wiced_ble_isoc_disconnect_cis(p_lepl_ase->cis_conn_handle) != WICED_SUCCESS)
                {
                    // Remove CIG if CIS is not connected
                    wiced_ble_isoc_central_remove_cig(p_ase->qos_configured.cig_id);
                }
            }
            break;

        default:
            break;
    }
    return TRUE;
}

void lepl_set_default_ase_params(lepl_ase_data_t *p_lepl_ase, wiced_bool_t is_client)
{
    if (!p_lepl_ase)
    {
        return;
    }

    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    p_ase->ase_state = GA_LIB_ASCS_STATE_IDLE;

    p_ase->qos_configured.cig_id = 0xFF;
    p_ase->qos_configured.cis_id = 0xFF;

    WICED_BT_TRACE("[%s] ase_id %d, ase_state 0x%x type %s cis_handle %d",
                   __FUNCTION__,
                   p_ase->ase_id,
                   p_ase->ase_state,
                   (p_ase->ase_type == GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE) ? "src" : "sink",
                   p_lepl_ase->cis_conn_handle);
}

void lepl_ascs_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data)
{
    int max_indexes =
        GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE + LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS;
    int index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.ascs,
                                                                   max_indexes,
                                                                   p_gatt_data->handle);
    uint8_t *p_data = p_gatt_data->p_data;

    if (index == GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT)
    {
        uint8_t opcode, num_of_ase;

        STREAM_TO_UINT8(opcode, p_data);
        STREAM_TO_UINT8(num_of_ase, p_data);

        // if the response code is Unsupported Opcode / Invalid Length num_of_ase is set to 0xFF
        if (0xFF == num_of_ase)
            num_of_ase = 1;

        for (uint8_t i = 0; i < num_of_ase; i++)
        {
            uint8_t ase_id, response_code, reason;

            STREAM_TO_UINT8(ase_id, p_data);
            STREAM_TO_UINT8(response_code, p_data);
            STREAM_TO_UINT8(reason, p_data);
            WICED_BT_TRACE("[%s] ase %d op %d rsp %d reason %d ", __FUNCTION__, ase_id, opcode, response_code, reason);

            UNUSED_VARIABLE(ase_id);
            UNUSED_VARIABLE(reason);
            UNUSED_VARIABLE(opcode);

            if (response_code != GA_LIB_ASCS_CP_RESPONSE_SUCCESS)
            {
                lepl_cap_handle_ascs_error_response(conn_id);
            }
        }

        return;
    }
    // handle ase state notification read response, update the state and send to application
    else if (index < max_indexes)
    {
        lepl_ase_data_t *p_lepl_ase;
        ga_lib_ascs_characteristics_t ase_type = GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE;

        if (index >= (GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE + LEPL_ASCS_MAX_SNK_CHARACTERISTICS))
        {
            ase_type = GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE;
        }
        uint8_t ase_id, ase_state;
        STREAM_TO_UINT8(ase_id, p_data);
        STREAM_TO_UINT8(ase_state, p_data);

        p_lepl_ase = lepl_get_remote_ase_data_by_ase_id(p_clcb, ase_id);
        if (!p_lepl_ase)
        {
            WICED_BT_TRACE("[%s] No ASE found for ASE ID %d", __FUNCTION__, ase_id);
            p_lepl_ase = lepl_assign_remote_ase_id(p_clcb, ase_id, ase_type);
            if (!p_lepl_ase)
            {
                WICED_BT_TRACE_CRIT("[%s] Could not assign remote ase %d type %d", __FUNCTION__, ase_id, ase_type);
                return;
            }
        }
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        p_ase->ase_state = ase_state;

        switch (ase_state)
        {
        case GA_LIB_ASCS_STATE_IDLE:
        {
            // reset the ASE data except ASE ID and type, as they are associated with the characteristic instance
            WICED_MEMSET(&p_ase->ase_cfg, 0, sizeof(ga_lib_ascs_ase_codec_config_data_t));
            WICED_MEMSET(&p_ase->qos_configured, 0, sizeof(ga_lib_ascs_config_qos_args_t));
            WICED_MEMSET(&p_ase->metadata, 0, sizeof(ga_lib_ascs_metadata_t));
        }
        break;
        case GA_LIB_ASCS_STATE_CODEC_CONFIGURED:
        {
            ga_lib_ascs_ase_codec_config_data_t *p_ase_cfg = &p_lepl_ase->ase.ase_cfg;
            ga_lib_ascs_csc_t *p_remote_cc = &p_ase_cfg->csc;

            //WICED_MEMSET(p_remote_cc, 0, sizeof(ga_lib_ascs_config_codec_args_t));
            WICED_MEMSET(p_ase_cfg, 0, sizeof(ga_lib_ascs_ase_codec_config_data_t));

            STREAM_TO_UINT8(p_ase_cfg->supported_framing, p_data);
            STREAM_TO_UINT8(p_ase_cfg->preferred_phy, p_data);
            STREAM_TO_UINT8(p_ase_cfg->preferred_retransmission_number, p_data);
            STREAM_TO_UINT16(p_ase_cfg->max_transport_latency, p_data);

            STREAM_TO_UINT24(p_ase_cfg->presentation_delay_in_us_min, p_data);
            STREAM_TO_UINT24(p_ase_cfg->presentation_delay_in_us_max, p_data);
            STREAM_TO_UINT24(p_ase_cfg->preferred_presentation_delay_in_us_min, p_data);
            STREAM_TO_UINT24(p_ase_cfg->preferred_presentation_delay_in_us_max, p_data);

            STREAM_TO_UINT8(p_ase_cfg->codec_id.coding_format, p_data);
            STREAM_TO_UINT16(p_ase_cfg->codec_id.company_id, p_data);
            STREAM_TO_UINT16(p_ase_cfg->codec_id.vendor_specific_codec_id, p_data);

            int csc_length = 0;

            STREAM_TO_UINT8(csc_length, p_data);

            p_data += ga_lib_bap_parse_csc(p_data, csc_length, p_remote_cc);
        }
        break;
        case GA_LIB_ASCS_STATE_QOS_CONFIGURED:
        {
            ga_lib_ascs_config_qos_args_t *p_qos = &p_ase->qos_configured;

            STREAM_TO_UINT8(p_qos->cig_id, p_data);
            STREAM_TO_UINT8(p_qos->cis_id, p_data);

            REVERSE_STREAM_TO_ARRAY(&p_qos->sdu_interval, p_data, SDU_INTERVAL_LENGTH);
            STREAM_TO_UINT8(p_qos->framing, p_data);
            STREAM_TO_UINT8(p_qos->phy, p_data);
            STREAM_TO_UINT16(p_qos->max_sdu, p_data);
            STREAM_TO_UINT8(p_qos->retransmission_number, p_data);
            STREAM_TO_UINT16(p_qos->max_transport_latency, p_data);
            REVERSE_STREAM_TO_ARRAY(&p_qos->presentation_delay, p_data, PRESENTATION_DELAY_LENGTH);
        }
        break;
        case GA_LIB_ASCS_STATE_ENABLING:
        {
            wiced_result_t data_path_setup_sts = WICED_ERROR;
            uint8_t cig_id, cis_id;
            uint16_t cis_conn_handle = p_lepl_ase->cis_conn_handle;

            STREAM_TO_UINT8(cig_id, p_data);
            STREAM_TO_UINT8(cis_id, p_data);

            UNUSED_VARIABLE(cig_id);
            UNUSED_VARIABLE(cis_id);

            //if client is sink setup datapath and send rcr start ready
            if (GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE == p_ase->ase_type)
            {
                if (!p_lepl_ase->data_path_established && cis_conn_handle)
                {
                    data_path_setup_sts = lepl_isoc_dhm_setup_cis_datapath(p_lepl_ase);

                    if (data_path_setup_sts)
                    {
                        WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful..(err:%d)\n",
                                            __FUNCTION__,
                                            data_path_setup_sts);
                        return;
                    }
                }
            }
        }
        break;
        case GA_LIB_ASCS_STATE_STREAMING:
        {
            wiced_result_t data_path_setup_sts = WICED_ERROR;

            WICED_BT_TRACE("[%s] type %d", __FUNCTION__, p_lepl_ase->ase.ase_type);
            if (!p_lepl_ase->data_path_established)
            {
                data_path_setup_sts = lepl_isoc_dhm_setup_cis_datapath(p_lepl_ase);

                if (data_path_setup_sts)
                {
                    WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful..(err:%d)\n",
                                        __FUNCTION__,
                                        data_path_setup_sts);
                    return;
                }
            }
            else
            {
                lepl_isoc_dhm_start_cis_stream(p_lepl_ase->cis_conn_handle, p_lepl_ase->ase.data_path_dir);
            }
        }
        break;
        case GA_LIB_ASCS_STATE_DISABLING:
        {
            // TODO: prepare for not receiving audio (ALSA config if any)
            // (source will not send data, datapath and CIS connection will be active..)
            ga_lib_ascs_send_receiver_stop_ready(
                conn_id,
                &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT],
                p_ase->ase_id);
        }
        break;
        case GA_LIB_ASCS_STATE_RELEASING:
            // Disconnect CIS after data path removal is successful
            if (!lepl_isoc_dhm_remove_cis_datapath(p_lepl_ase))
            {
                uint16_t cis_conn_handle = p_lepl_ase->cis_conn_handle;
                if (!cis_conn_handle)
                {
                    // Get CIS connection handle if it's in connecting state
                    cis_conn_handle = wiced_ble_isoc_central_get_cis_conn_handle(p_ase->qos_configured.cig_id,
                                                                                 p_ase->qos_configured.cis_id);
                }
                if (wiced_ble_isoc_disconnect_cis(cis_conn_handle) != WICED_SUCCESS)
                {
                    // Remove CIG if CIS is not connected
                    wiced_ble_isoc_central_remove_cig(p_ase->qos_configured.cig_id);
                }
            }
            break;
        default:
            break;
        }

        lepl_cap_ascs_state_update(conn_id, p_clcb, p_lepl_ase);
    }
}

wiced_result_t lepl_ascs_write_control_point_config_codec(lepl_clcb_t *p_clcb,
                                                          ga_lib_ascs_ase_t *p_ase,
                                                          ga_lib_ascs_config_codec_args_t *p_codec_arg)
{
    uint8_t buff[256];
    uint8_t *p_data = buff;
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];

    UINT8_TO_STREAM(p_data, GA_LIB_ASCS_OPCODE_CONFIG_CODEC);
    UINT8_TO_STREAM(p_data, 1); // num of ASE

    p_data += ga_lib_ascs_build_cp_config_codec(p_data, sizeof(buff) - (p_data - buff), p_ase->ase_id, p_codec_arg);

    return gatt_intf_write_no_rsp(p_clcb->conn_id, p_handle, buff, (uint16_t)(p_data - buff));
}

wiced_result_t lepl_ascs_write_control_point_config_qos(lepl_clcb_t *p_clcb,
                                                        lepl_ase_data_t *p_lepl_ase,
                                                        ga_lib_ascs_config_qos_args_t *p_qos_arg)
{
    uint8_t buff[256];
    uint8_t *p_data = buff;
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    UINT8_TO_STREAM(p_data, GA_LIB_ASCS_OPCODE_CONFIG_QOS);
    UINT8_TO_STREAM(p_data, 1); // num of ASE

    p_data += ga_lib_ascs_build_cp_config_qos(p_data, sizeof(buff) - (p_data - buff), p_ase->ase_id, p_qos_arg);
    return gatt_intf_write_no_rsp(p_clcb->conn_id, p_handle, buff, (uint16_t)(p_data - buff));
}

wiced_result_t lepl_ascs_write_control_point_enable(lepl_clcb_t *p_clcb,
                                                    lepl_ase_data_t *p_lepl_ase)
{
    uint8_t buff[256];
    uint8_t *p_data = buff;
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    UINT8_TO_STREAM(p_data, GA_LIB_ASCS_OPCODE_ENABLE);
    UINT8_TO_STREAM(p_data, 1); // num of ASE

    p_data += ga_lib_ascs_build_cp_metadata_cmd(p_data, sizeof(buff) - (p_data - buff), p_ase->ase_id, &p_ase->metadata);
    return gatt_intf_write_no_rsp(p_clcb->conn_id, p_handle, buff, (uint16_t)(p_data - buff));
}

wiced_result_t lepl_ascs_write_control_point_update_metadata(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase)
{
    uint8_t buff[256];
    uint8_t *p_data = buff;
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    UINT8_TO_STREAM(p_data, GA_LIB_ASCS_OPCODE_UPDATE_METADATA);
    UINT8_TO_STREAM(p_data, 1); // num of ASE

    p_data += ga_lib_ascs_build_cp_metadata_cmd(p_data, sizeof(buff) - (p_data - buff), p_ase->ase_id, &p_ase->metadata);
    return gatt_intf_write_no_rsp(p_clcb->conn_id, p_handle, buff, (uint16_t)(p_data - buff));
}

wiced_result_t lepl_ascs_write_control_point_disable(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase)
{
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    return ga_lib_ascs_send_disable(p_clcb->conn_id, p_handle, p_lepl_ase->ase.ase_id);
}

wiced_result_t lepl_ascs_write_control_point_release(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase)
{
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    return ga_lib_ascs_send_release(p_clcb->conn_id, p_handle, p_lepl_ase->ase.ase_id);
}

wiced_result_t lepl_ascs_write_control_point_receiver_start_ready(lepl_clcb_t *p_clcb,
                                                                  lepl_ase_data_t *p_lepl_ase)
{
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    return ga_lib_ascs_send_receiver_start_ready(p_clcb->conn_id, p_handle, p_lepl_ase->ase.ase_id);
}

wiced_result_t lepl_ascs_write_control_point_receiver_stop_ready(lepl_clcb_t *p_clcb,
                                                                 lepl_ase_data_t *p_lepl_ase)
{
    gatt_intf_characteristic_handles_t *p_handle = &p_clcb->peer_profiles.ascs[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT];
    return ga_lib_ascs_send_receiver_stop_ready(p_clcb->conn_id, p_handle, p_lepl_ase->ase.ase_id);
}

// Helper function to count number of bits set in a value
static uint8_t get_num_bit_set(uint32_t val)
{
    uint8_t count = 0;

    if (val == 0)
        return val;

    while (val > 0)
    {
        val &= (val - 1);
        count++;
    }
    return count;
}

// Compare codec parameters with PACS capabilities
wiced_bool_t lepl_ascs_compare_codec_param(le_audio_utils_pacs_t *p_pac,
                                           ga_lib_ascs_config_codec_args_t *p_codec_arg)
{
    uint8_t num_ch = 1;
    uint32_t block_per_sdu = 0;

    if (p_codec_arg->csc.octets_per_codec_frame)
    {
        // verify octed per frame
        if ((p_pac->min_data_per_frame > p_codec_arg->csc.octets_per_codec_frame) ||
            (p_pac->max_data_per_frame < p_codec_arg->csc.octets_per_codec_frame))
        {
            WICED_BT_TRACE("BAP_CODEC_CONFIG_OCTET_PER_CODEC_FRAME_TYPE Error. OPF %d requested min %d max %d ",
                           p_codec_arg->csc.octets_per_codec_frame,
                           p_pac->min_data_per_frame,
                           p_pac->max_data_per_frame);
            return WICED_FALSE;
        }
    }

    if (p_codec_arg->csc.sampling_frequency_hz)
    {
        uint8_t bit_index = ga_lib_bap_get_sampling_freq_index(p_codec_arg->csc.sampling_frequency_hz);
        if (!bit_index)
        {
            WICED_BT_TRACE("Invalid sampling freq %d ", p_codec_arg->csc.sampling_frequency_hz);
            return WICED_FALSE;
        }
        bit_index -= 1; // convert to supported sf LTV structures
        if (!(p_pac->sf & (1 << bit_index)))
        {
            WICED_BT_TRACE("BAP_CODEC_CONFIG_SAMPLING_FREQUENCY_TYPE Error. ");
            return WICED_FALSE;
        }
    }

    if (p_codec_arg->csc.frame_duration_us)
    {
        // verify frame duration
        uint8_t bit_index = ga_lib_bap_get_frame_duration_index(p_codec_arg->csc.frame_duration_us);
        if (bit_index == 0xFF)
        {
            WICED_BT_TRACE("Invalid frame duration %d ", p_codec_arg->csc.frame_duration_us);
            return WICED_FALSE;
        }
        if (!(p_pac->frame_duration & (1 << bit_index)))
        {
            WICED_BT_TRACE("BAP_CODEC_CONFIG_FRAME_DURATION_TYPE Error. ");
            return WICED_FALSE;
        }
    }

    if (p_codec_arg->csc.audio_channel_allocation)
    {
        num_ch = get_num_bit_set(p_codec_arg->csc.audio_channel_allocation);
    }

    if (p_codec_arg->csc.lc3_blocks_per_sdu)
    {
        block_per_sdu = p_codec_arg->csc.lc3_blocks_per_sdu;
    }

    if (num_ch && block_per_sdu)
    {
        // check lc3 block per sdu & channel allocation
        if ((block_per_sdu * num_ch) < p_pac->frame_per_sdu)
            return WICED_FALSE;
    }

    return WICED_TRUE;
}
