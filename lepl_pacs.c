/*
 * $ Copyright Cypress Semiconductor $
 */
#include "lepl.h"


void lepl_pacs_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data)
{
    int max_indexes =
        GA_LIB_PACS_CHARACTERISTIC_MAX_UNIQUE + LEPL_PACS_MAX_SNK_CHARACTERISTICS + LEPL_PACS_MAX_SRC_CHARACTERISTICS;
    int index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.pacs,
                                                                   max_indexes,
                                                                   p_gatt_data->handle);
    ga_lib_pacs_data_t *p_pacs = &p_clcb->pacs;
    uint8_t *p_data = p_gatt_data->p_data;
    uint16_t val_len = p_gatt_data->len;

    WICED_BT_TRACE("[%s] p_clcb 0x%x status 0x%x index %d handle 0x%x max index %d",
                   __FUNCTION__,
                   p_clcb,
                   status,
                   index,
                   p_gatt_data->handle,
                   max_indexes);

    if (index == GA_LIB_PACS_CHARACTERISTIC_AVAILABILE_AUDIO_CONTEXTS)
    {
        STREAM_TO_UINT16(p_pacs->available.snk_contexts, p_data);
        STREAM_TO_UINT16(p_pacs->available.src_contexts, p_data);
        WICED_BT_TRACE("[%s] AVAILABILE Snk %d Src %d",
                       __FUNCTION__,
                       p_pacs->available.snk_contexts,
                       p_pacs->available.src_contexts);
    }
    else if (index == GA_LIB_PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS)
    {
        STREAM_TO_UINT16(p_pacs->supported.snk_contexts, p_data);
        STREAM_TO_UINT16(p_pacs->supported.src_contexts, p_data);
        WICED_BT_TRACE("[%s] Supported Snk %d Src %d",
                       __FUNCTION__,
                       p_pacs->supported.snk_contexts,
                       p_pacs->supported.src_contexts);
    }
    else if (index == GA_LIB_PACS_CHARACTERISTIC_SNK_AUDIO_LOCATIONS)
    {
        STREAM_TO_UINT32(p_pacs->snk_audio_location, p_data);
        WICED_BT_TRACE("[%s] snk location 0x%x", __FUNCTION__, p_pacs->snk_audio_location);
    }
    else if (index == GA_LIB_PACS_CHARACTERISTIC_SRC_AUDIO_LOCATIONS)
    {
        STREAM_TO_UINT32(p_pacs->src_audio_location, p_data);
        WICED_BT_TRACE("[%s] src location 0x%x", __FUNCTION__, p_pacs->src_audio_location);
    }
    else if (index < (GA_LIB_PACS_CHARACTERISTIC_MAX_UNIQUE + LEPL_PACS_MAX_SNK_CHARACTERISTICS))
    {
        WICED_BT_TRACE("[%s] SINK PACS num_of_records %d", __FUNCTION__, *p_data);
        lepl_rpc_send_pacs_records(conn_id, p_data, val_len, TRUE);
    }
    else if (index < (GA_LIB_PACS_CHARACTERISTIC_MAX_UNIQUE + LEPL_PACS_MAX_SNK_CHARACTERISTICS +
                      LEPL_PACS_MAX_SRC_CHARACTERISTICS))
    {
        WICED_BT_TRACE("[%s] SRC PACS num_of_records %d", __FUNCTION__, *p_data);
        lepl_rpc_send_pacs_records(conn_id, p_data, val_len, FALSE);
    }
}

wiced_bool_t lepl_ccs_pacs_does_peer_support_ringtone(uint16_t conn_id)
{
    lepl_clcb_t *p_clcb = (lepl_clcb_t *)lepl_gatt_get_clcb_by_conn_id(conn_id);
    ga_lib_pacs_data_t *p_pacs = &p_clcb->pacs;

    if (p_pacs->available.snk_contexts & BAP_CONTEXT_TYPE_RINGTONE)
    {
        WICED_BT_TRACE("[%s] peer supports inband", __FUNCTION__);
        return WICED_TRUE;
    }
    WICED_BT_TRACE("[%s] peer doesnot supports inband", __FUNCTION__);
    return WICED_FALSE;
}

wiced_bool_t lepl_pacs_verify_context_type(uint16_t conn_id, uint8_t ase_type, uint16_t req_context)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    ga_lib_pacs_data_t *p_pacs = &p_clcb->pacs;
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No clcb conn id %d", __FUNCTION__, conn_id);
        return WICED_FALSE;
    }
    ga_lib_bap_context_type_t avl_ctx = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
                                                 ? p_pacs->available.snk_contexts
                                                 : p_pacs->available.src_contexts;
    ga_lib_bap_context_type_t supported_ctx = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
                                                       ? p_pacs->supported.snk_contexts
                                                       : p_pacs->supported.src_contexts;
    WICED_BT_TRACE("[%s] supp contexts %d avlbl contexts %d req context %d\n",
                   __FUNCTION__,
                   avl_ctx,
                   supported_ctx,
                   req_context);

    if (!(avl_ctx & req_context))
    {
        return WICED_FALSE;
    }
    if (!(supported_ctx & req_context))
    {
        // Not Available Error
        return WICED_FALSE;
    }
    return WICED_TRUE;
}

wiced_bool_t lepl_pacs_verify_audio_location(uint16_t conn_id, uint8_t ase_type, ga_lib_pacs_audio_location_t req)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    ga_lib_pacs_data_t *p_pacs = &p_clcb->pacs;
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No clcb conn id %d", __FUNCTION__, conn_id);
        return WICED_FALSE;
    }
    ga_lib_pacs_audio_location_t supported_location = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
                                                               ? p_pacs->snk_audio_location
                                                               : p_pacs->src_audio_location;

    if (req != BAP_AUDIO_LOCATION_NOT_ALLOWED)
    {
        if (!(req & supported_location))
            return WICED_FALSE;
    }
    return WICED_TRUE;
}

static void fill_lc3_codec_param(uint8_t *p_data, uint8_t len, le_audio_utils_pacs_t *p_output)
{
    uint8_t type = 0;
    while (len > 2)
    {
        uint8_t type_len = 0;
        STREAM_TO_UINT8(type_len, p_data);
        STREAM_TO_UINT8(type, p_data);
        len -= 2;
        switch (type)
        {
        case BAP_CODEC_CAPABILITIES_SUPPORTED_SAMPLING_FREQUENCIES_TYPE:
            STREAM_TO_UINT16(p_output->sf, p_data);
            len -= 2;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_FRAME_DURATIONS_TYPE:
            STREAM_TO_UINT8(p_output->frame_duration, p_data);
            len -= 1;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_AUDIO_CHANNEL_COUNTS_TYPE:
            STREAM_TO_UINT8(p_output->audio_ch_count, p_data);
            len -= 1;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE:
            STREAM_TO_UINT16(p_output->min_data_per_frame, p_data);
            STREAM_TO_UINT16(p_output->max_data_per_frame, p_data);
            len -= 4;
            break;
        case BAP_CODEC_CAPABILITIES_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_TYPE:
            STREAM_TO_UINT8(p_output->frame_per_sdu, p_data);
            len--;
            break;
        default:
            break;
        }
        UNUSED_VARIABLE(type_len);
    }
    WICED_BT_TRACE("[%s] sf %d frame_duration %d ch_count %d octet_min %d octet_max %d frame_per_sdu %d",
                   __FUNCTION__,
                   p_output->sf,
                   p_output->frame_duration,
                   p_output->audio_ch_count,
                   p_output->min_data_per_frame,
                   p_output->max_data_per_frame,
                   p_output->frame_per_sdu);
}

wiced_bool_t lepl_pacs_verify_codec(uint16_t conn_id,
                                    uint8_t ase_type,
                                    ga_lib_ascs_config_codec_args_t *p_codec_config)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    ga_lib_pacs_data_t *p_pacs = &p_clcb->pacs;
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("[%s] No clcb conn id %d", __FUNCTION__, conn_id);
        return WICED_FALSE;
    }
    ga_lib_pacs_char_data_t *p_pac_list =
        (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? &p_pacs->snk_pac_list : &p_pacs->src_pac_list;
    le_audio_utils_pacs_t record_param = {0};
    int index = 0;

    for (index = 0; index < p_pac_list->num_records; index++)
    {
        ga_lib_pacs_record_t *record = &p_pac_list->p_records[index];
        if (record->codec_id.coding_format != p_codec_config->codec_id.coding_format)
        {
            WICED_BT_TRACE("[%s] req_id %d peer_id %d  \n",
                           __FUNCTION__,
                           record->codec_id.coding_format,
                           p_codec_config->codec_id.coding_format);
            WICED_BT_TRACE("Codec id didn't match ");
            return WICED_FALSE;
        }
        else
        {
            // go through the codec capabilities
            memset(&record_param, 0, sizeof(le_audio_utils_pacs_t));
            fill_lc3_codec_param(record->codec_specific_capabilities,
                                 record->codec_specific_capabilities_length,
                                 &record_param);
            if (lepl_ascs_compare_codec_param(&record_param, p_codec_config))
            {
                break;
            }
        }
    }
    if (index >= p_pac_list->num_records)
        return WICED_FALSE;
    return WICED_TRUE;
}
