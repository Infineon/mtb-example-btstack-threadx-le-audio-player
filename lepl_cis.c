/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"
#include "lepl_bis.h"

extern lepl_broadcast_source_cb_t g_broadcast_source_cb;
typedef struct
{
    wiced_ble_isoc_cis_acl_t conn_hdl;
    wiced_timer_t cis_conn_timer;
} lepl_cis_fail_to_estb_t;

lepl_cis_fail_to_estb_t cis_fail_to_estb;

/* Find ASE with matching given CIG_ID, CIS_ID and Characteristic Type - sink or source*/
static lepl_ase_data_t *lepl_find_ase_with_cis_id(uint8_t cig_id,
                                                  uint8_t cis_id,
                                                  uint8_t char_type)
{
    lepl_clcb_t *p_clcb = g_lepl_gatt_cb.clcb;
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++, p_clcb++)
    {
        if (p_clcb->in_use == 0)
            continue;

        lepl_ase_data_t *p_lepl_ase = p_clcb->ases;
        for (int j = 0; j < LEPL_MAX_ASES; j++, p_lepl_ase++)
        {
            ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
            if ((p_ase->qos_configured.cis_id == cis_id) && (p_ase->ase_type == char_type))
            {
                return p_lepl_ase;
            }
        }
    }
    return NULL;
}

void lepl_create_cis(wiced_ble_isoc_set_cig_cmd_status_evt_t *p_cig_cmd_sts)
{
    for (int i = 0; i < p_cig_cmd_sts->cis_count; i++)
    {
        lepl_audio_mode_t final_state = lepl_cap_get_final_audio_mode();
        lepl_ase_data_t *p_lepl_ase = lepl_find_ase_with_cis_id(
            p_cig_cmd_sts->cig_id,
            1 + i,
            (final_state == LEPL_AUDIO_MODE_MIC) ? GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE : GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE);
        if (!p_lepl_ase || p_lepl_ase->ase.ase_state < GA_LIB_ASCS_STATE_QOS_CONFIGURED)
        {
            WICED_BT_TRACE("[%s] ASE not found or QoS not configured %d ", __FUNCTION__, (p_lepl_ase) ? p_lepl_ase->ase.ase_state : -1);
            continue;
        }
        wiced_ble_isoc_cis_acl_t conn_hdl = {0};
        conn_hdl.cis_conn_handle = p_cig_cmd_sts->cis_connection_handle_list[i];
        conn_hdl.acl_conn_handle = p_lepl_ase->acl_conn_handle;
        wiced_result_t res = wiced_ble_isoc_central_create_cis(1, &conn_hdl);
        WICED_BT_TRACE("[%s] create cis acl 0x%x cis 0x%x res %d",
                       __FUNCTION__,
                       conn_hdl.acl_conn_handle,
                       conn_hdl.cis_conn_handle,
                       res);
    }
}

/* Handle CIS connection event for sink or source */
static void lepl_cis_handle_connection(wiced_ble_isoc_cis_t *p_cis, uint8_t char_type)
{
    wiced_result_t res = WICED_ERROR;
    lepl_ase_data_t *p_lepl_ase = lepl_find_ase_with_cis_id(p_cis->cig_id, p_cis->cis_id, char_type);
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    CHECK_FOR_NULL_AND_RETURN(p_lepl_ase);

    WICED_BT_TRACE("[%s] ase_id %d, state : %d characteristic_type %d",
                   __FUNCTION__,
                   p_ase->ase_id,
                   p_ase->ase_state,
                   p_ase->ase_type);

    //assign CIS to ASE
    p_lepl_ase->cis_conn_handle = p_cis->cis_conn_handle;
    if (p_ase->ase_state < GA_LIB_ASCS_STATE_ENABLING)
    {
        return;
    }
    else if (char_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE && p_ase->ase_state < GA_LIB_ASCS_STATE_STREAMING)
    {
        return;
    }
    else
    {
        res = lepl_isoc_dhm_setup_cis_datapath(p_lepl_ase);

        if (res)
        {
            WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful...(err:%d)\n", __FUNCTION__, res);
        }
    }
    return;
}

/* Handle cis disconnection */
static void lepl_cis_handle_disconnection(wiced_ble_isoc_cis_t *p_cis)
{
    lepl_ase_data_t *p_sink_ase = NULL;
    lepl_ase_data_t *p_source_ase = NULL;
    uint8_t active_data_path = 0;

    // in case of bi-directional CIS handle for both the ASE's attached to the CIS
    p_sink_ase = lepl_find_ase_with_cis_id(p_cis->cig_id, p_cis->cis_id, GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE);
    p_source_ase = lepl_find_ase_with_cis_id(p_cis->cig_id, p_cis->cis_id, GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE);
    if (p_sink_ase)
    {
        if (p_sink_ase->data_path_established)
        {
            lepl_isoc_dhm_remove_cis_datapath(p_sink_ase);
            active_data_path = 1;
        }
        else
        {
            p_sink_ase->cis_conn_handle = 0;
        }
    }

    if (p_source_ase)
    {
        if (p_source_ase->data_path_established)
        {
            lepl_isoc_dhm_remove_cis_datapath(p_source_ase);
            active_data_path = 1;
        }
        else
        {
            p_source_ase->cis_conn_handle = 0;
        }
    }

    if (!active_data_path)
    {
        wiced_result_t res = wiced_ble_isoc_central_remove_cig(p_cis->cig_id);
        if (res != WICED_BT_ILLEGAL_ACTION)
        {
            lepl_isoc_dhm_stop_stream(p_cis->cis_conn_handle);
            lepl_isoc_dhm_disable_audio();
            lepl_gatt_handle_disconnecting_state();
        }
    }
}

/* Get ASE app data pointer by CIS connection handle and characteristic type - sink or source */
static lepl_ase_data_t *lepl_get_ase_app_data_ptr_by_cis_conn_hdl(uint16_t cis_conn_hdl, uint8_t char_type)
{
    lepl_clcb_t *p_clcb = g_lepl_gatt_cb.clcb;
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++, p_clcb++)
    {
        if (p_clcb->in_use == 0)
            continue;

        lepl_ase_data_t *p_lepl_ase = p_clcb->ases;
        for (int j = 0; j < LEPL_MAX_ASES; j++, p_lepl_ase++)
        {
            if ((p_lepl_ase->cis_conn_handle == cis_conn_hdl) && (p_lepl_ase->ase.ase_type == char_type))
            {
                return p_lepl_ase;
            }
        }
    }

    return NULL;
}

static void lepl_cis_handle_data_path_setup(uint16_t cis_conn_hdl,
                                            wiced_bool_t is_cl,
                                            lepl_ase_data_t * p_lepl_ase)
{
    uint8_t ase_type;
    wiced_ble_isoc_data_path_direction_t dir;
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;

    dir = p_lepl_ase->ase.data_path_dir;
    ase_type = p_lepl_ase->ase.ase_type;
    WICED_BT_TRACE("[%s] dir %s type %d\n",
                   __FUNCTION__,
                   (dir == WICED_BLE_ISOC_DPD_INPUT) ? "ASCS_SOURCE" : "ASCS_SINK",
                   ase_type);

    // Update ASE data to indicate data path is setup successfully
    p_lepl_ase->data_path_established = 1;

    if (is_cl)
    {
        // if client + source and in streaming state, start audio streaming
        // if client + sink and in enabling state, send receiver start ready
        if (GA_LIB_ASCS_STATE_STREAMING == p_ase->ase_state)
        {
            lepl_isoc_dhm_start_cis_stream(cis_conn_hdl, dir);
        }
        else if (GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE == ase_type &&
                 GA_LIB_ASCS_STATE_ENABLING == p_ase->ase_state)
        {
            lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_handle(p_lepl_ase->acl_conn_handle);

            lepl_ascs_write_control_point_receiver_start_ready(p_clcb, p_lepl_ase);
        }
        else
        {
            WICED_BT_TRACE_CRIT("[%s] cl unexpected state %d char_type %d\n",
                                __FUNCTION__,
                                p_ase->ase_state,
                                ase_type);
        }
    }
}

static void lepl_cis_handle_data_path_remove(wiced_ble_isoc_setup_data_path_evt_t datapath)
{
    uint8_t active_data_path = 0;
    lepl_ase_data_t *p_lepl_ase = (lepl_ase_data_t *)datapath.p_app_ctx;

    if (!p_lepl_ase || datapath.status)
    {
        return;
    }

    p_lepl_ase->data_path_established = 0;
    if (p_lepl_ase->ase.ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
    {
        lepl_ase_data_t *p_source_ase =
            lepl_get_ase_app_data_ptr_by_cis_conn_hdl(datapath.conn_hdl, GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE);
        if (p_source_ase && p_source_ase->data_path_established)
            active_data_path = 1;
    }
    else
    {
        lepl_ase_data_t *p_sink_ase =
            lepl_get_ase_app_data_ptr_by_cis_conn_hdl(datapath.conn_hdl, GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE);
        if (p_sink_ase && p_sink_ase->data_path_established)
            active_data_path = 1;
    }

    if (wiced_ble_isoc_is_cis_connected_with_conn_hdl(datapath.conn_hdl))
    {
        WICED_BT_TRACE("[%s] disconnecting CIS..\n", __FUNCTION__);
        wiced_ble_isoc_disconnect_cis(datapath.conn_hdl);
        return;
    }

    p_lepl_ase->cis_conn_handle = 0;
    if (!active_data_path)
    {
        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        if ((p_ase->ase_state > GA_LIB_ASCS_STATE_CODEC_CONFIGURED) && (p_ase->ase_state < GA_LIB_ASCS_STATE_MAX))
        {

            wiced_result_t res = wiced_ble_isoc_central_remove_cig(p_ase->qos_configured.cig_id);
            if (res != WICED_BT_ILLEGAL_ACTION)
            {
                lepl_isoc_dhm_stop_stream(datapath.conn_hdl);
                lepl_isoc_dhm_disable_audio();
            }
        }
    }
}

void lepl_handle_cis_failed_to_established(WICED_TIMER_PARAM_TYPE p_inst)
{
    wiced_result_t res = wiced_ble_isoc_central_create_cis(1, &cis_fail_to_estb.conn_hdl);
    WICED_BT_TRACE("[%s] create cis res %d", __FUNCTION__, res);
}

/* ISOC Event Handler */
void lepl_isoc_event_handler(wiced_ble_isoc_event_t event, wiced_ble_isoc_event_data_t *p_event_data)
{
    static wiced_bool_t is_client = TRUE;
    wiced_ble_isoc_set_cig_cmd_status_evt_t *p_cig_status_data = NULL;
    wiced_ble_isoc_create_big_cmpl_evt_t *p_create_big_sts = NULL;
    lepl_broadcast_source_cb_t *p_big = &g_broadcast_source_cb;
    wiced_result_t res = WICED_ERROR;

    WICED_BT_TRACE("[%s] event %d ", __FUNCTION__, event);

    switch (event)
    {
    case WICED_BLE_ISOC_SET_CIG_CMD_COMPLETE_EVT:

        p_cig_status_data = &p_event_data->cig_status_data;

        if (WICED_BT_SUCCESS != p_cig_status_data->status) return;

        WICED_BT_TRACE("status %d cig_id %d cis_count %d CIS Handle %d\n",
                       p_cig_status_data->status,
                       p_cig_status_data->cig_id,
                       p_cig_status_data->cis_count,
                       p_cig_status_data->cis_connection_handle_list[0]);
        lepl_create_cis(p_cig_status_data);

        break;

    case WICED_BLE_ISOC_CIS_ESTABLISHED_EVT:
    {
        wiced_ble_isoc_cis_t *p_cis = &p_event_data->cis_established_data.cis;
        if (p_event_data->cis_established_data.status == 0x3E) // Failed to Synchronize within sync timeout
        {
            cis_fail_to_estb.conn_hdl.cis_conn_handle = p_cis->cis_conn_handle;
            cis_fail_to_estb.conn_hdl.acl_conn_handle = p_cis->acl_conn_handle;

            if (cis_fail_to_estb.cis_conn_timer.p_cback == NULL)
            {
                wiced_init_timer(&cis_fail_to_estb.cis_conn_timer,
                                 lepl_handle_cis_failed_to_established,
                                 NULL,
                                 WICED_MILLI_SECONDS_TIMER);
            }
            wiced_start_timer(&cis_fail_to_estb.cis_conn_timer, 1000);
            return;
        }

        // Setup data path after CIS establishment for sink role as server/client,
        // Data path is setup upon receiving streaming notification / receiver start ready
        // as client and server
        if (is_client)
        {
            lepl_cis_handle_connection(p_cis, GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE);
            lepl_cis_handle_connection(p_cis, GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE);
        }
    }
        break;

    case WICED_BLE_ISOC_CIS_DISCONNECTED_EVT:
        lepl_cis_handle_disconnection(&p_event_data->cis_disconnect.cis);
        break;

    case WICED_BLE_ISOC_DATA_PATH_SETUP_EVT:
        if (p_event_data->datapath.status)
        {
            WICED_BT_TRACE_CRIT("[%s] Data path setup not successful\n", __FUNCTION__);
            return;
        }

        if (wiced_ble_isoc_is_cis_connected_with_conn_hdl(p_event_data->datapath.conn_hdl))
        {
            lepl_ase_data_t *p_ase = (lepl_ase_data_t *)p_event_data->datapath.p_app_ctx;
		    WICED_BT_TRACE("[%s] p_ase 0x%x",__FUNCTION__, p_event_data->datapath.p_app_ctx);
            if (p_ase)
            {
                lepl_cis_handle_data_path_setup(p_event_data->datapath.conn_hdl, is_client, p_ase);
            }
        }
        else if (wiced_ble_isoc_is_bis_created(p_event_data->datapath.conn_hdl))
        {
             p_big->base.state = BAP_BROADCAST_STATE_STREAMING;
             WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);

            // start streaming as broadcast source
            lepl_isoc_dhm_start_bis_stream(p_event_data->datapath.conn_hdl);
        }
        break;

    case WICED_BLE_ISOC_DATA_PATH_REMOVED_EVT:
        if (p_event_data->datapath.status)
        {
            WICED_BT_TRACE_CRIT("[%s] Data path removal not successful\n", __FUNCTION__);
        }
        if (wiced_ble_isoc_is_bis_created(p_event_data->datapath.conn_hdl))
        {
            lepl_isoc_dhm_stop_stream(p_event_data->datapath.conn_hdl);
        }
        else
        {
            if (is_client)
            {
                lepl_cis_handle_data_path_remove(p_event_data->datapath);
            }
        }
        break;

    case WICED_BLE_ISOC_BIG_CREATED_EVT:
    {
        p_create_big_sts = &p_event_data->create_big;

         if (p_create_big_sts->sync_data.status)
         {
             WICED_BT_TRACE_CRIT("[%s] BIG Creation unsuccessful\n", __FUNCTION__);
             return;
         }

         /* Map BIS index and bis_conn_handle */
         p_big->bis_conn_id_count = p_create_big_sts->sync_data.num_bis;
         memcpy(p_big->bis_conn_id_list,
                p_create_big_sts->sync_data.bis_conn_hdl_list,
                p_big->bis_conn_id_count * sizeof(uint16_t));

         /* start setting up data paths for all the BIS streams */
         for (int i = 0; i < p_big->bis_conn_id_count; i++)
         {
             ga_lib_ascs_csc_t *p_csc = &p_big->base.sub_group[0].csc;
             if (p_big->base.sub_group[0].bis_config[i].bis_csc.audio_channel_allocation)
                 p_csc->audio_channel_allocation =
                     p_big->base.sub_group[0].bis_config[i].bis_csc.audio_channel_allocation;

             res = lepl_isoc_dhm_setup_bis_datapath(p_big->bis_conn_id_list[i], p_csc);
             if (res)
             {
                 WICED_BT_TRACE_CRIT("[%s] data path setup unsuccessful...(err:%d)\n", __FUNCTION__, res);
             }

         }
        }
        break;

    case WICED_BLE_ISOC_BIG_TERMINATED_EVT:
    {
         // p_big->base.state = BAP_BROADCAST_STATE_CONFIGURED;
         // FIXME: Disabling state check to allow this fn. call immediately after
         // disabling stream. Should have a mechanism to queue release req to handle
         // from data path removed
        lepl_isoc_dhm_disable_audio();
         WICED_BT_TRACE("[%s] BASE State [%d] \n", __FUNCTION__, p_big->base.state);
    }break;

    default:
        break;
    }
}

/* Initialize ISOC module */
void lepl_isoc_init(void)
{
    wiced_ble_isoc_cfg_t cfg = {.max_cis = 2, .max_bis = 2};

    wiced_ble_isoc_init(&cfg, lepl_isoc_event_handler);

    /* Initialize audio interfaces and register callbacks for data handling */
    lepl_isoc_dhm_init();
}
