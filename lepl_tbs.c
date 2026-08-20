
/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"

char default_bearer_provider_name[] = "CyIFX_provider";
char default_bearer_uci[] = "uci";
char default_bearer_uri[] = "skype,phone";
char default_friendly_name[] = "tel:+19991110011";
char default_tg_caller_id[] = "aaa:77777";

void lepl_tbs_set_call_control_server_state(lepl_ccs_states_t state)
{
    g_lepl_gatt_cb.local_service_data.gtbs.ccs_state = state;
}

lepl_ccs_states_t lepl_get_call_control_server_state()
{
    return g_lepl_gatt_cb.local_service_data.gtbs.ccs_state;
}

ga_lib_tbs_call_operation_result_t place_call(lepl_tbs_call_state_data_t *p_call, char *call_name, uint8_t *call_id)
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    uint8_t len;
    if (p_tbs->current_call_id == 255)
        p_tbs->current_call_id = 1;
    else
        p_tbs->current_call_id++;

    *call_id = p_tbs->current_call_id;
    p_tbs->num_calls++;

    //fill call friendly name
    len = (uint8_t)strlen(default_friendly_name);
    memcpy(p_call->friendly_name, default_friendly_name, len);

    //place the call and return the status back
    return GA_LIB_CALL_SUCCESS;
}

wiced_bool_t check_URI_validity(char * p_uri)
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    if (p_tbs->is_caller_id_invalid_enabled)
        return WICED_FALSE;
    else
        return WICED_TRUE;
}

ga_lib_tbs_call_operation_result_t change_call_state(lepl_tbs_call_state_data_t *p_call_state,
                                                          ga_lib_tbs_call_state_t call_state)
{
    ga_lib_tbs_call_operation_result_t res;
    if (!p_call_state)
    {
        WICED_BT_TRACE_CRIT("[%s] p_call_state",__FUNCTION__);
        return GA_LIB_CALL_INVALID_CALL_ID;
    }
    switch (call_state)
    {
    case GA_LIB_TBS_CALL_STATE_ACTIVE:
    case GA_LIB_TBS_CALL_STATE_LOCALLY_HELD:
    case GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
    case GA_LIB_TBS_CALL_STATE_REMOTELY_HELD:
    case GA_LIB_TBS_CALL_STATE_ALERTING:
        p_call_state->call.call_state = call_state;
        le_audio_rpc_update_call_state(0, p_call_state->call.call_id, NULL, call_state); // conn id 0 as LEPL is TBS server
        res = GA_LIB_CALL_SUCCESS;
        break;

    default:
        res = GA_LIB_CALL_OPCODE_NOT_SUPPORTED;
        break;
    }
    return res;
}

void lepl_notify_call_state_to_all(void)
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call = p_tbs->call_state_data;

    for (int call_num = 0; call_num < LEPL_TBS_BEARER_MAX_CALL_COUNT; call_num++)
    {
        WICED_BT_TRACE("[%s] call_id %d call_state %d\n",
                       __FUNCTION__,
                       p_tbs->call_state_data[call_num].call.call_id,
                       p_tbs->call_state_data[call_num].call.call_state);

        for (int j = 0; j < LEPL_MAX_CONNECTIONS; j++)
        {
            lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[j];
            if (p_clcb->in_use)
            {
                ga_lib_tbs_notify_call_state(p_clcb->conn_id,
                                             HDLC_GTBS_CALL_STATE_VALUE,
                                             &p_call->call,
                                             (p_call->in_use) ? 1 : 0);
            }
        }
    }
}

void lepl_notify_call_list_current_calls(void)
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call = p_tbs->call_state_data;

    for (int num_call = 0; num_call < LEPL_TBS_BEARER_MAX_CALL_COUNT; num_call++)
    {
        WICED_BT_TRACE("[%s] call_id %d call_state %d\n",
                       __FUNCTION__,
                       p_tbs->call_state_data[num_call].call.call_id,
                       p_tbs->call_state_data[num_call].call.call_state);

        for (int j = 0; j < LEPL_MAX_CONNECTIONS; j++)
        {
            lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[j];
            if (p_clcb->in_use)
            {
                ga_lib_tbs_notify_current_calls(p_clcb->conn_id,
                                                HDLC_GTBS_BEARER_LIST_CURRENT_CALL_VALUE,
                                                &p_call->call,
                                                (p_call->in_use) ? 1 : 0);
            }
        }
    }
}

lepl_tbs_call_state_data_t *get_call_state(uint8_t call_id, lepl_tbs_call_state_data_t *p_call, uint8_t num_calls)
{
    uint8_t i;
    WICED_BT_TRACE("[%s] ", __FUNCTION__);

    for (i = 0; i < num_calls; i++, p_call++)
    {
        if (p_call->in_use && p_call->call.call_id == call_id)
        {
            return p_call;
        }
    }
    return NULL;
}

void lepl_tbs_stop_inband_ringtone(void)
{
    for (int i = 0; i < MAX_CIS_CONN * 2; i++)
    {
        lepl_ase_data_t *p_ase = g_lepl_gatt_cb.audio_state.p_lepl_ase_list[i];
        if (p_ase && (p_ase->ase.ase_state == GA_LIB_ASCS_STATE_STREAMING))
        {
            lepl_ccs_isoc_handle_ringtone_to_convo(p_ase);
        }
    }
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_accept_call(uint16_t conn_id, uint8_t call_id)
{
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_INVALID_CALL_ID;
    lepl_tbs_call_state_data_t *p_call_state;
    lepl_tbs_call_state_data_t *p_call_state_accept;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;

    WICED_BT_TRACE("[%s] ", __FUNCTION__);

    p_call_state_accept = get_call_state(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

    if (p_call_state_accept == NULL)
    {
        return result;
    }

    if (p_call_state_accept->call.call_state != GA_LIB_TBS_CALL_STATE_INCOMING &&
        p_call_state_accept->call.call_state != GA_LIB_TBS_CALL_STATE_DIALING)
    {
        return GA_LIB_CALL_STATE_MISMATCH;
    }

    for (int i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
    {
        p_call_state = (p_tbs->call_state_data + i);
        if (p_call_state->in_use == 1 && p_call_state->call.call_id != call_id)
        {
            //for all the other caller ID having call state active must be moved to locally held
            if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
            {
                //call the application's hold call function
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
            }
            //for all the other caller ID having call state active must be moved to locally held
            else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_REMOTELY_HELD)
            {
                //call the application's hold call function
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        }
    }

    //call the application's accept call function which will actually accept the call
    result = change_call_state(p_call_state_accept, GA_LIB_TBS_CALL_STATE_ACTIVE);
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (p_clcb && p_clcb->peer_profiles.service_handles[LEPL_ASCS].start_handle)
    {
        if (IS_INBAND_RINGTONE_SUPPORTED(p_tbs->bearer_status_flag) &&
            lepl_get_call_control_server_state() == CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE)
        {
            lepl_tbs_set_call_control_server_state(CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE_CONVO);
            lepl_tbs_stop_inband_ringtone(); //Stop sending ringtone Media
        }
        else
        {
            lepl_ccs_start_streaming_convo(conn_id, &g_lepl_gatt_cb.audio_state.current_strm_config);
        }
    }
    return result;
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_hold_call(uint8_t call_id)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);

    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_INVALID_CALL_ID;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call_state;

    p_call_state = get_call_state(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

    if (p_call_state == NULL)
    {
        return result;
    }

    if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_INCOMING ||
        p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
    {
        //call the application's hold call function
        result = change_call_state(p_call_state,
                                   GA_LIB_TBS_CALL_STATE_LOCALLY_HELD); // incoming / active ----> locally held
    }
    else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_REMOTELY_HELD)
    {
        //call the application's hold call function
        result = change_call_state(
            p_call_state,
            GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD); // remotely----> locally and remotely held
    }
    else
    {
        return GA_LIB_CALL_STATE_MISMATCH;
    }
    return result;
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_retrieve_call(uint8_t call_id)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);
    uint8_t i;
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_INVALID_CALL_ID;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call_state;

    p_call_state = get_call_state(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

    if (p_call_state == NULL)
    {
        return result;
    }

    if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_LOCALLY_HELD)
    {
        result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_ACTIVE);

        //move all other calls to inactive state
        for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
        {
            p_call_state = (p_tbs->call_state_data + i);
            if (!p_call_state->in_use || p_call_state->call.call_id == call_id)
            {
                continue;
            }

            if(p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
            }
            else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_REMOTELY_HELD)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        }
    }
    else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_REMOTELY_HELD);

        //move all other calls to inactive state
        for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
        {
            p_call_state = (p_tbs->call_state_data + i);

            if (!p_call_state->in_use || p_call_state->call.call_id == call_id)
            {
                continue;
            }

            if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
            }
            else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_REMOTELY_HELD)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        }
    }
    return result;
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_retrieve_remotely_hold_call(uint8_t call_id,
                                                                             lepl_tbs_call_state_data_t *p_call_list,
                                                                             uint8_t num_calls)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);
    uint8_t i;
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_INVALID_CALL_ID;
    lepl_tbs_call_state_data_t *p_call_state;

    p_call_state = get_call_state(call_id, p_call_list, num_calls);
    if (p_call_state == NULL)
    {
        return result;
    }


    if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_REMOTELY_HELD)
    {
        result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_ACTIVE);

        //move all other calls to inactive state
        for (i = 0; i < num_calls; i++)
        {
            p_call_state = (p_call_list + i);

            if (p_call_state->in_use == 1 && p_call_state->call.call_id != call_id &&
                p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
            }
        }
    }
    else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
    }
    return result;
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_place_call(char * p_uri, uint8_t uri_len)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);
    uint8_t i;
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_LACK_OF_RESOURCES;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call_state = p_tbs->call_state_data;

    if (!check_URI_validity(p_uri))
    {
        return GA_LIB_CALL_INVALID_URI;
    }


    // for the caller ID in the parameter , we have to set the state to alerting
    for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++, p_call_state++)
    {
        if (p_call_state->in_use == 0)
        {
            result = place_call(p_call_state, p_uri, &p_call_state->call.call_id);

            if (result == GA_LIB_CALL_SUCCESS)
            {
                uint8_t len = (uri_len < LEPL_TBS_RM_CALLERID_MAX_SIZE)
                                  ? uri_len
                                  : LEPL_TBS_RM_CALLERID_MAX_SIZE - 1;
                p_call_state->call.call_flags = GA_LIB_TBS_OUTGOING_CALL;
                memcpy(p_call_state->uri, p_uri, uri_len);
                p_call_state->uri[len] = '\0';
                p_call_state->call.call_state = GA_LIB_TBS_CALL_STATE_DIALING;
                p_call_state->in_use = 1;
                //place the call using remote caller id, remote caller id has to be populated before calling place call
                WICED_BT_TRACE("[%s] outgoing place call uri %s", __FUNCTION__, p_call_state->uri);
                result = GA_LIB_CALL_SUCCESS;
                break;
            }
            else
            {
                return result;
            }
        }
    }

    //move all active calls to locally held state
    for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
    {
        p_call_state = (p_tbs->call_state_data + i);

        if (p_call_state->in_use == WICED_FALSE)
        {
            if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
            {
                p_call_state->call.call_state = GA_LIB_TBS_CALL_STATE_LOCALLY_HELD;
            }
        }
    }
    return result;
}

static ga_lib_tbs_call_operation_result_t lepl_tbs_join_call(uint8_t num_call_ids, uint8_t *p_call_ids)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_SUCCESS;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    lepl_tbs_call_state_data_t *p_call_state;
    uint8_t i = 0, j = 0;

    if (num_call_ids < 2) return GA_LIB_CALL_OPERATION_NOT_POSSIBLE;

    //check if all calls are valid
    for (i = 0; i < num_call_ids; i++)
    {
        p_call_state =
            get_call_state(p_call_ids[i], p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);
        if (p_call_state == NULL) return GA_LIB_CALL_INVALID_CALL_ID;
    }

    //check if any of the call state is incoming, if so return op not possible
    for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
    {
        p_call_state =
            get_call_state(p_call_ids[i], p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);
        if (p_call_state != NULL && p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_INCOMING)
            return GA_LIB_CALL_OPERATION_NOT_POSSIBLE;
    }

    //move all the calls which is in active state but not in the join call list to locally held state
    for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
    {
        p_call_state = (p_tbs->call_state_data + i);
        if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ACTIVE)
        {
            for (j = 0; j < num_call_ids; j++)
            {
                if (p_call_state->call.call_id == p_call_ids[j]) break;
            }

            if (j == num_call_ids)
            {
                result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_LOCALLY_HELD);
            }
        }
    }

    //for all the calls in the join list
    for (i = 0; i < num_call_ids; i++)
    {
        if (p_call_ids[i] == 0) // reserved for the call from the client
        {
            return GA_LIB_CALL_INVALID_CALL_ID;
        }
        p_call_state =
            get_call_state(p_call_ids[i], p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

        if (p_call_state == NULL)
        {
            continue;
        }
        if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_LOCALLY_HELD)
        {
            result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_ACTIVE);
        }
        else if (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
        {
            result = change_call_state(p_call_state, GA_LIB_TBS_CALL_STATE_REMOTELY_HELD);
        }
    }
    return result;
}

/* actions performed by server */
static ga_lib_tbs_call_operation_result_t lepl_tbs_remotely_hold_call(uint8_t call_id,
                                                                    lepl_tbs_call_state_data_t *p_call_list,
                                                                    uint8_t num_calls)
{
    WICED_BT_TRACE("[%s] ", __FUNCTION__);

    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_SUCCESS;
    lepl_tbs_call_state_data_t *p_call_state;

    p_call_state = get_call_state(call_id, p_call_list, num_calls);

    if (p_call_state == NULL)
    {
        return GA_LIB_CALL_INVALID_CALL_ID;
    }

    if ((p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_ALERTING) ||
        (p_call_state->call.call_state == GA_LIB_TBS_CALL_STATE_DIALING))
        return GA_LIB_CALL_STATE_MISMATCH;

    if (p_call_state->call.call_state != GA_LIB_TBS_CALL_STATE_LOCALLY_HELD)
        p_call_state->call.call_state = GA_LIB_TBS_CALL_STATE_REMOTELY_HELD;
    else
        p_call_state->call.call_state = GA_LIB_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;

    le_audio_rpc_update_call_state(0, call_id, NULL, p_call_state->call.call_state);
    return result;
}

void lepl_tbs_set_incoming_remote_call(lepl_tbs_t *p_tbs, char *uri_scheme, char *friendly_name)

{
    uint8_t i;
    uint32_t prefix_len;

    if (p_tbs->num_calls >= LEPL_TBS_BEARER_MAX_CALL_COUNT) return;

    if (p_tbs->current_call_id >= 255)
        p_tbs->current_call_id = 1;
    else
        p_tbs->current_call_id++;

    for (i = 0; i < LEPL_TBS_BEARER_MAX_CALL_COUNT; i++)
    {
        if (p_tbs->call_state_data[i].in_use == 0) break;
    }

    if (i == LEPL_TBS_BEARER_MAX_CALL_COUNT)
    {
        //no resourse for new call
        p_tbs->current_call_id--;
        return;
    }

    p_tbs->num_calls++;

    WICED_BT_TRACE("[%s] call_id value:%d  Call count:%d  i : %d",
                   __FUNCTION__,
                   p_tbs->current_call_id,
                   p_tbs->num_calls,
                   i);

    prefix_len = (uint8_t)strlen(uri_scheme);
    if (prefix_len > LEPL_TBS_RM_CALLERID_MAX_SIZE) prefix_len = LEPL_TBS_RM_CALLERID_MAX_SIZE;

    //------------------------updatecall URI---------------------------------
    memset(p_tbs->bearer_uri, 0, LEPL_TBS_BEARER_URI_MAX_SIZE);
    memcpy(p_tbs->bearer_uri, uri_scheme, prefix_len);

    //------------------------update golbal TBS Data---------------------------------
    p_tbs->call_state_data[i].call.call_id = p_tbs->current_call_id;
    p_tbs->call_state_data[i].call_type = INCOMING_CALL; //1 = Call is an outgoingcall
    p_tbs->call_state_data[i].call.call_state = GA_LIB_TBS_CALL_STATE_INCOMING;
    p_tbs->call_state_data[i].call.call_flags = 1; //inband ringtone is enabled
    p_tbs->call_state_data[i].in_use = 1;

    //--------------------------friendly name -------------------------------
    memset(p_tbs->call_state_data[i].friendly_name, 0, LEPL_TBS_FRIENDLY_NAME_MAX_SIZE);
    strcpy(p_tbs->call_state_data[i].friendly_name, friendly_name);
    p_tbs->call_state_data[i].call.call_id = p_tbs->current_call_id;
    WICED_BT_TRACE("[%s] friendly name %s ", __FUNCTION__, p_tbs->call_state_data[i].friendly_name);

    //--------------------------Target caller ID -------------------------------
    strcpy(p_tbs->incoming_tg_caller_id, default_tg_caller_id);
    WICED_BT_TRACE("[%s] Tg caller ID %s ", __FUNCTION__, p_tbs->incoming_tg_caller_id);
}

wiced_result_t lepl_gtbs_handle_write_cp(uint16_t conn_id, uint8_t *p_data, uint16_t len_to_write)
{
    ga_lib_tbs_call_operation_result_t result = GA_LIB_CALL_OPCODE_NOT_SUPPORTED;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    ga_lib_tbs_call_action_t opcode;

    STREAM_TO_UINT8(opcode, p_data);

    switch (opcode)
    {
    case GA_LIB_CCP_ACTION_ACCEPT_CALL:
    {
        uint8_t call_id;
        STREAM_TO_UINT8(call_id, p_data);
        result = lepl_tbs_accept_call(conn_id, call_id);
    }
    break;
    case GA_LIB_CCP_ACTION_TERMINATE_CALL:
    {
        uint8_t call_id;
        STREAM_TO_UINT8(call_id, p_data);

        result = lepl_tbs_terminate_call(conn_id, call_id, GA_LIB_TBS_CLIENT_TERMINATED);
    }
    break;
    case GA_LIB_CCP_ACTION_HOLD_CALL:
    {
        uint8_t call_id;
        STREAM_TO_UINT8(call_id, p_data);

        if (IS_HOLD_SUPPORTED(p_tbs->ccp_supported_opcode))
        {
            result = lepl_tbs_hold_call(call_id);
        }
    }
    break;
    case GA_LIB_CCP_ACTION_RETRIEVE_CALL:
    {
        uint8_t call_id;
        STREAM_TO_UINT8(call_id, p_data);
        result = lepl_tbs_retrieve_call(call_id);
    }
    break;
    case GA_LIB_CCP_ACTION_ORIGINATE:
    {
        uint8_t call_id;
        char uri[LEPL_TBS_BEARER_URI_MAX_SIZE];
        uint8_t uri_len = len_to_write - 1; // excluding call_id

        if (uri_len > LEPL_TBS_BEARER_URI_MAX_SIZE)
        {
            uri_len = LEPL_TBS_BEARER_URI_MAX_SIZE;
        }

        STREAM_TO_UINT8(call_id, p_data);
        STREAM_TO_ARRAY(uri, p_data, uri_len);

        UNUSED_VARIABLE(call_id);

        result = lepl_tbs_place_call(uri, uri_len);
    }
    break;
    case GA_LIB_CCP_ACTION_JOIN_CALL:
    {
        uint8_t num_call_ids;

        if (IS_JOIN_SUPPORTED(p_tbs->ccp_supported_opcode))
        {
            num_call_ids = len_to_write - 1;
            result = lepl_tbs_join_call(num_call_ids, p_data);
        }
    }
    break;
    default:
        result = GA_LIB_CALL_OPCODE_NOT_SUPPORTED;
        break;
    }
    if (result == GA_LIB_CALL_SUCCESS)
    {
        lepl_notify_call_state_to_all();
    }
    else
    {
        WICED_BT_TRACE_CRIT("[%s] res %d", __FUNCTION__, result);
    }
    return (wiced_result_t)result;
}

wiced_result_t lepl_rpc_ccs_set_incoming_remote_call(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;

    WICED_BT_TRACE("[%s] call_id value:%d  Call count:%d ", __FUNCTION__, p_tbs->current_call_id, p_tbs->num_calls);


    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[i];
        if (p_clcb->in_use)
        {
            // we need friendly name update in the incoming call notification, hence sending this first
            ga_lib_tbs_notify_call_friendly_name(p_clcb->conn_id,
                                                 HDLC_GTBS_CALL_FRIENDLY_NAME_VALUE,
                                                 p_tbs->call_state_data->call.call_id,
                                                 p_tbs->call_state_data->friendly_name);

            //---------------------------------------------------------
            ga_lib_tbs_notify_incoming_call(p_clcb->conn_id,
                                            HDLC_GTBS_INCOMING_CALL_VALUE,
                                            p_tbs->current_call_id,
                                            p_tbs->bearer_uri);
        }
    }

    //---------------------------------------------------------
    lepl_notify_call_state_to_all();
    //---------------------------------------------------------

    lepl_notify_call_list_current_calls();

    //---------------------------------------------------------

    if (IS_INBAND_RINGTONE_SUPPORTED(p_tbs->bearer_status_flag) && lepl_ccs_pacs_does_peer_support_ringtone(conn_id))
    {
        //start_inband_ringtone
        lepl_ccs_start_inband_ringtone(conn_id, p_stream_config);
    }
    else
    {
        lepl_tbs_set_call_control_server_state(CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE);
    }

    le_audio_rpc_update_call_state(conn_id, p_tbs->current_call_id, p_tbs->bearer_uri, GA_LIB_TBS_CALL_STATE_DIALING);
    return WICED_SUCCESS;
}



void lepl_rpc_ccs_set_remote_hold_call(uint8_t call_id)
{
    ga_lib_tbs_call_operation_result_t result;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;

    //update state
    result = lepl_tbs_remotely_hold_call(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

    if (result == GA_LIB_CALL_SUCCESS)
    {
        WICED_BT_TRACE("[%s] result %d call_id %d call_state %d\n",
                       __FUNCTION__,
                       result,
                       call_id,
                       p_tbs->call_state_data->call.call_state);
        lepl_notify_call_state_to_all();
    }
    else
        WICED_BT_TRACE("[%s] result %d Call state not updated for call_id %d\n", __FUNCTION__, result, call_id);
}



void lepl_rpc_ccs_set_retrieve_remote_hold_call(uint8_t call_id)
{
    ga_lib_tbs_call_operation_result_t result;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;

    //update state
    result = lepl_tbs_retrieve_remotely_hold_call(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

    if (result == GA_LIB_CALL_SUCCESS)
    {
        WICED_BT_TRACE("[%s] Remote call rertieve Result %d call_id %d call_state %d\n",
                       __FUNCTION__,
                       result,
                       call_id,
                       p_tbs->call_state_data->call.call_state);

        lepl_notify_call_state_to_all();
    }
    else
        WICED_BT_TRACE("[%s] result %d Call state not updated for call_id %d\n", __FUNCTION__, result, call_id);
}

ga_lib_tbs_call_operation_result_t lepl_tbs_terminate_call(uint16_t conn_id,
                                                           uint8_t call_id,
                                                           uint8_t termination_reason)
{
    lepl_tbs_call_state_data_t *p_call_state;
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;

    p_call_state = get_call_state(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);
    if (!p_call_state)
    {
        return GA_LIB_CALL_INVALID_CALL_ID;
    }

    lepl_ccs_states_t call_state = lepl_get_call_control_server_state();
    WICED_BT_TRACE("[%s] state : %d\n", __FUNCTION__, call_state);

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (p_clcb && p_clcb->peer_profiles.service_handles[LEPL_ASCS].start_handle)
    {
        lepl_cap_stop_streaming(conn_id);
    }

    //clear data
    p_call_state->in_use = 0;
    p_tbs->num_calls--;
    WICED_BT_TRACE("[%s] call_id %d call_state %d\n",
                   __FUNCTION__,
                   p_call_state->call.call_id,
                   p_call_state->call.call_state);

    for (int j = 0; j < LEPL_MAX_CONNECTIONS; j++)
    {
        lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[j];
        if (p_clcb->in_use)
        {
            ga_lib_tbs_notify_call_termination_reason(p_clcb->conn_id,
                                                   HDLC_GTBS_TERMINATION_REASON_VALUE,
                                                   p_call_state->call.call_id,
                                                   termination_reason);
        }
    }

    le_audio_rpc_send_call_terminated_event(conn_id, call_id, termination_reason);

    lepl_notify_call_state_to_all();

    lepl_notify_call_list_current_calls();

    lepl_cap_set_next_audio_mode(LEPL_AUDIO_MODE_NONE, NULL);
    if ((lepl_get_call_control_server_state() == CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE) &&
        (!IS_INBAND_RINGTONE_SUPPORTED(p_tbs->bearer_status_flag) ||
        !lepl_ccs_pacs_does_peer_support_ringtone(conn_id)))
    {
        lepl_audio_state_info_t *p_audio_state = &g_lepl_gatt_cb.audio_state;
        p_audio_state->current_mode = LEPL_AUDIO_MODE_NONE;
        if (p_audio_state->paused_mode)
        {
            lepl_cap_resume_streaming(conn_id, p_audio_state->paused_mode, &p_audio_state->paused_strm_config);
            lepl_cap_set_next_audio_mode(p_audio_state->paused_mode, &p_audio_state->paused_strm_config);
            p_audio_state->paused_mode = LEPL_AUDIO_MODE_NONE;
        }
    }
    lepl_tbs_set_call_control_server_state(CALL_CONTROL_SERVER_STATE_IDLE);
    return GA_LIB_CALL_SUCCESS;
}

void lepl_tbs_initialize_data()
{
    lepl_tbs_t *p_tbs = &g_lepl_gatt_cb.local_service_data.gtbs;
    WICED_BT_TRACE("[%s] \n", __FUNCTION__);

    //set initial default values
    memset(p_tbs, 0, sizeof(lepl_tbs_t));
    p_tbs->content_control_id = 2;
    strcpy(p_tbs->bearer_provider_name, default_bearer_provider_name);
    strcpy(p_tbs->bearer_uci, default_bearer_uci);
    strcpy(p_tbs->bearer_uri, default_bearer_uri);
    p_tbs->bearer_signal_strength = 5;
    p_tbs->bearer_signal_strength_reporting_interval = 0;
    p_tbs->prev_bearer_signal_strength = 0;
    p_tbs->bearer_technology = GA_LIB_TBS_3G_TECHNOLOGY;
    p_tbs->bearer_status_flag =
#ifdef INBAND_RINGTONE_ENABLED
        GA_LIB_TBS_FEATURE_BIT_INBAND_RINGTONE |
#endif // INBAND_RINGTONE_ENABLED
        GA_LIB_TBS_FEATURE_BIT_SILENT_MODE;

    p_tbs->ccp_supported_opcode =
        GA_LIB_TBS_FEATURE_BIT_LOCAL_HOLD | GA_LIB_TBS_FEATURE_BIT_JOIN; //bit 0 and 1 set to 1
    strcpy(p_tbs->incoming_tg_caller_id, default_tg_caller_id);
    memset(p_tbs->call_state_data->friendly_name, 0, sizeof(p_tbs->call_state_data->friendly_name));
    p_tbs->ccs_state = CALL_CONTROL_SERVER_STATE_IDLE;
}

void lepl_ccs_start_streaming_convo(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] conn id %x\n", __FUNCTION__, conn_id);
    p_stream_config->context_type = BAP_CONTEXT_TYPE_CONVERSATIONAL;
    lepl_tbs_set_call_control_server_state(CALL_CONTROL_SERVER_STATE_CONVO);
    lepl_cap_start_streaming(conn_id, p_stream_config);
}

void lepl_ccs_start_inband_ringtone(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("call_control_server_start_inband_ringtone %x\n", conn_id);
    p_stream_config->context_type = BAP_CONTEXT_TYPE_RINGTONE;
    lepl_tbs_set_call_control_server_state(CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE);
    lepl_cap_start_streaming(conn_id, p_stream_config);
}

wiced_bt_gatt_status_t lepl_tbs_handle_gatt_read_request(lepl_clcb_t *p_clcb,
                                                    uint16_t handle,
                                                    uint8_t *p_data,
                                                    uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    const uint8_t *p_data_start = p_data;
    lepl_local_service_data_t *p_local = &g_lepl_gatt_cb.local_service_data;
    lepl_tbs_t *p_tbs = &p_local->gtbs;

    switch (handle)
    {
    case HDLC_GTBS_BEARER_PROVIDER_NAME_VALUE:
    {
        int len = strlen((char *)p_local->gtbs.bearer_provider_name);
        memcpy(p_data, p_local->gtbs.bearer_provider_name, len);
        p_data += len;
    }
    break;
    case HDLC_GTBS_BEARER_UCI_VALUE:
    {
        int len = strlen((char *)p_local->gtbs.bearer_uci);
        memcpy(p_data, p_local->gtbs.bearer_uci, len);
        p_data += len;
    }
    break;
    case HDLC_GTBS_BEARER_TECHNOLOGY_VALUE:
        UINT8_TO_STREAM(p_data, p_local->gtbs.bearer_technology);
        break;
    case HDLC_GTBS_BEARER_URI_SCHEMES_VALUE: // 0x022A , 554
    {
        int len = strlen((char *)p_local->gtbs.bearer_uri);
        memcpy(p_data, p_local->gtbs.bearer_uri, *p_len);
        p_data += len;
    }
    break;

    case HDLC_GTBS_BEARER_SIGNAL_STRENGTH_VALUE: // 0x022D , 557
        UINT8_TO_STREAM(p_data, p_local->gtbs.bearer_signal_strength);
        break;
    case HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE: // 0x0230 , 560
        UINT8_TO_STREAM(p_data, p_local->gtbs.bearer_signal_strength_reporting_interval);
        break;

    case HDLC_GTBS_BEARER_LIST_CURRENT_CALL_VALUE: // 0x0232 , 562
    {
        lepl_tbs_call_state_data_t *p_call_state = &p_local->gtbs.call_state_data[0];
        if (!p_call_state->in_use)
            break;
        ga_lib_tbs_current_call_t *p_call = &p_call_state->call;
        UINT8_TO_STREAM(p_data, 3 + strlen(p_call_state->uri));          // list item length
        UINT8_TO_STREAM(p_data, p_call->call_id);                      // call index
        UINT8_TO_STREAM(p_data, p_call->call_state);                   // call state
        UINT8_TO_STREAM(p_data, p_call->call_flags);                   // call flags
        ARRAY_TO_STREAM(p_data, p_call_state->uri, strlen(p_call_state->uri)); // call URI + null termination
    }
    break;
    case HDLC_GTBS_CONTENT_CONTROL_ID_VALUE: // 0x0235 , 565
    {
        UINT8_TO_STREAM(p_data, p_local->gtbs.content_control_id);
    }
    break;
    case HDLC_GTBS_INCOMING_TG_URI_VALUE: // 0x0237 , 567
    {
        int len = strlen((char *)p_local->gtbs.incoming_tg_caller_id);
        memcpy(p_data, p_local->gtbs.incoming_tg_caller_id, len);
        p_data += len;
    }
    break;
    case HDLC_GTBS_STATUS_FLAG_VALUE: // 0x023A , 570
    {
        UINT16_TO_STREAM(p_data, p_local->gtbs.bearer_status_flag); // bearer status flag
    }
    break;
    case HDLC_GTBS_CALL_STATE_VALUE: // 0x023D , 573
    {
        lepl_tbs_call_state_data_t *p_call_state = &p_local->gtbs.call_state_data[0];

        if (p_call_state->in_use == 0)
        {
            break;
        }
        UINT8_TO_STREAM(p_data, p_call_state->call.call_id);    // call index
        UINT8_TO_STREAM(p_data, p_call_state->call.call_state); // call state
        UINT8_TO_STREAM(p_data, p_call_state->call.call_flags); // call flags
    }
    break;
    case HDLC_GTBS_CALL_CONTROL_POINT_VALUE: // 0x0240 , 576
    {
        // This is a writable characteristic, read is not expected. Return error.
        status = WICED_BT_GATT_READ_NOT_PERMIT;
    }
    break;
    case HDLC_GTBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE_VALUE: // 0x0243 , 579
    {
        UINT16_TO_STREAM(p_data, p_local->gtbs.ccp_supported_opcode);
    }
    break;
    case HDLC_GTBS_TERMINATION_REASON_VALUE: // 0x0245 , 581
    { // This is a notify only characteristic, read is not expected. Return error.
        status = WICED_BT_GATT_READ_NOT_PERMIT;
    }
    break;
    case HDLC_GTBS_INCOMING_CALL_VALUE: // 0x0248 , 584
    {
        lepl_tbs_call_state_data_t *p_call = get_call_state(p_local->gtbs.latest_incoming_remote_call_id,
                                                            p_tbs->call_state_data,
                                                            LEPL_TBS_BEARER_MAX_CALL_COUNT);
        if (p_call)
        {
            UINT8_TO_STREAM(p_data, p_local->gtbs.latest_incoming_remote_call_id); // call index
            ARRAY_TO_STREAM(p_data,
                            p_local->gtbs.incoming_tg_caller_id,
                            strlen(p_local->gtbs.incoming_tg_caller_id)); // call flags
        }
    }
    break;
    case HDLC_GTBS_CALL_FRIENDLY_NAME_VALUE: // 0x024B , 587
    {
        lepl_tbs_call_state_data_t *p_call;
        uint8_t call_id = p_local->gtbs.latest_incoming_remote_call_id;


        p_call = get_call_state(call_id, p_tbs->call_state_data, LEPL_TBS_BEARER_MAX_CALL_COUNT);

        if (p_call)
        {
            UINT8_TO_STREAM(p_data, call_id); // friendly call index
            ARRAY_TO_STREAM(p_data, p_call->friendly_name, strlen(p_call->friendly_name));
        }
    }
    break;

    default:
        status = WICED_BT_GATT_INVALID_HANDLE;
        break;
    }

    *p_len = p_data - p_data_start;
    WICED_BT_TRACE("[%s] hdl %d len %d status 0x%x", __FUNCTION__, handle, *p_len, status);

    return status;
}
