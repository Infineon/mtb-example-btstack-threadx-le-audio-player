/*
 * $ Copyright Cypress Semiconductor $
 */
#include "lepl.h"

enum
{
    SET_ACTIVE_PRESET = 1,
    SET_NEXT_PRESET,
    SET_PREVIOUS_PRESET,
};

static int lepl_search_preset_index(lepl_clcb_t *p_clcb, uint8_t preset_index)
{
    lepl_has_t *p_has = &p_clcb->has;
    lepl_has_preset_rec_t *p_rec = p_has->preset_rec_list;

    for (int index = 0; index < LEPL_HAS_MAX_PRESET_RECORDS; index++, p_rec++)
    {
        if (preset_index && (p_rec->preset_index == preset_index))
        {
            WICED_BT_TRACE("[%s] preset index %d pos %d", __FUNCTION__, preset_index, index);
            return index;
        }
    }

    WICED_BT_TRACE("[%s] Not available in the list", __FUNCTION__);
    return -1;
}

static int predict_pos_of_preset_rec_in_list(lepl_has_preset_rec_t *p_rec, uint8_t preset_index)
{
    for (int index = 0; index < LEPL_HAS_MAX_PRESET_RECORDS; index++)
    {
        if ((p_rec[index].preset_index >= preset_index) || (p_rec[index].preset_index == 0))
        {
            return index;
        }
    }

    return -1;
}

//Delete one or more preset records from the HAS preset list
static void lepl_hap_delete_preset_rec(lepl_has_preset_rec_t *p_rec, int start, int end)
{
    /* Validate input parameters */
    if (!p_rec || (start < 0) || (end >= LEPL_HAS_MAX_PRESET_RECORDS) || (start > end))
    {
        WICED_BT_TRACE_CRIT("[%s] Invalid parameters", __FUNCTION__);
        return;
    }

    uint8_t rec_to_delete = end - start + 1;
    /* Shift remaining records up to fill the gap (if any exist after the deleted range) */
    if (end < (LEPL_HAS_MAX_PRESET_RECORDS - 1))
    {
        WICED_MEMCPY(&p_rec[start],
                     &p_rec[end + 1],
                     (LEPL_HAS_MAX_PRESET_RECORDS - end - 1) * sizeof(lepl_has_preset_rec_t));
    }

    /* Clear the freed space at the end of the array */
    WICED_MEMSET(&p_rec[LEPL_HAS_MAX_PRESET_RECORDS - rec_to_delete], 0, rec_to_delete * sizeof(lepl_has_preset_rec_t));
}

// Add a preset record to the HAS preset list in sorted order
static void lepl_hap_add_preset_rec_to_list(lepl_clcb_t *p_clcb, int pos, lepl_has_preset_rec_t *p_pset)
{
    lepl_has_t *p_has = &p_clcb->has;
    lepl_has_preset_rec_t *p_rec = p_has->preset_rec_list;

    if (pos < 0 || pos >= LEPL_HAS_MAX_PRESET_RECORDS)
    {
        WICED_BT_TRACE_CRIT("[%s] Invalid position", __FUNCTION__);
        return;
    }
    else if ((p_pset->preset_index == p_rec[pos].preset_index) || (pos == (LEPL_HAS_MAX_PRESET_RECORDS - 1)))
    {
        /* Exact match at position or last position - direct copy */
        WICED_MEMCPY(&p_rec[pos], p_pset, sizeof(lepl_has_preset_rec_t));
        le_audio_rpc_send_preset_record(p_clcb->conn_id, p_pset->preset_index, p_pset->name, p_pset->name_len);
        WICED_BT_TRACE("[%s] Added preset %d at index %d", __FUNCTION__, p_pset->preset_index, pos);
        return;
    }

    int nxt_prst_pos = pos;
    //Prest Index which is higher than current Preset Index
    while ((nxt_prst_pos < LEPL_HAS_MAX_PRESET_RECORDS) && (p_pset->preset_index > p_rec[nxt_prst_pos].preset_index))
    {
        nxt_prst_pos++;
    }

    if (nxt_prst_pos == LEPL_HAS_MAX_PRESET_RECORDS)
    {
        /* Clear everything after pos */
        WICED_MEMSET(&p_rec[pos + 1], 0, (LEPL_HAS_MAX_PRESET_RECORDS - pos - 1) * sizeof(lepl_has_preset_rec_t));
    }
    else if (nxt_prst_pos > pos)
    {
        /*Found conflicting records - delete them */
        lepl_hap_delete_preset_rec(p_rec, pos, nxt_prst_pos - 1);
    }
    else
    {
        /*Need to insert before existing record - shift down */
        WICED_MEMMOVE(&p_rec[pos + 1],
                      &p_rec[pos],
                      (LEPL_HAS_MAX_PRESET_RECORDS - pos - 1) * sizeof(lepl_has_preset_rec_t));
    }
    WICED_MEMCPY(&p_rec[pos], p_pset, sizeof(lepl_has_preset_rec_t));

    WICED_BT_TRACE("[%s] Added preset %d at index %d", __FUNCTION__, p_pset->preset_index, pos);
    le_audio_rpc_send_preset_record(p_clcb->conn_id, p_pset->preset_index, p_pset->name, p_pset->name_len);
}


void lepl_rpc_has_cp_read_preset_records(uint16_t conn_id)
{
    wiced_bt_gatt_status_t status;
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        return;
    }

    gatt_intf_characteristic_handles_t *p_cp_handles =
        &p_clcb->peer_profiles.has[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT];

    status = ga_lib_has_write_control_point_read_presets(conn_id, p_cp_handles, 1, 5);

    WICED_BT_TRACE("[%s] status %d \n", __FUNCTION__, status);
}

void lepl_rpc_has_cp_set_active_preset(uint16_t conn_id, uint8_t opcode, uint8_t preset_index)
{
    wiced_bt_gatt_status_t status;
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("No clcb");
        return;
    }

    gatt_intf_characteristic_handles_t *p_cp_handles =
        &p_clcb->peer_profiles.has[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT];

    switch (opcode)
    {
    case SET_ACTIVE_PRESET:
        status = ga_lib_has_write_control_point_set_active_preset(conn_id, p_cp_handles, preset_index, 0);
        break;
    case SET_NEXT_PRESET:
        status = ga_lib_has_write_control_point_set_next_preset(conn_id, p_cp_handles, 0);
        break;
    case SET_PREVIOUS_PRESET:
        status = ga_lib_has_write_control_point_set_previous_preset(conn_id, p_cp_handles, 0);
        break;
    default:
        status = WICED_BT_GATT_ERROR;
        break;
    }
    WICED_BT_TRACE("[%s] status %d \n", __FUNCTION__, status);
}

void lepl_rpc_has_cp_set_preset_name(uint16_t conn_id, uint8_t preset_index, char * preset_name)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
    {
        WICED_BT_TRACE_CRIT("No clcb");
        return;
    }

    int index = lepl_search_preset_index(p_clcb, preset_index);
    if ((index == -1) || (0 == (p_clcb->has.preset_rec_list[index].properties & 1)))
    {
        WICED_BT_TRACE("[%s] write is not allowed", __FUNCTION__);
        return;
    }

    gatt_intf_characteristic_handles_t *p_cp_handles =
        &p_clcb->peer_profiles.has[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT];
    wiced_bt_gatt_status_t status = ga_lib_has_write_control_point_preset_name(conn_id, p_cp_handles, preset_index, preset_name);

    WICED_BT_TRACE("[%s] status %d \n", __FUNCTION__, status);

}

static void lepl_hap_handle_read_preset_response(lepl_clcb_t *p_clcb, lepl_has_preset_rec_t *p_rec)
{

    int pos = lepl_search_preset_index(p_clcb, p_rec->preset_index);
    if (pos == -1)
    {
        pos = predict_pos_of_preset_rec_in_list(p_clcb->has.preset_rec_list, p_rec->preset_index);
    }

    lepl_hap_add_preset_rec_to_list(p_clcb, pos, p_rec);
}

void lepl_has_handle_received_characteristics_value(uint16_t conn_id,
                                                    lepl_clcb_t *p_clcb,
                                                    wiced_bt_gatt_optype_t op,
                                                    wiced_bt_gatt_status_t status,
                                                    wiced_bt_gatt_data_t *p_gatt_data)
{
    int has_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.has,
                                                                       GA_LIB_HAS_CHARACTERISTIC_MAX,
                                                                       p_gatt_data->handle);
    lepl_has_t *p_has = &p_clcb->has;
    uint8_t *p_data = p_gatt_data->p_data;

    WICED_BT_TRACE("[%s] handle %d index %d ", __FUNCTION__, p_gatt_data->handle, has_index);

    switch (has_index)
    {
    case GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_FEATURES:
        STREAM_TO_UINT8(p_has->hearing_aid_features, p_data);
        break;
    case GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT:
    {
        uint8_t opcode;
        STREAM_TO_UINT8(opcode, p_data);

        switch (opcode)
        {
        case GA_LIB_HAS_OPCODE_READ_PRESET_RESPONSE:
        {
            lepl_has_preset_rec_t rec;
            uint8_t is_last;

            STREAM_TO_UINT8(is_last, p_data);
            STREAM_TO_UINT8(rec.preset_index, p_data);
            STREAM_TO_UINT8(rec.properties, p_data);

            UNUSED_VARIABLE(is_last);
            rec.name_len = p_gatt_data->len - (p_data - p_gatt_data->p_data);
            if (rec.name_len > LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH)
            {
                WICED_BT_TRACE("[%s] preset name len %d exceeds limit, truncating", __FUNCTION__, rec.name_len);
                rec.name_len = LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH;
            }
            STREAM_TO_ARRAY(rec.name, p_data, (rec.name_len));

            lepl_hap_handle_read_preset_response(p_clcb, &rec);
        }
        break;
        case GA_LIB_HAS_OPCODE_PRESET_CHANGED:
        {
            uint8_t change_id, is_last;
            STREAM_TO_UINT8(change_id, p_data);
            STREAM_TO_UINT8(is_last, p_data);

            UNUSED_VARIABLE(is_last);
            switch (change_id)
            {
            case GA_LIB_HAS_GENERIC_UPDATE:
            {
                lepl_has_preset_rec_t rec = {0};
                uint8_t prev_index;
                STREAM_TO_UINT8(prev_index, p_data);
                STREAM_TO_UINT8(rec.preset_index, p_data);
                STREAM_TO_UINT8(rec.properties, p_data);
                rec.name_len = p_gatt_data->len - (p_data - p_gatt_data->p_data);
                if (rec.name_len > LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH)
                {
                    rec.name_len = LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH;
                }
                STREAM_TO_ARRAY(rec.name, p_data, (rec.name_len));
                int pos = predict_pos_of_preset_rec_in_list(p_clcb->has.preset_rec_list, prev_index);
                lepl_hap_add_preset_rec_to_list(p_clcb, pos, &rec);
            }
            break;
            case GA_LIB_HAS_PRESET_RECORD_DELETED:
            {
                uint8_t preset_index;
                STREAM_TO_UINT8(preset_index, p_data);
                int pos = lepl_search_preset_index(p_clcb, preset_index);
                lepl_hap_delete_preset_rec(p_clcb->has.preset_rec_list, pos, pos);
            }
            break;
            case GA_LIB_HAS_PRESET_RECORD_AVAILABLE:
            {
                uint8_t preset_index;
                STREAM_TO_UINT8(preset_index, p_data);

                int index = lepl_search_preset_index(p_clcb, preset_index);
                if (index != -1)
                {
                    p_has->preset_rec_list[index].properties |= 2;
                }
            }
            break;
            case GA_LIB_HAS_PRESET_RECORD_UNAVAILABLE:
            {
                uint8_t preset_index;
                STREAM_TO_UINT8(preset_index, p_data);

                int index = lepl_search_preset_index(p_clcb, preset_index);
                if (index != -1)
                {
                    p_has->preset_rec_list[index].properties &= ~(2);
                }
            }
            break;
            default:
                break;
            }
        }
        break;
        }
    }
    break;
    case GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_ACTIVE_PRESET_INDEX:
        STREAM_TO_UINT8(p_has->active_preset_index, p_data);
        le_audio_rpc_update_active_preset(conn_id, p_has->active_preset_index);
        break;
    }
}
