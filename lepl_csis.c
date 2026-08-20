/*
* $ Copyright Cypress Semiconductor $
*/

#include "lepl.h"

#define MAX_DEVICES 2
#define LEPL_CSIP_SET_MEMBER_DISCOVERY_TIMEOUT_IN_SEC 100

lepl_coordinated_set_member_t *get_device(lepl_cordinated_set_t *p_set, wiced_bt_device_address_t address)
{
    for (int i = 0; i < p_set->set_size; i++)
    {
        lepl_coordinated_set_member_t *p_mem = &p_set->member_list[i];

        WICED_BT_TRACE("[%s] %d. %B %B ", __FUNCTION__, i, address, p_mem->address);
        if (memcmp(address, p_mem->address, BD_ADDR_LEN) == 0)
        {
            return &p_set->member_list[i];
        }
    }
    return NULL;
}

static void csis_scan_timeout(WICED_TIMER_PARAM_TYPE p_inst)
{
    //scan timeout stop scan
    lepl_start_stop_set_member_discovery((lepl_cordinated_set_t *)p_inst, 0);
}

void set_current_device(lepl_cordinated_set_t *p_set, lepl_clcb_t *p_clcb)
{
    lepl_coordinated_set_member_t *p_dev;

    WICED_BT_TRACE("[%s] p_set %x num %d size %d", __FUNCTION__, p_set, p_set->num_devices, p_set->set_size);

    //if device is not present add it
    if ((p_dev = get_device(p_set, p_clcb->conn_addr)) == NULL)
    {
        WICED_BT_TRACE("[%s] num %d size %d", __FUNCTION__, p_set->num_devices, p_set->set_size);
        if ((p_set->num_devices < p_set->set_size) && (p_set->num_devices < LEPL_MAX_CONNECTIONS))
        {
            int index;
            for (index = 0; index < p_set->set_size; index++)
            {
                if (!p_set->member_list[index].is_used)
                {
                    WICED_BT_TRACE("[%s] index %d", __FUNCTION__, index);
                    WICED_MEMCPY(p_set->member_list[index].address, p_clcb->identity_bd_address, BD_ADDR_LEN);
                    p_dev = &p_set->member_list[index];
                    p_dev->is_used = 1;
                    p_dev->conn_id = p_clcb->conn_id;
                    p_dev->rank = p_clcb->csis.rank;
                    p_dev->addr_type = p_clcb->addr_type;
                    p_set->num_devices++;
                    break;
                }
            }
        }
        else
        {
            WICED_BT_TRACE_CRIT("[%s] Device cannot be added!", __FUNCTION__);
        }
    }
    else
    {
        if (!p_dev->is_used)
        {
            p_dev->is_used = 1;
            p_set->num_devices++;
        }
        WICED_MEMCPY(p_dev->address, p_clcb->identity_bd_address, BD_ADDR_LEN);
        p_dev->conn_id = p_clcb->conn_id;
        p_dev->rank = p_clcb->csis.rank;
        p_dev->addr_type = p_clcb->addr_type;
    }
}

static void csis_scan_result_cback(wiced_ble_ext_scan_results_t *p_scan_result, uint16_t adv_len, uint8_t *p_adv_data)
{
    if ((p_scan_result == NULL) || (p_adv_data == NULL) || (adv_len == 0))
    {
        return;
    }
    lepl_cordinated_set_t *p_set = NULL;
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (lepl_if_sirk_zero(g_lepl_gatt_cb.csis_set[i].sirk))
            continue;

        if (ga_lib_csis_check_if_belongs_to_coordinated_set(p_scan_result,
                                                            adv_len,
                                                            p_adv_data,
                                                            &g_lepl_gatt_cb.csis_set[i].sirk))
        {
            p_set = &g_lepl_gatt_cb.csis_set[i];
            break;
        }
    }

    if (p_set == NULL)
    {
        WICED_BT_TRACE("[%s] Not a CSIS Set Member!", __FUNCTION__);
        return;
    }

    WICED_BT_TRACE("[%s] Found CSIS Device!: %B ", __FUNCTION__, p_scan_result->remote_bd_addr);
    lepl_coordinated_set_member_t *p_dev = get_device(p_set, p_scan_result->remote_bd_addr);
    if (p_dev == NULL)
    {
        int index;
        for (index = 0; index < p_set->set_size; index++)
        {
            if (!p_set->member_list[index].is_used)
            {
                WICED_BT_TRACE("[%s] index %d", __FUNCTION__, index);
                p_dev = &p_set->member_list[index];
                break;
            }
        }
    }
    else if (p_dev->is_used)
    {

        WICED_BT_TRACE("[%s] Already Found! %B", __FUNCTION__, p_scan_result->remote_bd_addr);
        return;
    }

    if (p_dev)
    {
        p_dev->is_used = 1;
        WICED_MEMCPY(p_dev->address, p_scan_result->remote_bd_addr, BD_ADDR_LEN);
        p_dev->addr_type = p_scan_result->ble_addr_type;
        p_set->num_devices++;
        lepl_start_stop_set_member_discovery(p_set, 0);
        if (lepl_gatt_get_clcb(p_dev->address) == NULL)
        {
            app_create_connection(p_scan_result->ble_addr_type, p_dev->address);
        }
    }
}


uint8_t lepl_if_sirk_zero(uint8_t *sirk)
{
    uint8_t res = 0;
    for (int i = 0; i < GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN; i++)
        res |= sirk[i];
    return (res == 0) ? 1 : 0;
}

wiced_result_t lepl_start_stop_set_member_discovery(lepl_cordinated_set_t *p_set, uint8_t start_scan)
{
    wiced_result_t status;
    uint8_t timer_in_use = 0;

    WICED_BT_TRACE("[%s] p_set 0x%x num_devices %d start_scan %d", __FUNCTION__, p_set, p_set->num_devices, start_scan);

    if (!p_set)
    {
        return WICED_SUCCESS;
    }


    if (p_set->scan_timer.p_cback == NULL)
    {
        wiced_init_timer(&p_set->scan_timer,
                         csis_scan_timeout,
                         (WICED_TIMER_PARAM_TYPE)p_set,
                         WICED_SECONDS_PERIODIC_TIMER);
    }

    lepl_ble_activity_state_t ble_activity = lepl_get_current_ble_activity();

    timer_in_use = wiced_is_timer_in_use(&p_set->scan_timer);
    if (start_scan && !timer_in_use && (ble_activity == LEPL_BLE_ACTIVITY_NONE))
    {
        wiced_start_timer(&p_set->scan_timer, LEPL_CSIP_SET_MEMBER_DISCOVERY_TIMEOUT_IN_SEC);
        status = lepl_start_stop_scan(1, csis_scan_result_cback);
        WICED_BT_TRACE("[%s] wiced_bt_ble_scan: %d\n", __FUNCTION__, status);
    }
    else if (!start_scan && timer_in_use && (ble_activity == LEPL_BLE_ACTIVITY_SCANNING))
    {
        status = lepl_start_stop_scan(0, csis_scan_result_cback);
        wiced_stop_timer(&p_set->scan_timer);
        WICED_BT_TRACE("[%s] stop: %d\n", __FUNCTION__, status);
    }
    return WICED_SUCCESS;
}

lepl_cordinated_set_t *allocate_csis_set(lepl_clcb_t *p_clcb)
{
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (!g_lepl_gatt_cb.csis_set[i].num_devices)
        {
            lepl_cordinated_set_t *p_set = &g_lepl_gatt_cb.csis_set[i];
            ga_lib_csis_sirk_data_t *p_sirk = &p_clcb->csis.sirk_data;
            p_set->set_size = p_clcb->csis.size;
            WICED_MEMSET(p_set->member_list, 0, LEPL_MAX_CONNECTIONS * sizeof(lepl_coordinated_set_member_t));
            p_set->sirk_type = p_sirk->sirk_type;
            p_set->group_lock_state = (p_clcb->csis.lock == GA_LIB_CSIS_LOCKED) ? CSIS_LOCKED : CSIS_UNLOCKED;
            WICED_MEMCPY(p_set->sirk, p_sirk->sirk, sizeof(ga_lib_csis_sirk_t));
            return &g_lepl_gatt_cb.csis_set[i];
        }
    }
    return NULL;
}

lepl_cordinated_set_t *get_csis_set(ga_lib_csis_sirk_t sirk)
{
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (0 == WICED_MEMCMP(g_lepl_gatt_cb.csis_set[i].sirk, sirk, GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN))
        {
            WICED_BT_TRACE("[%s] found sirk on %d %A", __FUNCTION__, i, sirk, sizeof(ga_lib_csis_sirk_t));
            return &g_lepl_gatt_cb.csis_set[i];
        }
    }
    WICED_BT_TRACE("[%s] no device with SIRK %A", __FUNCTION__, sirk, sizeof(ga_lib_csis_sirk_t));
    return NULL;
}

lepl_cordinated_set_t *lepl_csis_get_cordinated_set(uint16_t conn_id)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (p_clcb && p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle)
    {
        return get_csis_set(p_clcb->csis.sirk_data.sirk);
    }
    return NULL;
}

void lepl_csis_handle_gatt_connection(uint16_t conn_id)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_cordinated_set_t *p_set = get_csis_set(p_clcb->csis.sirk_data.sirk);

    WICED_BT_TRACE("[%s] p_set found %x", __FUNCTION__, p_set);
    if (!p_set)
    {
        p_set = allocate_csis_set(p_clcb);
        WICED_BT_TRACE("[%s] p_set alloced %x", __FUNCTION__, p_set);
    }

    if(!p_set)
    {
        WICED_BT_TRACE_CRIT("[%s] no set available", __FUNCTION__);
        return;
    }

    set_current_device(p_set, p_clcb);
    lepl_start_stop_set_member_discovery(p_set, (p_set->num_devices < p_set->set_size) ? 1 : 0);
    lepl_cap_handle_set_member_reconnection(conn_id);
}

void lepl_csis_handle_gatt_disconnection(wiced_bt_device_address_t address)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb(address);
    if (!p_clcb)
    {
        return;
    }
    lepl_cordinated_set_t *p_set = get_csis_set(p_clcb->csis.sirk_data.sirk);
    if (p_set == NULL)
    {
        WICED_BT_TRACE_CRIT("[%s] p_set is NULL", __FUNCTION__);
        return;
    }
    lepl_coordinated_set_member_t *p_dev = get_device(p_set, p_clcb->identity_bd_address);
    if (p_dev == NULL)
    {
        WICED_BT_TRACE_CRIT("[%s] p_dev is NULL", __FUNCTION__);
        return;
    }

    p_dev->is_used = 0;
    p_dev->conn_id = 0;
    p_set->num_devices--;
    if (p_set->num_devices == 0)
    {
        lepl_start_stop_set_member_discovery(p_set, 0);
        lepl_cap_handle_set_disconnection();
        WICED_MEMSET(p_set, 0, sizeof(lepl_cordinated_set_t));
    }
    else if (!p_clcb->disconnecting)
    {
        // Continue set member discovery as device is not disconnecting
        lepl_start_stop_set_member_discovery(p_set, 1);
    }
}

wiced_bool_t lepl_csis_check_is_csis_supported(lepl_clcb_t *p_clcb)
{
    if (p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle)
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

wiced_bool_t lepl_csis_device_belongs_to_coordinated_set(uint16_t conn_id, lepl_clcb_t *p_clcb, ga_lib_csis_sirk_t sirk)
{
    if (!p_clcb || !p_clcb->in_use)
    {
        return WICED_FALSE;
    }
    if (!lepl_csis_check_is_csis_supported(p_clcb))
    {
        WICED_BT_TRACE_CRIT("[%s] Doesn't support CSIS %x", __FUNCTION__, conn_id);
    }

    if (WICED_MEMCMP(p_clcb->csis.sirk_data.sirk, sirk, GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN) == 0)
    {
        WICED_BT_TRACE("[%s] Device belongs to coordinated set! %x", __FUNCTION__, conn_id);
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

void csis_handle_lock_state(uint16_t conn_id, lepl_clcb_t *p_clcb)
{
    lepl_cordinated_set_t *p_set = get_csis_set(p_clcb->csis.sirk_data.sirk);
    uint8_t expected_lock_val = p_clcb->csis.lock;
    if (!p_set)
    {
        return;
    }

    lepl_csis_lock_state_t group_lock_state = p_set->group_lock_state;
    if (expected_lock_val == GA_LIB_CSIS_LOCKED &&
        (group_lock_state != CSIS_LOCK_IN_PROGRESS && group_lock_state != CSIS_LOCKED))
    {
        lepl_csis_set_lock(p_clcb->conn_id, FALSE);
        return;
    }
    else if (expected_lock_val == GA_LIB_CSIS_UNLOCKED &&
             (group_lock_state != CSIS_UNLOCK_IN_PROGRESS && group_lock_state != CSIS_UNLOCKED))
    {
        lepl_csis_set_lock(p_clcb->conn_id, FALSE);
        return;
    }

    for (int i = 0; i < p_set->num_devices; i++)
    {
        p_clcb = lepl_gatt_get_clcb_by_conn_id(p_set->member_list[i].conn_id);
        if (p_clcb && p_clcb->csis.lock != expected_lock_val)
        {
            return;
        }
    }
    p_set->group_lock_state = (expected_lock_val == GA_LIB_CSIS_LOCKED) ? CSIS_LOCKED : CSIS_UNLOCKED;
    lepl_rpc_send_csis_lock_state(conn_id, expected_lock_val);
}

void lepl_csis_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data)
{
    int csis_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.csis,
                                                                        GA_LIB_CSIS_CHARACTERISTIC_MAX,
                                                                        p_gatt_data->handle);
    lepl_csis_data_t *p_csis = &p_clcb->csis;
    uint8_t *p_data = p_gatt_data->p_data;

    WICED_BT_TRACE("[%s] csis_index %d status %x \n", __FUNCTION__, csis_index, status);
    if (status != WICED_BT_GATT_SUCCESS)
    {
        WICED_BT_TRACE_CRIT("[%s] status %d", __FUNCTION__, status);
        return;
    }

    switch (csis_index)
    {
    case GA_LIB_CSIS_CHARACTERISTIC_SIRK: //0
    {
        // Handle ga_lib_csis_sirk_t characteristic
        STREAM_TO_UINT8(p_csis->sirk_data.sirk_type, p_data); //first byte is ga_lib_csis_sirk_t type
        if (p_csis->sirk_data.sirk_type == GA_LIB_CSIS_SIRK_ENCR)
        {
            wiced_bt_device_link_keys_t ltk = {0};
            ga_lib_csis_sirk_t enc_sirk = {0};
            WICED_MEMCPY(ltk.bd_addr, p_clcb->conn_addr, sizeof(wiced_bt_device_address_t));
            lepl_nvram_read_keys(&ltk);
            REVERSE_STREAM_TO_ARRAY(enc_sirk, p_data, GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN);
            ga_lib_csis_sirk_decryption_func(&enc_sirk, &ltk, &p_csis->sirk_data.sirk);
        }
        else
        {
            REVERSE_STREAM_TO_ARRAY(p_csis->sirk_data.sirk, p_data, GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN);
        }
        WICED_BT_TRACE("[%s] sirk type %x [%A]",
                       __FUNCTION__,
                       p_csis->sirk_data.sirk_type,
                       p_csis->sirk_data.sirk,
                       GA_LIB_CSIS_SET_IDENTITY_RESOLVING_KEY_LEN);
    }
    break;
    case GA_LIB_CSIS_CHARACTERISTIC_SIZE: //1
    {
        // Handle SIZE characteristic
        STREAM_TO_UINT8(p_csis->size, p_data);
        WICED_BT_TRACE("[%s] size value:%x ", __FUNCTION__, p_csis->size);
    }
    break;
    case GA_LIB_CSIS_CHARACTERISTIC_LOCK: //2
    {
        // Handle LOCK characteristic
        STREAM_TO_UINT8(p_csis->lock, p_data);
        WICED_BT_TRACE("[%s] lock value:%x ", __FUNCTION__, p_csis->lock);
        csis_handle_lock_state(p_clcb->conn_id, p_clcb);
    }
    break;
    case GA_LIB_CSIS_CHARACTERISTIC_RANK: //3
    {
        // Handle RANK characteristic
        STREAM_TO_UINT8(p_csis->rank, p_data);
        WICED_BT_TRACE("[%s] rank value:%x ", __FUNCTION__, p_csis->rank);
    }
    break;
    default:
        break;
    }
}

wiced_result_t lepl_csis_set_lock(uint16_t conn_id, uint8_t lock)
{
    return WICED_SUCCESS;
}
