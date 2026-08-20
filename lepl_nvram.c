/*
 * $ Copyright Cypress Semiconductor $
 */

/* Application includes */
#include "lepl.h"

/* BT Stack includes */
#include "wiced_hal_nvram.h"

#if SIMULATED_NVRAM
lepl_nvram_data_t local_nvram_key_store = {0};
void lepl_nvram_init(void)
{
    WICED_MEMSET(&local_nvram_key_store, 0, sizeof(local_nvram_key_store));
}

static wiced_bool_t is_bd_addr_zero(wiced_bt_device_address_t bd_addr)
{
    wiced_bt_device_address_t null_addr = {0};
    return (WICED_MEMCMP(null_addr, bd_addr, BD_ADDR_LEN) == 0);
}

wiced_result_t app_handle_irk_request_evt(wiced_bt_local_identity_keys_t *p_id_keys)
{
    // Handle IRK request event
    WICED_BT_TRACE("[%s] IRK request event received\n", __FUNCTION__);
    wiced_bt_local_identity_keys_t id_keys = {0};
    if (WICED_MEMCMP(&id_keys, &local_nvram_key_store.local_id_keys, sizeof(wiced_bt_local_identity_keys_t)) == 0)
    {
        return WICED_BT_ERROR; // No valid keys found
    }
    WICED_MEMCPY(p_id_keys, &local_nvram_key_store.local_id_keys, sizeof(wiced_bt_local_identity_keys_t));
    return WICED_BT_SUCCESS;
}

void app_handle_irk_update_evt(wiced_bt_local_identity_keys_t *p_id_keys)
{
    // Handle IRK update event
    WICED_BT_TRACE("[%s] IRK update event received\n", __FUNCTION__);
    WICED_MEMCPY(&local_nvram_key_store.local_id_keys, p_id_keys, sizeof(wiced_bt_local_identity_keys_t));
    lepl_rpc_send_identity_resolving_key(&local_nvram_key_store.local_id_keys);
}

lepl_nvram_paired_device_key_t *lepl_nvram_get_paired_device_key_info(wiced_bt_device_address_t bd_addr)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;

    for (int i = 0; i < MAX_NUM_DEVICES_IN_NVRAM; i++, p_pdkeys++)
    {
        if (!is_bd_addr_zero(bd_addr) &&
            ((WICED_MEMCMP(p_pdkeys->link_keys.bd_addr, bd_addr, BD_ADDR_LEN) == 0) ||
             (WICED_MEMCMP(p_pdkeys->link_keys.conn_addr, bd_addr, BD_ADDR_LEN) == 0)))
        {
            return p_pdkeys;
        }
    }
    return NULL;
}

lepl_nvram_paired_device_key_t * app_free_up_paired_device_key_info(void)
{
    // If all entries in NVRAM are occupied, overwrite the not connected one.
    int index = 0;
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;
    for (index = 0; index < MAX_NUM_DEVICES_IN_NVRAM; index++, p_pdkeys++)
    {
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb(p_pdkeys->link_keys.bd_addr);
        if (p_clcb == NULL)
        {
            break;
        }
    }

    WICED_BT_TRACE("[%s] Free up NVRAM entry at index %d for device %B",
                    __FUNCTION__,
                    index,
                    p_pdkeys->link_keys.bd_addr);
    return p_pdkeys;
}

uint16_t lepl_nvram_get_nvram_id(lepl_nvram_paired_device_key_t *p_pdkeys)
{
    return UNICAST_APP_NVRAM_ID_PAIRED_KEYS + (p_pdkeys - local_nvram_key_store.paired_device_keys);
}

// Allocate entry in NVRAM for new paired device and write the link keys to it.
lepl_nvram_paired_device_key_t *lepl_nvram_allocate_paired_device_key_info(wiced_bt_device_link_keys_t *p_linkkeys)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;
    for (int i = 0; i < MAX_NUM_DEVICES_IN_NVRAM; i++, p_pdkeys++)
    {
        if (is_bd_addr_zero(p_pdkeys->link_keys.bd_addr))
        {
            break;
        }
    }

    /* free up one entry */
    if (p_pdkeys == NULL)
    {
        p_pdkeys = app_free_up_paired_device_key_info();
    }
    WICED_MEMCPY(&p_pdkeys->link_keys, p_linkkeys, sizeof(wiced_bt_device_link_keys_t));
    return p_pdkeys;
}

int lepl_nvram_write_keys(wiced_bt_device_link_keys_t *p_linkkeys)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = lepl_nvram_get_paired_device_key_info(p_linkkeys->bd_addr);

    WICED_BT_TRACE("[%s] bda %B len %d %A",
                   __FUNCTION__,
                   p_linkkeys->bd_addr,
                   sizeof(wiced_bt_device_link_keys_t),
                   p_linkkeys,
                   sizeof(wiced_bt_device_link_keys_t));

    /* free up one entry */
    if (p_pdkeys == NULL)
    {
        p_pdkeys = lepl_nvram_allocate_paired_device_key_info(p_linkkeys);
    }

    return lepl_nvram_get_nvram_id(p_pdkeys);
}

int lepl_nvram_read_keys(wiced_bt_device_link_keys_t *p_linkkeys)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = lepl_nvram_get_paired_device_key_info(p_linkkeys->bd_addr);
    if (p_pdkeys == NULL)
    {
        return 0;
    }
    WICED_BT_TRACE("[%s] bda %B len %d",
                    __FUNCTION__,
                    p_linkkeys->bd_addr, sizeof(wiced_bt_device_link_keys_t));
    WICED_MEMCPY(p_linkkeys, &p_pdkeys->link_keys, sizeof(wiced_bt_device_link_keys_t));
    return lepl_nvram_get_nvram_id(p_pdkeys);
}

int app_read_paired_key_nvram_data(uint16_t nvram_id, lepl_nvram_paired_device_key_t *p_key_data)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;
    for (int i = 0; i < MAX_NUM_DEVICES_IN_NVRAM; i++, p_pdkeys++)
    {
        if (lepl_nvram_get_nvram_id(p_pdkeys) == nvram_id)
        {
            WICED_MEMCPY(p_key_data, p_pdkeys, sizeof(lepl_nvram_paired_device_key_t));
            return sizeof(lepl_nvram_paired_device_key_t);
        }
    }
    return 0;
}

void lepl_nvram_write_data(uint16_t nvram_id, uint8_t *p_key_data, uint32_t data_len, uint16_t offset)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;
    if (nvram_id == UNICAST_APP_NVRAM_ID_LOCAL_IRK)
    {
        if (wiced_ble_init_ctlr_private_addr_generation((wiced_bt_local_identity_keys_t *)p_key_data) == WICED_BT_SUCCESS)
        {
            WICED_MEMCPY(&local_nvram_key_store.local_id_keys, p_key_data, sizeof(wiced_bt_local_identity_keys_t));
        }
        return;
    }
    for (int i = 0; i < MAX_NUM_DEVICES_IN_NVRAM; i++, p_pdkeys++)
    {
        if (lepl_nvram_get_nvram_id(p_pdkeys) == nvram_id)
        {
            WICED_MEMCPY((uint8_t *)p_pdkeys + offset, p_key_data, data_len);
            wiced_bt_dev_add_device_to_address_resolution_db(&p_pdkeys->link_keys);
            break;
        }
    }

}

void lepl_nvram_delete_keys(uint16_t nvram_id)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = local_nvram_key_store.paired_device_keys;
    if (nvram_id == UNICAST_APP_NVRAM_ID_LOCAL_IRK)
    {
        WICED_MEMSET(&local_nvram_key_store.local_id_keys, 0, sizeof(wiced_bt_local_identity_keys_t));
        return;
    }
    for (int i = 0; i < MAX_NUM_DEVICES_IN_NVRAM; i++, p_pdkeys++)
    {
        if (lepl_nvram_get_nvram_id(p_pdkeys) == nvram_id)
        {
            wiced_bt_dev_remove_device_from_address_resolution_db(&p_pdkeys->link_keys);
            lepl_clcb_t *p_clcb = lepl_gatt_get_clcb(p_pdkeys->link_keys.conn_addr);
            if (p_clcb)
            {
                lepl_disconnect_device(p_clcb->conn_id);
            }
            WICED_MEMSET(&p_pdkeys->link_keys, 0, sizeof(wiced_bt_device_link_keys_t));
            break;
        }
    }
}

void lepl_print_nvram_data(char *msg, lepl_nvram_paired_device_key_t *p_pdkeys)
{
    lepl_peer_profiles_t *p_pp = &p_pdkeys->peer_profiles;
    gatt_intf_service_range_t *p_sr = p_pp->service_handles;

    WICED_BT_TRACE("[%s] %s", __FUNCTION__, msg);
    WICED_BT_TRACE("[%s] NVRAM ID 0x%x %B size %d stored_ccd_size %d",
                   __FUNCTION__,
                   lepl_nvram_get_nvram_id(p_pdkeys),
                   p_pdkeys->link_keys.bd_addr,
                   sizeof(lepl_nvram_paired_device_key_t),
                   sizeof(p_pdkeys->stored_cccd_bits));
    uint8_t *p_link_keys = (uint8_t *)&p_pdkeys->link_keys;
    for (int i = 0; i < sizeof(wiced_bt_device_link_keys_t); i += 32)
    {
        int end = (i + 31) > sizeof(wiced_bt_device_link_keys_t) ? sizeof(wiced_bt_device_link_keys_t) : (i + 31);
        WICED_BT_TRACE("[%s] Link Keys[%d:%d]: %A", __FUNCTION__, i, end, p_link_keys + i, end - i + 1);
    }
    WICED_BT_TRACE("[dbhash] %A", p_pdkeys->db_hash, sizeof(wiced_bt_db_hash_t));
    {
        gatt_intf_characteristic_handles_t *p = p_pp->pacs;
        WICED_BT_TRACE("[pacs %d-%d] ctx_supp (%d) ctx_avail (%d, %d) loc_snk (%d) loc_src (%d)",
                       p_sr[LEPL_PACS].start_handle,
                       p_sr[LEPL_PACS].end_handle,
                       p[GA_LIB_PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS].value_handle,
                       p[GA_LIB_PACS_CHARACTERISTIC_AVAILABILE_AUDIO_CONTEXTS].value_handle,
                       p[GA_LIB_PACS_CHARACTERISTIC_AVAILABILE_AUDIO_CONTEXTS].descriptor_handle,
                       p[GA_LIB_PACS_CHARACTERISTIC_SNK_AUDIO_LOCATIONS].value_handle,
                       p[GA_LIB_PACS_CHARACTERISTIC_SRC_AUDIO_LOCATIONS].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->ascs;
        gatt_intf_characteristic_handles_t *p_snk = &p_pp->ascs[GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE];
        gatt_intf_characteristic_handles_t *p_src =
            &p_pp->ascs[GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE + LEPL_ASCS_MAX_SNK_CHARACTERISTICS];

        WICED_BT_TRACE("[ascs %d-%d] cp %d snk %d, %d src %d, %d",
                       p_sr[LEPL_ASCS].start_handle,
                       p_sr[LEPL_ASCS].end_handle,
                       p[GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT].value_handle,
                       p_snk[0].value_handle,
                       p_snk[1].value_handle,
                       p_src[0].value_handle,
                       p_src[1].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->csis;
        WICED_BT_TRACE("[csis %d-%d] sirk %d size %d rank %d loc %d",
                       p_sr[LEPL_CSIS].start_handle,
                       p_sr[LEPL_CSIS].end_handle,
                       p[GA_LIB_CSIS_CHARACTERISTIC_SIRK].value_handle,
                       p[GA_LIB_CSIS_CHARACTERISTIC_SIZE].value_handle,
                       p[GA_LIB_CSIS_CHARACTERISTIC_RANK].value_handle,
                       p[GA_LIB_CSIS_CHARACTERISTIC_LOCK].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->vcs;
        WICED_BT_TRACE("[vcs %d-%d] state %d cp %d flag %d",
                       p_sr[LEPL_VCS].start_handle,
                       p_sr[LEPL_VCS].end_handle,
                       p[GA_LIB_VCS_CHARACTERISTIC_VOLUME_STATE].value_handle,
                       p[GA_LIB_VCS_CHARACTERISTIC_CONTROL_POINT].value_handle,
                       p[GA_LIB_VCS_CHARACTERISTIC_VOLUME_FLAG].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->mics;
        WICED_BT_TRACE("[mics %d-%d] state %d",
                       p_sr[LEPL_MICS].start_handle,
                       p_sr[LEPL_MICS].end_handle,
                       p[GA_LIB_MICS_CHARACTERISTIC_MUTE_STATE].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->has;
        WICED_BT_TRACE("[has %d-%d] feat %d cp %d api %d",
                       p_sr[LEPL_HAS].start_handle,
                       p_sr[LEPL_HAS].end_handle,
                       p[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_FEATURES].value_handle,
                       p[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT].value_handle,
                       p[GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_ACTIVE_PRESET_INDEX].value_handle);
    }
    {
        WICED_BT_TRACE("[mics_aics %d-%d]", p_sr[LEPL_MICS_AICS].start_handle, p_sr[LEPL_MICS_AICS].end_handle);
    }
    {
        WICED_BT_TRACE("[ias %d-%d]", p_sr[LEPL_IAS].start_handle, p_sr[LEPL_IAS].end_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->gmap;
        WICED_BT_TRACE("[gmap %d-%d] role %d",
                       p_sr[LEPL_GMAP].start_handle,
                       p_sr[LEPL_GMAP].end_handle,
                       p[GA_LIB_GMAP_CHARACTERISTIC_ROLE].value_handle);
    }
    {
        gatt_intf_characteristic_handles_t *p = p_pp->bass;
        gatt_intf_characteristic_handles_t *p_bass = &p_pp->bass[GA_LIB_BASS_CHARACTERISTIC_MAX_UNIQUE];
        WICED_BT_TRACE("[bass %d-%d] cp %d bass %d",
                       p_sr[LEPL_BASS].start_handle,
                       p_sr[LEPL_BASS].end_handle,
                       p[GA_LIB_BASS_CHARACTERISTIC_BROADCAST_AUDIO_SCAN_CONTROL_POINT].value_handle,
                       p_bass[0].value_handle);
    }
    WICED_BT_TRACE("[stored_cccd_bits] %08x-%08x", p_pdkeys->stored_cccd_bits[0], p_pdkeys->stored_cccd_bits[1]);
}


void lepl_save_device_data_to_nvram(lepl_clcb_t *p_clcb)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = lepl_nvram_get_paired_device_key_info(p_clcb->identity_bd_address);
    if (p_pdkeys == NULL)
    {
        WICED_BT_TRACE("[%s] No paired device found for %B", __FUNCTION__, p_clcb->identity_bd_address);
        return;
    }
    WICED_MEMCPY(&p_pdkeys->peer_profiles, &p_clcb->peer_profiles, sizeof(lepl_peer_profiles_t));
    WICED_MEMCPY(p_pdkeys->db_hash, &p_clcb->db_hash, sizeof(wiced_bt_db_hash_t));
    WICED_MEMCPY(p_pdkeys->stored_cccd_bits, p_clcb->cccd_bits, sizeof(p_clcb->cccd_bits));
    lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(p_clcb->conn_id);
    if (p_set != NULL)
    {
        for (int i = 0; i< p_set->set_size; i++)
        {
            lepl_coordinated_set_member_t *p_dev = &p_set->member_list[i];
            if (WICED_MEMCMP(p_dev->address, p_clcb->identity_bd_address, BD_ADDR_LEN) != 0)
            {
                WICED_MEMCPY(p_pdkeys->csis_pair_device.bda, p_dev->address, BD_ADDR_LEN);
                p_pdkeys->csis_pair_device.type = p_dev->addr_type;
                break;
            }
        }
    }
    lepl_print_nvram_data("Saving device data", p_pdkeys);

    lepl_rpc_send_link_keys(lepl_nvram_get_nvram_id(p_pdkeys), p_pdkeys);
}


#else
void lepl_nvram_init(void)
{
}

static int get_next_nvram_index(int current_nvram_index)
{
    if (current_nvram_index >= UNICAST_APP_NVRAM_ID_PAIRED_KEYS &&
        (current_nvram_index + UNICAST_APP_MAX_IDS_PER_DEVICE_ALIGNED) < UNICAST_APP_NVRAM_ID_PAIRED_KEYS_MAX)
    {
        return current_nvram_index + UNICAST_APP_MAX_IDS_PER_DEVICE_ALIGNED;
    }

    if (current_nvram_index == 0)
    {
        return UNICAST_APP_NVRAM_ID_PAIRED_KEYS;
    }

    return 0;
}

int lepl_nvram_write(int nvram_id, wiced_bt_device_address_t bdaddr, uint8_t *p_data, uint32_t len)
{
    uint32_t write_len;
    wiced_result_t result;

    WICED_BT_TRACE_CRIT("[%s] nvram id 0x%x", __FUNCTION__, nvram_id);

    if ((nvram_id <= UNICAST_APP_NVRAM_ID_START) || (nvram_id >= UNICAST_APP_NVRAM_ID_END))
    {
        WICED_BT_TRACE_CRIT("[%s] bad id 0x%x", __FUNCTION__, nvram_id);
        return 0;
    }
    write_len = wiced_hal_write_nvram(nvram_id, len, (uint8_t *)p_data, &result);

    WICED_BT_TRACE_CRIT("[%s] write nvram result 0x%x", __FUNCTION__, result);

    if (write_len != len)
    {
        return 0;
    }

    WICED_BT_TRACE_CRIT("[%s] bytes written 0x%x", __FUNCTION__, write_len);
    return write_len;
}


int lepl_nvram_write_keys(wiced_bt_device_link_keys_t *p_linkkeys)
{
    int nvram_id = 0;
    lepl_nvram_data_t bonded_dev_info = {0};
    uint32_t last_used_nvram_id = 0;
    uint32_t write_last_used_nvram_id = 0;

    if (p_linkkeys == NULL)
    {
        WICED_BT_TRACE("[%s] p_linkkeys is null", __FUNCTION__, p_linkkeys);
        return 0;
    }

    WICED_BT_TRACE("[%s] bda %B %A",
                   __FUNCTION__,
                   p_linkkeys->bd_addr,
                   p_linkkeys,
                   sizeof(wiced_bt_device_link_keys_t));

    nvram_id = lepl_nvram_read_peer_device(p_linkkeys->bd_addr, &bonded_dev_info);
    if (nvram_id == 0)
    {
        WICED_BT_TRACE("[%s] nvram_id 0x%x", __FUNCTION__, nvram_id);
        if (!lepl_nvram_read(UNICAST_APP_NVRAM_ID_LAST_PAIRED_KEY,
                             p_linkkeys->bd_addr,
                             (uint8_t *)&last_used_nvram_id,
                             sizeof(last_used_nvram_id)))
        {
            // no last used nvram entry
            last_used_nvram_id = UNICAST_APP_NVRAM_ID_PAIRED_KEYS;
            WICED_BT_TRACE("[%s] no last used nvram id 0x%x", __FUNCTION__, last_used_nvram_id);
        }
        else
        {
            // found last used nvram entry, increment it
            WICED_BT_TRACE("[%s] last_used_nvram_id 0x%x", __FUNCTION__, last_used_nvram_id);
            last_used_nvram_id += (last_used_nvram_id + 1) % MAX_NUM_DEVICES_IN_NVRAM;
        }

        //create entry
        nvram_id = last_used_nvram_id;
        write_last_used_nvram_id = 1;
    }

    bonded_link_keys = *p_linkkeys;
    lepl_nvram_write(nvram_id, p_linkkeys->bd_addr, (uint8_t *)&bonded_dev_info, sizeof(bonded_dev_info));

    if (write_last_used_nvram_id)
    {
        lepl_nvram_write(UNICAST_APP_NVRAM_ID_LAST_PAIRED_KEY,
                         p_linkkeys->bd_addr,
                         (uint8_t *)&last_used_nvram_id,
                         sizeof(last_used_nvram_id));
    }

    return nvram_id;
}

int lepl_nvram_read_keys(wiced_bt_device_link_keys_t *p_linkkeys)
{
    int nvram_id = 0;
    lepl_nvram_data_t bonded_dev_info;

    if (p_linkkeys == NULL)
    {
        WICED_BT_TRACE("[%s] p_linkkeys is null", __FUNCTION__, p_linkkeys);
        return 0;
    }

    nvram_id = lepl_nvram_read_peer_device(p_linkkeys->bd_addr, &bonded_dev_info);
    if (nvram_id == 0)
    {
        return nvram_id;
    }

    *p_linkkeys = bonded_link_keys;

    WICED_BT_TRACE("[%s] %d %A", __FUNCTION__, nvram_id, p_linkkeys, sizeof(wiced_bt_device_link_keys_t));

    return nvram_id;
}

void lepl_nvram_delete(int nvram_id, wiced_bt_device_address_t bdaddr)
{
    wiced_result_t result = WICED_SUCCESS;
    wiced_hal_delete_nvram(nvram_id, &result);
    WICED_BT_TRACE("[%s] result %d", __FUNCTION__, result);
}
#endif
