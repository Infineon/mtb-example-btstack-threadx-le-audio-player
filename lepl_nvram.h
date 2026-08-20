/*
 * $ Copyright Cypress Semiconductor $
 */

#ifndef __LEPL_NVRAM_H__
#define __LEPL_NVRAM_H__

#include "lepl.h"
#include "wiced_hal_nvram.h"


#define MAX_NUM_DEVICES_IN_NVRAM 10 /**< Maximum number of devices to store in NVRAM */
#define LEPL_MAX_CCCD_TO_STORE  24U /**< Maximum number of CCCDs to store in NVRAM */
#define LEPL_MAX_DWORD_TO_STORE_CCCD(count)            \
    (((count) * 2) / 32 + ((((count) * 2) % 32) ? 1 : 0)) /**< Calculate the number of DWORDs needed to store the CCCDs */

/*
* @brief NVRAM ID definitions for storing local identity keys and paired device keys.
*/
enum
{
    UNICAST_APP_NVRAM_ID_START =
        (WICED_NVRAM_VSID_START),         // 0x200   /**< Start of NVRAM ID range for the application */
    UNICAST_APP_NVRAM_ID_LOCAL_IRK,       // 0x201   /**< NVRAM ID for storing the local identity resolving key (IRK) */
    UNICAST_APP_NVRAM_ID_LAST_PAIRED_KEY, // 0x202   /**< NVRAM ID for storing the last paired device key */

    UNICAST_APP_NVRAM_ID_PAIRED_KEYS, // 0x203   /**< NVRAM ID for storing paired device keys */
    UNICAST_APP_NVRAM_ID_END = UNICAST_APP_NVRAM_ID_PAIRED_KEYS +
                               MAX_NUM_DEVICES_IN_NVRAM, // 0x20D   /**< End of NVRAM ID range for the application */
};

#define LEPL_MAX_MICS_AICS 2 /**< Maximum number of MICS AICS characteristics can be stored*/


/*
* @brief Enumeration of supported LE Audio profiles for storing service and characteristic handles.
*/
enum lepl_profiles_e
{
    LEPL_PACS,   //0
    LEPL_ASCS,   //1
    LEPL_VCS,    //2
    LEPL_CSIS,   //3
    LEPL_MICS,   //4
    LEPL_HAS,    //5
    LEPL_IAS,    //6
    LEPL_GMAP,   //7
    LEPL_BASS,   //8
    LEPL_MICS_AICS, //9
    /* if(LEPL_MAX_MICS_AICS ==2), then LEPL_MICS_AICS_END = LEPL_MICS_AICS + (2 - 1) */
    LEPL_MICS_AICS_END = LEPL_MICS_AICS + LEPL_MAX_MICS_AICS - 1, // 10
    LEPL_MAX_PROFILES // 11
};
#define LEPL_PACS_MAX_SNK_CHARACTERISTICS 5     /**< Maximum number of PACS sink characteristics can be stored*/
#define LEPL_PACS_MAX_SRC_CHARACTERISTICS 5     /**< Maximum number of PACS source characteristics can be stored*/

#define LEPL_ASCS_MAX_SNK_CHARACTERISTICS 5     /**< Maximum number of ASCS sink characteristics can be stored*/
#define LEPL_ASCS_MAX_SRC_CHARACTERISTICS 5     /**< Maximum number of ASCS source characteristics can be stored*/

#define LEPL_MAX_BASS_RCV_STATE_SUPPORTED 2     /**< Maximum number of BASS receiver state characteristics can be stored*/


/*
* @brief Structure to store the GATT service and characteristic handles for the peer device profiles.
*/
typedef struct
{
    gatt_intf_service_range_t service_handles[LEPL_MAX_PROFILES]; /**< Service handles for each profile */
    gatt_intf_characteristic_handles_t
        pacs[GA_LIB_PACS_CHARACTERISTIC_MAX_UNIQUE + LEPL_PACS_MAX_SNK_CHARACTERISTICS +
             LEPL_PACS_MAX_SRC_CHARACTERISTICS]; /**< Characteristic handles for PACS profile */
    gatt_intf_characteristic_handles_t
        ascs[GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE + LEPL_ASCS_MAX_SNK_CHARACTERISTICS +
             LEPL_ASCS_MAX_SRC_CHARACTERISTICS]; /**< Characteristic handles for ASCS profile */
    gatt_intf_characteristic_handles_t
        vcs[GA_LIB_VCS_CHARACTERISTIC_MAX]; /**< Characteristic handles for VCS profile */
    gatt_intf_characteristic_handles_t
        mics[GA_LIB_MICS_CHARACTERISTIC_MAX]; /**< Characteristic handles for MICS profile */
    gatt_intf_characteristic_handles_t
        csis[GA_LIB_CSIS_CHARACTERISTIC_MAX]; /**< Characteristic handles for CSIS profile */
    gatt_intf_characteristic_handles_t
        has[GA_LIB_HAS_CHARACTERISTIC_MAX]; /**< Characteristic handles for HAS profile */
    gatt_intf_characteristic_handles_t
        ias[GA_LIB_IAS_CHARACTERISTIC_MAX]; /**< Characteristic handles for IAS profile */
    gatt_intf_characteristic_handles_t
        gmap[GA_LIB_GMAP_CHARACTERISTIC_MAX]; /**< Characteristic handles for GMAP profile */
    gatt_intf_characteristic_handles_t
        bass[GA_LIB_BASS_CHARACTERISTIC_MAX_UNIQUE +
             LEPL_MAX_BASS_RCV_STATE_SUPPORTED]; /**< Characteristic handles for BASS profile */
    gatt_intf_characteristic_handles_t
        mics_aics[LEPL_MAX_MICS_AICS]
                 [GA_LIB_AICS_CHARACTERISTIC_MAX]; /**< Characteristic handles for MICS AICS profile */
} lepl_peer_profiles_t;

/*
* @brief Structure to store the paired device keys and related information in NVRAM.
*/
typedef struct
{
    wiced_bt_device_link_keys_t link_keys; /**< Link keys for the paired device */
    uint32_t stored_cccd_bits[LEPL_MAX_DWORD_TO_STORE_CCCD(
        LEPL_MAX_CCCD_TO_STORE)]; /**< Stored CCCD bits for the paired device */
    wiced_bt_db_hash_t db_hash;   /**< Database hash for the paired device */
    wiced_bt_gatt_client_supported_features_t csf; /**< Client supported features for the paired device */
    lepl_peer_profiles_t peer_profiles; /**< GATT service and characteristic handles for the paired device profiles */
    wiced_bt_ble_address_t csis_pair_device; /**< device address of second member of CSIS set */
} lepl_nvram_paired_device_key_t;

/*
* @brief Structure to store the local identity keys and paired device keys in NVRAM.
*/
typedef struct
{
    wiced_bt_local_identity_keys_t local_id_keys; /**< Local identity keys */
    lepl_nvram_paired_device_key_t paired_device_keys[MAX_NUM_DEVICES_IN_NVRAM]; /**< Paired device keys */
} lepl_nvram_data_t;


/*
* @brief Read the paired device keys from NVRAM.
*
* @param[out] p_linkkeys: Reference to the structure to store the read link keys.
* @return nvram id if successful, otherwise 0.
*/
int lepl_nvram_read_keys(wiced_bt_device_link_keys_t *p_linkkeys);

/*
* @brief Write the paired device keys to NVRAM.
*
* @param[in] p_linkkeys: Reference to the structure containing the link keys to write.
* @return nvram id if successful, otherwise 0.
*/
int lepl_nvram_write_keys(wiced_bt_device_link_keys_t *p_linkkeys);

/*
* @brief Initialize the NVRAM.
*/
void lepl_nvram_init(void);

/*
* @brief Handle the IRK request event.
*
* @param[out] p_id_keys: Reference to the structure to store the local identity keys.
* @return WICED_SUCCESS if the IRK present, otherwise an error code.
*/
wiced_result_t app_handle_irk_request_evt(wiced_bt_local_identity_keys_t *p_id_keys);

/*
* @brief Handle the IRK update event.
*
* @param[in] p_id_keys: Reference to the structure containing the updated local identity keys.
*/
void app_handle_irk_update_evt(wiced_bt_local_identity_keys_t *p_id_keys);

/*
* @brief Store NVRAM data received from the client control.
*
* @param[in] nvram_id: The NVRAM ID to write the data to.
* @param[in] p_key_data: Pointer to the data to write.
* @param[in] data_len: Length of the data to write.
* @param[in] offset: Offset in the NVRAM to write the data.
*/
void lepl_nvram_write_data(uint16_t nvram_id, uint8_t *p_key_data, uint32_t data_len, uint16_t offset);

/*
* @brief Delete the paired device keys from NVRAM.
*
* @param[in] nvram_id: The NVRAM ID of the paired device keys to delete.
*/
void lepl_nvram_delete_keys(uint16_t nvram_id);

/*
* @brief Get the paired device key information for a given Bluetooth device address.
*
* @param[in] bd_addr: The Bluetooth device address of the paired device.
* @return Reference to the paired device key information, or NULL if not found.
*/
lepl_nvram_paired_device_key_t *lepl_nvram_get_paired_device_key_info(wiced_bt_device_address_t bd_addr);

/*
* @brief Get the NVRAM ID for a given paired device key information.
*
* @param[in] p_pdkeys: Reference to the paired device key information.
* @return The NVRAM ID for the paired device key information, or 0 if not found.
*/
uint16_t lepl_nvram_get_nvram_id(lepl_nvram_paired_device_key_t *p_pdkeys);

/*
* @brief Print the NVRAM data for debugging purposes.
*
* @param[in] msg: Message to print before the NVRAM data.
* @param[in] p_pdkeys: Reference to the paired device key information to print.
*/
void lepl_print_nvram_data(char *msg, lepl_nvram_paired_device_key_t *p_pdkeys);

#endif /* __LEPL_NVRAM_H__ */
