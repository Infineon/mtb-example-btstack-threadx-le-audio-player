/*
 * $ Copyright Cypress Semiconductor $
 */

/* Application includes */
#include "lepl.h"

extern wiced_bt_cfg_ble_t lepl_ble_cfg;
extern wiced_bt_heap_t *p_lea_default_heap;

const char *app_get_gatt_state_str(lepl_clcb_t *p_state);

lepl_gatt_cb_t g_lepl_gatt_cb = {0};

const uint8_t lepl_gatt_database[] = {
    /* Primary Service 'Generic Attribute' */
    PRIMARY_SERVICE_UUID16(HDLS_GATT_GENERIC_ATTRIBUTE_SERVICE, UUID_SERVICE_GATT),

    // Service Changed Characteristic
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED,
                          HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED_VALUE,
                          GATT_UUID_GATT_SRV_CHGD,
                          GATTDB_CHAR_PROP_INDICATE,
                          GATTDB_PERM_NONE),
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED_CLIENT_CONFIGURATION,
                                    GATT_UUID_CHAR_CLIENT_CONFIG,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ),
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_DB_HASH,
                          HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_DB_HASH_VALUE,
                          GATT_UUID_GATT_DATABASE_HASH,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CLIENT_FEATURES,
                                   HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CLIENT_FEATURES_VALUE,
                                   GATT_UUID_CLIENT_SUPPORTED_FEATURES,
                                   (GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE),
                                   (GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ)),
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES,
                          HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES_VALUE,
                          GATT_UUID_SERVER_SUPPORTED_FEATURES,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),


    /* Primary Service 'Generic Access' */
    PRIMARY_SERVICE_UUID16(HDLS_GATT_GENERIC_ACCESS_SERVICE, UUID_SERVICE_GAP),

    /* Characteristic 'Device Name' */
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME,
                          HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME_VALUE,
                          UUID_CHARACTERISTIC_DEVICE_NAME,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),

    /* Characteristic 'Appearance' */
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ACCESS_APPEARANCE,
                          HDLC_GATT_GENERIC_ACCESS_APPEARANCE_VALUE,
                          UUID_CHARACTERISTIC_APPEARANCE,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),
    // Declare characteristic for preferred connection parameters
    CHARACTERISTIC_UUID16(HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM,
                          HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM_VALUE,
                          GATT_UUID_GAP_PREF_CONN_PARAM,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),

#if (ENABLE_MCS == 1)
    /* Primary Service 'MCS' */
    PRIMARY_SERVICE_UUID16(HDLS_MCS, WICED_BT_UUID_MEDIA_CONTROL),

    /* Characteristic 'Media Player Name' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_PLAYER_NAME,
                          HDLC_MCS_MEDIA_PLAYER_NAME_VALUE,
                          WICED_BT_UUID_MEDIA_PLAYER_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_PLAYER_NAME_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Changed' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_TRACK_CHANGED,
                          HDLC_MCS_MEDIA_TRACK_CHANGED_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_CHANGED,
                          GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_NONE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_TRACK_CHANGED_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Title' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_TRACK_TITLE,
                          HDLC_MCS_MEDIA_TRACK_TITLE_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_TITLE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_TRACK_TITLE_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Duration' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_TRACK_DURATION,
                          HDLC_MCS_MEDIA_TRACK_DURATION_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_DURATION,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_TRACK_DURATION_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Position' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_MCS_MEDIA_TRACK_POSITION,
                                   HDLC_MCS_MEDIA_TRACK_POSITION_VALUE,
                                   WICED_BT_UUID_MEDIA_TRACK_POSITION,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_TRACK_POSITION_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),
#if 0
    /* Characteristic 'Playback Speed' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_MCS_PLAYBACK_SPEED,
                                   HDLC_MCS_PLAYBACK_SPEED_VALUE,
                                   WICED_BT_UUID_MEDIA_PLAYBACK_SPEED,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_MCS_PLAYBACK_SPEED_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE
                                    | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Seeking Speed' */
    CHARACTERISTIC_UUID16(HDLC_MCS_SEEKING_SPEED,
                          HDLC_MCS_SEEKING_SPEED_VALUE,
                          WICED_BT_UUID_MEDIA_SEEKING_SPEED,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_MCS_SEEKING_SPEED_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Playing Order' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_MCS_PLAYING_ORDER,
                                   HDLC_MCS_PLAYING_ORDER_VALUE,
                                   WICED_BT_UUID_MEDIA_PLAYING_ORDER,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_MCS_PLAYING_ORDER_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE ),

    /* Characteristic 'Playing Order Supported' */
    CHARACTERISTIC_UUID16(HDLC_MCS_PLAYING_ORDER_SUPPORTED,
                          HDLC_MCS_PLAYING_ORDER_SUPPORTED_VALUE,
                          WICED_BT_UUID_MEDIA_PLAYING_ORDER_SUPPORTED,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
#endif

    /* Characteristic 'Media State' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_STATE,
                          HDLC_MCS_MEDIA_STATE_VALUE,
                          WICED_BT_UUID_MEDIA_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_MEDIA_STATE_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Media Control Point ' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_MCS_MEDIA_CONTROL_POINT,
                                   HDLC_MCS_MEDIA_CONTROL_POINT_VALUE,
                                   WICED_BT_UUID_MEDIA_CONTROL_POINT,
                                   GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_CONTROL_POINT_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Media Control Point Opcode supported' */
    CHARACTERISTIC_UUID16(HDLC_MCS_MEDIA_CONTROL_POINT_OPCODE_SUPPORTED,
                          HDLC_MCS_MEDIA_CONTROL_POINT_OPCODE_SUPPORTED_VALUE,
                          WICED_BT_UUID_MEDIA_CONTROL_OPCODE_SUPPORTED,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_MCS_CONTROL_POINT_OPCODE_SUPPORTED_DESCRIPTION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'Content Control ID (CCID)' */
    CHARACTERISTIC_UUID16(HDLC_MCS_CONTENT_CONTROL_ID,
                          HDLC_MCS_CONTENT_CONTROL_ID_VALUE,
                          WICED_BT_UUID_MEDIA_CONTENT_CONTROL_ID,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
#endif // ENABLE_MCS == 1

    /* Primary Service 'GMCS' */
    PRIMARY_SERVICE_UUID16(HDLS_GMCS, WICED_BT_UUID_GENERIC_MEDIA_CONTROL),

    /* Characteristic 'Player Name' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_PLAYER_NAME,
                          HDLC_GMCS_PLAYER_NAME_VALUE,
                          WICED_BT_UUID_MEDIA_PLAYER_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_PLAYER_NAME_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Changed' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_TRACK_CHANGED,
                          HDLC_GMCS_TRACK_CHANGED_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_CHANGED,
                          GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_NONE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_TRACK_CHANGED_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Title' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_TRACK_TITLE,
                          HDLC_GMCS_TRACK_TITLE_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_TITLE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_TRACK_TITLE_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Duration' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_TRACK_DURATION,
                          HDLC_GMCS_TRACK_DURATION_VALUE,
                          WICED_BT_UUID_MEDIA_TRACK_DURATION,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_TRACK_DURATION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Track Position' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GMCS_TRACK_POSITION,
                                   HDLC_GMCS_TRACK_POSITION_VALUE,
                                   WICED_BT_UUID_MEDIA_TRACK_POSITION,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY |
                                       GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_TRACK_POSITION_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

#if 0
    /* Characteristic 'Playback Speed' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GMCS_PLAYBACK_SPEED,
                                   HDLC_GMCS_PLAYBACK_SPEED_VALUE,
                                   WICED_BT_UUID_MEDIA_PLAYBACK_SPEED,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY |
                                       GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_GMCS_PLAYBACK_SPEED_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE ),

    /* Characteristic 'Seeking Speed' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_SEEKING_SPEED,
                          HDLC_GMCS_SEEKING_SPEED_VALUE,
                          WICED_BT_UUID_MEDIA_SEEKING_SPEED,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE  | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_GMCS_SEEKING_SPEED_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE ),

    /* Characteristic 'Playing Order' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GMCS_PLAYING_ORDER,
                                   HDLC_GMCS_PLAYING_ORDER_VALUE,
                                   WICED_BT_UUID_MEDIA_PLAYING_ORDER,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_NOTIFY |
                                       GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(
                                    HDLD_GMCS_PLAYING_ORDER_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE ),

    /* Characteristic 'Playing Order Supported' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_PLAYING_ORDER_SUPPORTED,
                          HDLC_GMCS_PLAYING_ORDER_SUPPORTED_VALUE,
                          WICED_BT_UUID_MEDIA_PLAYING_ORDER_SUPPORTED,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
#endif

    /* Characteristic 'Media State' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_MEDIA_STATE,
                          HDLC_GMCS_MEDIA_STATE_VALUE,
                          WICED_BT_UUID_MEDIA_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_MEDIA_STATE_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Media Control Point ' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GMCS_CONTROL_POINT,
                                   HDLC_GMCS_CONTROL_POINT_VALUE,
                                   WICED_BT_UUID_MEDIA_CONTROL_POINT,
                                   GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_WRITABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_CONTROL_POINT_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Media Control Point Opcode supported' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_CONTROL_POINT_OPCODE_SUPPORTED,
                          HDLC_GMCS_CONTROL_POINT_OPCODE_SUPPORTED_VALUE,
                          WICED_BT_UUID_MEDIA_CONTROL_OPCODE_SUPPORTED,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GMCS_CONTROL_POINT_OPCODE_SUPPORTED_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'Content Control ID (CCID)' */
    CHARACTERISTIC_UUID16(HDLC_GMCS_CONTENT_CONTROL_ID,
                          HDLC_GMCS_CONTENT_CONTROL_ID_VALUE,
                          WICED_BT_UUID_MEDIA_CONTENT_CONTROL_ID,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

#if (ENABLE_TBS == 1)

    /* Primary Service 'generic telephone_bearer_service' */
    PRIMARY_SERVICE_UUID16(HDLS_TBS, WICED_BT_UUID_TELEPHONE_BEARER),

    /* Characteristic 'bearer_provider_name' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_PROVIDER_NAME,
                          HDLC_TBS_BEARER_PROVIDER_NAME_VALUE,
                          WICED_BT_UUID_TBS_BEARER_PROVIDER_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_BEARER_PROVIDER_NAME_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'bearer_UCI' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_UCI,
                          HDLC_TBS_BEARER_UCI_VALUE,
                          WICED_BT_UUID_TBS_BEARER_UCI,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'bearer_technology' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_TECHNOLOGY,
                          HDLC_TBS_BEARER_TECHNOLOGY_VALUE,
                          WICED_BT_UUID_TBS_BEARER_TECHNOLOGY,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_BEARER_TECHNOLOGY_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'bearer_URI_prefix' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_URI_SCHEMES,
                          HDLC_TBS_BEARER_URI_SCHEMES_VALUE,
                          WICED_BT_UUID_TBS_BEARER_URI_SCHEMES,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_BEARER_URI_SCHEMES_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'bearer_signal_strength' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_SIGNAL_STRENGTH,
                          HDLC_TBS_BEARER_SIGNAL_STRENGTH_VALUE,
                          WICED_BT_UUID_TBS_BEARER_SIGNAL_STRENGTH,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_BEARER_SIGNAL_STRENGTH_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'bearer_sig_str_reporting_interval' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_TBS_BEARER_SIG_STR_REPORTING_INTERVAL,
                                   HDLC_TBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE,
                                   WICED_BT_UUID_TBS_SIG_STR_REPORTING_INTERVAL,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ |
                                       GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_AUTH_WRITABLE),

    /* Characteristic 'bearer_list_current_call' */
    CHARACTERISTIC_UUID16(HDLC_TBS_BEARER_LIST_CURRENT_CALL,
                          HDLC_TBS_BEARER_LIST_CURRENT_CALL_VALUE,
                          WICED_BT_UUID_TBS_LIST_CURRENT_CALL,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_BEARER_LIST_CURRENT_CALL_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'content_control_ID' */
    CHARACTERISTIC_UUID16(HDLC_TBS_CONTENT_CONTROL_ID,
                          HDLC_TBS_CONTENT_CONTROL_ID_VALUE,
                          WICED_BT_UUID_TBS_CONTENT_CONTROL_ID,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'status_flags' */
    CHARACTERISTIC_UUID16(HDLC_TBS_STATUS_FLAG,
                          HDLC_TBS_STATUS_FLAG_VALUE,
                          WICED_BT_UUID_TBS_STATUS_FLAGS,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_STATUS_FLAG_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'incoming_tg_caller_ID' */
    CHARACTERISTIC_UUID16(HDLC_TBS_INCOMING_TG_URI,
                          HDLC_TBS_INCOMING_TG_URI_VALUE,
                          WICED_BT_UUID_TBS_INCOMING_TG_URI,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_INCOMING_TG_CALLER_ID_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'call_state' */
    CHARACTERISTIC_UUID16(HDLC_TBS_CALL_STATE,
                          HDLC_TBS_CALL_STATE_VALUE,
                          WICED_BT_UUID_TBS_CALL_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_CALL_STATE_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_WRITE_CMD |
                                        GATTDB_PERM_AUTH_WRITABLE | GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'call_control_point' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_TBS_CALL_CONTROL_POINT,
                                   HDLC_TBS_CALL_CONTROL_POINT_VALUE,
                                   WICED_BT_UUID_TBS_CALL_CONTROL_POINT,
                                   GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_WRITE_CMD | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_CALL_CONTROL_POINT_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'call control point optional opcodes' */
    CHARACTERISTIC_UUID16(HDLC_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,
                          HDLC_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE_VALUE,
                          WICED_BT_UUID_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'termination_reason' */
    CHARACTERISTIC_UUID16(HDLC_TBS_TERMINATION_REASON,
                          HDLC_TBS_TERMINATION_REASON_VALUE,
                          WICED_BT_UUID_TBS_TERMINATION_REASON,
                          GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_NONE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_TERMINATION_REASON_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'incoming call' */
    CHARACTERISTIC_UUID16(HDLC_TBS_INCOMING_CALL,
                          HDLC_TBS_INCOMING_CALL_VALUE,
                          WICED_BT_UUID_TBS_INCOMING_CALL,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_INCOMING_CALL_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),

    /* Characteristic 'incoming call friendly name' */
    CHARACTERISTIC_UUID16(HDLC_TBS_CALL_FRIENDLY_NAME,
                          HDLC_TBS_CALL_FRIENDLY_NAME_VALUE,
                          WICED_BT_UUID_TBS_CALL_FRIENDLY_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_TBS_CALL_FRIENDLY_NAME_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_WRITE_REQ | GATTDB_PERM_AUTH_WRITABLE |
                                        GATTDB_PERM_WRITE_CMD),
#endif // (ENABLE_TBS == 1)

#define ENABLE_GTBS 1
#if (ENABLE_GTBS == 1)
    /* Primary Service 'generic telephone_bearer_service' */
    PRIMARY_SERVICE_UUID16(HDLS_GTBS, WICED_BT_UUID_GENERIC_TELEPHONE_BEARER),

    /* Characteristic 'bearer_provider_name' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_PROVIDER_NAME,
                          HDLC_GTBS_BEARER_PROVIDER_NAME_VALUE,
                          WICED_BT_UUID_TBS_BEARER_PROVIDER_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_BEARER_PROVIDER_NAME_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'bearer_UCI' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_UCI,
                          HDLC_GTBS_BEARER_UCI_VALUE,
                          WICED_BT_UUID_TBS_BEARER_UCI,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'bearer_technology' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_TECHNOLOGY,
                          HDLC_GTBS_BEARER_TECHNOLOGY_VALUE,
                          WICED_BT_UUID_TBS_BEARER_TECHNOLOGY,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_BEARER_TECHNOLOGY_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'bearer_URI_prefix' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_URI_SCHEMES,
                          HDLC_GTBS_BEARER_URI_SCHEMES_VALUE,
                          WICED_BT_UUID_TBS_BEARER_URI_SCHEMES,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_BEARER_URI_SCHEMES_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

#if 0
    /* Characteristic 'bearer_signal_strength' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_SIGNAL_STRENGTH,
                          HDLC_GTBS_BEARER_SIGNAL_STRENGTH_VALUE,
                          WICED_BT_UUID_TBS_BEARER_SIGNAL_STRENGTH,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_BEARER_SIGNAL_STRENGTH_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),


    /* Characteristic 'bearer_sig_str_reporting_interval' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL,
                                   HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE,
                                   WICED_BT_UUID_TBS_SIG_STR_REPORTING_INTERVAL,
                                   GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE,
                                   GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
#endif

    /* Characteristic 'bearer_list_current_call' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_BEARER_LIST_CURRENT_CALL,
                          HDLC_GTBS_BEARER_LIST_CURRENT_CALL_VALUE,
                          WICED_BT_UUID_TBS_LIST_CURRENT_CALL,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_BEARER_LIST_CURRENT_CALL_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'content_control_ID' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_CONTENT_CONTROL_ID,
                          HDLC_GTBS_CONTENT_CONTROL_ID_VALUE,
                          WICED_BT_UUID_TBS_CONTENT_CONTROL_ID,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'status_flags' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_STATUS_FLAG,
                          HDLC_GTBS_STATUS_FLAG_VALUE,
                          WICED_BT_UUID_TBS_STATUS_FLAGS,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_STATUS_FLAG_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

#if 0
    /* Characteristic 'incoming_tg_caller_ID' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_INCOMING_TG_URI,
                          HDLC_GTBS_INCOMING_TG_URI_VALUE,
                          WICED_BT_UUID_TBS_INCOMING_TG_URI,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_INCOMING_TG_CALLER_ID_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),
#endif

    /* Characteristic 'call_state' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_CALL_STATE,
                          HDLC_GTBS_CALL_STATE_VALUE,
                          WICED_BT_UUID_TBS_CALL_STATE,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_CALL_STATE_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'call_control_point' */
    CHARACTERISTIC_UUID16_WRITABLE(HDLC_GTBS_CALL_CONTROL_POINT,
                                   HDLC_GTBS_CALL_CONTROL_POINT_VALUE,
                                   WICED_BT_UUID_TBS_CALL_CONTROL_POINT,
                                   GATTDB_CHAR_PROP_WRITE | GATTDB_CHAR_PROP_WRITE_NO_RESPONSE |
                                       GATTDB_CHAR_PROP_NOTIFY,
                                   GATTDB_PERM_WRITABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_CALL_CONTROL_POINT_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'call control point optional opcodes' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,
                          HDLC_GTBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE_VALUE,
                          WICED_BT_UUID_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic 'termination_reason' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_TERMINATION_REASON,
                          HDLC_GTBS_TERMINATION_REASON_VALUE,
                          WICED_BT_UUID_TBS_TERMINATION_REASON,
                          GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_NONE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_TERMINATION_REASON_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'incoming call' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_INCOMING_CALL,
                          HDLC_GTBS_INCOMING_CALL_VALUE,
                          WICED_BT_UUID_TBS_INCOMING_CALL,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_INCOMING_CALL_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

    /* Characteristic 'incoming call friendly name' */
    CHARACTERISTIC_UUID16(HDLC_GTBS_CALL_FRIENDLY_NAME,
                          HDLC_GTBS_CALL_FRIENDLY_NAME_VALUE,
                          WICED_BT_UUID_TBS_CALL_FRIENDLY_NAME,
                          GATTDB_CHAR_PROP_READ | GATTDB_CHAR_PROP_NOTIFY,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Descriptor 'Client Characteristic Configuration' */
    CHAR_DESCRIPTOR_UUID16_WRITABLE(HDLD_GTBS_CALL_FRIENDLY_NAME_CLIENT_CONFIGURATION,
                                    UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                    GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE | GATTDB_PERM_WRITABLE),

#if TMAP_ENABLED
    /* Primary Service 'telephone_bearer_service' */
    PRIMARY_SERVICE_UUID16(HDLS_TMAP, WICED_BT_UUID_TMAS),

    /* Characteristic 'bearer_provider_name' */
    CHARACTERISTIC_UUID16(HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE,
                          HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE_VALUE,
                          WICED_BT_UUID_TMAP_ROLE,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
#endif
#endif // (ENABLE_GTBS == 1)

#if GMAP_SERVER_ENABLED
    /* Primary Service Gaming Audio Service */
    PRIMARY_SERVICE_UUID16(HDLS_GMAP, WICED_BT_UUID_GAMING_AUDIO_SERVICE),

    /* Characteristic GMAP Role */
    CHARACTERISTIC_UUID16(HDLS_GMAP_ROLE,
                          HDLS_GMAP_ROLE_VALUE,
                          WICED_BT_UUID_GMAP_ROLE,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic GMAP UGG Features */
    CHARACTERISTIC_UUID16(HDLS_GMAP_UGG_FEATURES,
                          HDLS_GMAP_UGG_FEATURES_VALUE,
                          WICED_BT_UUID_GMAP_UGG_FEATURES,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),

    /* Characteristic GMAP BGS Features */
    CHARACTERISTIC_UUID16(HDLS_GMAP_BGS_FEATURES,
                          HDLS_GMAP_BGS_FEATURES_VALUE,
                          WICED_BT_UUID_GMAP_BGS_FEATURES,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
#endif

            /* Primary Service: Device Information */
    PRIMARY_SERVICE_UUID16(HDLS_DIS, UUID_SERVICE_DEVICE_INFORMATION),

    /* Characteristic: Manufacturer Name String */
    CHARACTERISTIC_UUID16(HDLC_DIS_MANUFACTURER_NAME_STRING,
                          HDLC_DIS_MANUFACTURER_NAME_STRING_VALUE,
                          UUID_CHARACTERISTIC_MANUFACTURER_NAME_STRING,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),
    /* Characteristic: Model Number String */
    CHARACTERISTIC_UUID16(HDLC_DIS_MODEL_NUMBER_STRING,
                          HDLC_DIS_MODEL_NUMBER_STRING_VALUE,
                          UUID_CHARACTERISTIC_MODEL_NUMBER_STRING,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE),
    /* Characteristic: Firmware Revision String */
    CHARACTERISTIC_UUID16(HDLC_DIS_FIRMWARE_REVISION_STRING,
                          HDLC_DIS_FIRMWARE_REVISION_STRING_VALUE,
                          UUID_CHARACTERISTIC_FIRMWARE_REVISION_STRING,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE),
    /* Characteristic: Software Revision String */
    CHARACTERISTIC_UUID16(HDLC_DIS_SOFTWARE_REVISION_STRING,
                          HDLC_DIS_SOFTWARE_REVISION_STRING_VALUE,
                          UUID_CHARACTERISTIC_SOFTWARE_REVISION_STRING,
                          GATTDB_CHAR_PROP_READ,
                          GATTDB_PERM_READABLE | GATTDB_PERM_AUTH_READABLE)

};

void lepl_set_current_ble_activity(lepl_ble_activity_state_t curr_activity)
{
    g_lepl_gatt_cb.ble_activity = curr_activity;
    WICED_BT_TRACE("[%s]  %d", __FUNCTION__, curr_activity);
}

lepl_ble_activity_state_t lepl_get_current_ble_activity(void)
{
    return g_lepl_gatt_cb.ble_activity;
}

static void lepl_gatt_scan_cb(wiced_ble_ext_scan_results_t *p_scr, uint16_t adv_len, uint8_t *p_adv_data)
{
    uint8_t *p_data;
    uint16_t adv_entry_length = 0;
    uint16_t uuid;
    wiced_bt_device_address_t peer_addr;

    if (p_scr == NULL)
    {
        return;
    }

    WICED_BT_TRACE("[%s] p_scan_result : %B", __FUNCTION__, p_scr->remote_bd_addr);

    // Return if device is already connected.
    // This can happen when there are multiple advertisements from same device.
    for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
    {
        if (g_lepl_gatt_cb.clcb[i].in_use &&
            ((memcmp(g_lepl_gatt_cb.clcb[i].identity_bd_address, p_scr->remote_bd_addr, BD_ADDR_LEN) == 0) ||
             (memcmp(g_lepl_gatt_cb.clcb[i].conn_addr, p_scr->remote_bd_addr, BD_ADDR_LEN) == 0)))
        {
            WICED_BT_TRACE("[%s] device %B ,already connected", __FUNCTION__, p_scr->remote_bd_addr);
            return;
        }
    }

    if (g_lepl_gatt_cb.enable_uuid_filter == 1)
    {
        wiced_bt_adv_ctx_t ctx = {.adv_len = adv_len, .p_adv = p_adv_data};
        p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, &adv_entry_length);
        WICED_BT_TRACE("[%s] offset %d length %d", __FUNCTION__, ctx.offset, adv_entry_length);
        if (p_data == NULL || adv_entry_length == 0)
        {
            return;
        }

        while (adv_entry_length) /*if no entry is found adv_entry_length will be 0*/
        {
            STREAM_TO_UINT16(uuid, p_data);
            WICED_BT_TRACE("[%s] uuid found %x", __FUNCTION__, uuid);
            memcpy(&peer_addr, p_scr->remote_bd_addr, BD_ADDR_LEN);
            if (uuid == WICED_BT_UUID_AUDIO_STREAM_CONTROL)
            {
                WICED_BT_TRACE("[%s] found ASCS device", __FUNCTION__);
                app_rpc_send_scan_res_event(p_scr, p_adv_data);
                return;
            }
            else
            {
                p_data = wiced_ble_adv_data_search(&ctx, BTM_BLE_ADVERT_TYPE_SERVICE_DATA, &adv_entry_length);
            }
        }
    }
    else
    {
        app_rpc_send_scan_res_event(p_scr, p_adv_data);
        WICED_BT_TRACE("[%s] filter disabled", __FUNCTION__);
    }
}

wiced_ble_ext_scan_params_t scan_params = {.own_addr_type = BLE_ADDR_PUBLIC,
                                           .scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT,
                                           .scan_filter_policy = WICED_BLE_SCAN_BASIC_UNFILTERED_SP,
                                           .sp_1m.scan_type = BTM_BLE_SCAN_MODE_ACTIVE,
                                           .sp_1m.scan_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_INTERVAL,
                                           .sp_1m.scan_window = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_WINDOW};

wiced_ble_ext_scan_enable_params_t scan_enable_params = {.filter_duplicates = 0, .scan_period = 0, .scan_duration = 0};

wiced_result_t lepl_start_stop_scan(uint32_t start, wiced_ble_ext_scan_result_cback_t *p_cback)
{
    wiced_result_t status = WICED_BT_SUCCESS;

    if (start)
    {
        status = wiced_ble_ext_scan_register_cb(p_cback);
        wiced_ble_ext_scan_configure_reassembly(255, 2);
        status = wiced_ble_ext_scan_set_params(&scan_params);
    }
    if (status == WICED_BT_SUCCESS)
    {
        wiced_ble_ext_scan_enable_params_t enable = scan_enable_params;
        status = wiced_ble_ext_scan_enable(start, &enable);
        lepl_set_current_ble_activity(start ? LEPL_BLE_ACTIVITY_SCANNING : LEPL_BLE_ACTIVITY_NONE);
    }

    return status;
}

wiced_result_t lepl_gatt_start_stop_scan(uint32_t start, uint8_t enable_uuid_filter)
{
    if (start && (lepl_get_current_ble_activity() != LEPL_BLE_ACTIVITY_NONE))
    {
        WICED_BT_TRACE("[%s] in progress", __FUNCTION__);
        return WICED_BT_ERROR;
    }
    g_lepl_gatt_cb.enable_uuid_filter = enable_uuid_filter;
    wiced_result_t status = lepl_start_stop_scan(start, lepl_gatt_scan_cb);

    return status;
}

const char *app_get_gatt_state_str(lepl_clcb_t *p_clcb)
{
    if (p_clcb->disconnecting)
    {
        return "disconnecting";
    }
    else if (p_clcb->ready)
    {
        return "ready";
    }
    else if (p_clcb->read_characteristics)
    {
        return "read_characteristics";
    }
    else if (p_clcb->enabled_notifications)
    {
        return "enabled_notifications";
    }
    else if (p_clcb->discovery_complete)
    {
        return "discovery_complete";
    }
    else if (p_clcb->mtu_exchanged)
    {
        return "mtu_exchanged";
    }
    else if (p_clcb->connected)
    {
        return "connected";
    }
    else
    {
        return "disconnected";
    }
}

static void lepl_print_gatt_state(lepl_clcb_t *p_clcb)
{
    WICED_BT_TRACE("[%s] %B new %s", __FUNCTION__, p_clcb->identity_bd_address, app_get_gatt_state_str(p_clcb));
}

void lepl_set_gatt_state_connection_sts(lepl_clcb_t *p_clcb, uint8_t connected)
{
    p_clcb->connected = connected;

    lepl_print_gatt_state(p_clcb);
}

void lepl_set_gatt_state_mtu_exchanged(lepl_clcb_t *p_clcb)
{
    p_clcb->mtu_exchanged = 1;
    lepl_print_gatt_state(p_clcb);
}

void lepl_set_gatt_state_discovery_complete(lepl_clcb_t *p_clcb)
{
    p_clcb->discovery_complete = 1;
    lepl_print_gatt_state(p_clcb);
}

void lepl_set_gatt_state_enabled_notifications(lepl_clcb_t *p_clcb)
{
    p_clcb->enabled_notifications = 1;
    p_clcb->ready = p_clcb->enabled_notifications & p_clcb->read_characteristics;
    lepl_print_gatt_state(p_clcb);
}

void lepl_set_gatt_state_read_characteristics(lepl_clcb_t *p_clcb)
{
    p_clcb->read_characteristics = 1;
    p_clcb->ready = p_clcb->enabled_notifications & p_clcb->read_characteristics;
    lepl_print_gatt_state(p_clcb);
}

lepl_clcb_t *lepl_gatt_alloc_cb(uint8_t *p_bd_addr,
                                wiced_bt_ble_address_type_t addr_type,
                                uint16_t conn_id,
                                uint16_t link_role)
{
    int index;
    lepl_clcb_t *p_clcb = NULL;
    for (index = 0; index < LEPL_MAX_CONNECTIONS; index++)
    {
        p_clcb = &g_lepl_gatt_cb.clcb[index];
        if (p_clcb->in_use == FALSE)
        {
            p_clcb->in_use = TRUE;
            p_clcb->conn_id = conn_id;
            p_clcb->addr_type = addr_type;
            p_clcb->identity_address_type = addr_type;
            memcpy(p_clcb->identity_bd_address, p_bd_addr, BD_ADDR_LEN);
            memcpy(p_clcb->conn_addr, p_bd_addr, BD_ADDR_LEN);
            p_clcb->b_is_central = (HCI_ROLE_CENTRAL == link_role) ? TRUE : FALSE;
            if (conn_id == 0)
            {
                lepl_set_gatt_state_connection_sts(p_clcb, 0);
            }
            else
            {
                lepl_set_gatt_state_connection_sts(p_clcb, 1);
            }
            return p_clcb;
        }
    }
    return p_clcb;
}

lepl_clcb_t *lepl_gatt_get_clcb(uint8_t *p_bd_addr)
{
    lepl_clcb_t *p_clcb = NULL;
    int index;
    for (index = 0; index < LEPL_MAX_CONNECTIONS; index++)
    {
        p_clcb = &g_lepl_gatt_cb.clcb[index];
        if (p_clcb->in_use && !WICED_MEMCMP(p_clcb->conn_addr, p_bd_addr, BD_ADDR_LEN))
        {
            return p_clcb;
        }
    }
    return NULL;
}

void lepl_gatt_free_discovery_ctx(lepl_clcb_t *p_clcb)
{
    if (p_clcb->p_discovery_ctx)
    {
        gatt_intf_free_service_discovery_ctx(p_clcb->p_discovery_ctx);
        p_clcb->p_discovery_ctx = NULL;
    }
}

wiced_bt_gatt_status_t lepl_gatt_free_cb(uint8_t *p_bd_addr)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb(p_bd_addr);
    if (!p_clcb)
        return WICED_ERROR;

    lepl_gatt_free_discovery_ctx(p_clcb);

    WICED_MEMSET(p_clcb, 0, sizeof(lepl_clcb_t));
    return WICED_SUCCESS;
}

lepl_clcb_t *lepl_gatt_get_clcb_by_conn_id(uint16_t conn_id)
{
    lepl_clcb_t *p_clcb = NULL;
    int index;
    for (index = 0; index < LEPL_MAX_CONNECTIONS; index++)
    {
        p_clcb = &g_lepl_gatt_cb.clcb[index];
        if (p_clcb->in_use && (p_clcb->conn_id == conn_id))
        {
            return p_clcb;
        }
    }
    return NULL;
}

lepl_clcb_t *lepl_gatt_get_clcb_by_conn_handle(uint16_t acl_conn_handle)
{
    lepl_clcb_t *p_clcb = NULL;
    int index;
    for (index = 0; index < LEPL_MAX_CONNECTIONS; index++)
    {
        p_clcb = &g_lepl_gatt_cb.clcb[index];
        if (p_clcb->in_use && (p_clcb->acl_handle == acl_conn_handle))
        {
            return p_clcb;
        }
    }
    return NULL;
}

/* Read the device and return the nvram id */
uint16_t lepl_read_device_from_nvram(lepl_clcb_t *p_clcb)
{
    lepl_nvram_paired_device_key_t *p_pdkeys = lepl_nvram_get_paired_device_key_info(p_clcb->identity_bd_address);
    if (p_pdkeys == NULL)
    {
        WICED_BT_TRACE("[%s] No paired device found for %B", __FUNCTION__, p_clcb->identity_bd_address);
        return 0;
    }
    lepl_print_nvram_data("read_from_nvram", p_pdkeys);

    WICED_MEMCPY(&p_clcb->peer_profiles, &p_pdkeys->peer_profiles, sizeof(lepl_peer_profiles_t));
    WICED_MEMCPY(&p_clcb->db_hash, &p_pdkeys->db_hash, sizeof(wiced_bt_db_hash_t));
    WICED_MEMCPY(p_clcb->cccd_bits, p_pdkeys->stored_cccd_bits, sizeof(p_clcb->cccd_bits));

    wiced_bt_device_address_t null_addr = {0};
    if ( WICED_MEMCMP(p_pdkeys->csis_pair_device.bda, null_addr, BD_ADDR_LEN) != 0 &&
         lepl_gatt_get_clcb(p_pdkeys->csis_pair_device.bda) == NULL)
    {
        wiced_bt_ble_address_t *p_addr = &p_pdkeys->csis_pair_device;
        if (lepl_get_current_ble_activity() == LEPL_BLE_ACTIVITY_SCANNING)
        {
            lepl_start_stop_scan(0, NULL);
        }
        app_create_connection(p_addr->type, p_addr->bda);
    }
    return lepl_nvram_get_nvram_id(p_pdkeys);
}


static void lepl_gatt_handle_connection(wiced_bt_gatt_connection_status_t *p_conn_sts)
{
    lepl_clcb_t *p_clcb =
        lepl_gatt_alloc_cb(p_conn_sts->bd_addr, p_conn_sts->addr_type, p_conn_sts->conn_id, p_conn_sts->link_role);

    WICED_BT_TRACE("[%s] connected to [%B] clcb 0x%x reason 0x%x\n",
                   __FUNCTION__,
                   p_conn_sts->bd_addr,
                   p_clcb,
                   p_conn_sts->reason);

    lepl_set_current_ble_activity(LEPL_BLE_ACTIVITY_NONE);
    WICED_MEMSET(g_lepl_gatt_cb.connecting_peer_addr, 0, BD_ADDR_LEN);

    /* Allocate GATT control block */
    if (!p_clcb)
    {
        // Assert !!
        return;
    }

    p_clcb->acl_handle = wiced_bt_gatt_get_acl_conn_handle(p_clcb->conn_id);
    app_rpc_send_app_status(p_conn_sts->conn_id, p_conn_sts->bd_addr, HCI_CONTROL_MISC_APP_STATE_CONNECTED, 0);

    /* Configure MTU */
    wiced_bt_gatt_status_t status =
        wiced_bt_gatt_client_configure_mtu(p_conn_sts->conn_id, lepl_ble_cfg.ble_max_rx_pdu_size);

    WICED_BT_TRACE("[%s] configure mtu result %d\n", __FUNCTION__, status);

    lepl_read_device_from_nvram(p_clcb);

    /* Inform CC */
    app_rpc_send_connect_event(p_conn_sts);
}

void lepl_gatt_handle_disconnection(wiced_bt_gatt_connection_status_t *p_conn_sts)
{

    if (WICED_MEMCMP(p_conn_sts->bd_addr, g_lepl_gatt_cb.connecting_peer_addr, BD_ADDR_LEN) == 0)
    {
        WICED_MEMSET(g_lepl_gatt_cb.connecting_peer_addr, 0, BD_ADDR_LEN);
        lepl_set_current_ble_activity(LEPL_BLE_ACTIVITY_NONE);
    }

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb(p_conn_sts->bd_addr);
    if (!p_clcb)
    {
        app_rpc_send_app_status(p_conn_sts->conn_id, p_conn_sts->bd_addr, HCI_CONTROL_MISC_APP_STATE_DISCONNECTED, 0);
        app_rpc_send_disconnect_evt(p_conn_sts);
        return;
    }

    app_rpc_send_app_status(p_conn_sts->conn_id, p_clcb->identity_bd_address, HCI_CONTROL_MISC_APP_STATE_DISCONNECTED, 0);
    app_rpc_send_disconnect_evt(p_conn_sts);
    lepl_save_device_data_to_nvram(p_clcb);

    if (p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle)
    {
        lepl_csis_handle_gatt_disconnection(p_conn_sts->bd_addr);
    }
    else
    {
        lepl_cap_handle_set_disconnection();
    }
    lepl_cap_cleanup_ases_for_acl_handle(p_clcb->acl_handle);
    lepl_gatt_free_cb(p_conn_sts->bd_addr);

    WICED_BT_TRACE("[%s] disconnected from [%B]\n", __FUNCTION__, p_conn_sts->bd_addr);
}

const gatt_intf_cccd_map_t lepl_cccd_map[] = {
    {CCCD_MAP(GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED)},
#if (ENABLE_MCS == 1)
    {CCCD_MAP(MCS_PLAYER_NAME)},
    {CCCD_MAP(MCS_TRACK_CHANGED)},
    {CCCD_MAP(MCS_TRACK_TITLE)},
    {CCCD_MAP(MCS_TRACK_DURATION)},
    {CCCD_MAP(MCS_TRACK_POSITION)},
    {CCCD_MAP(MCS_PLAYBACK_SPEED)},
    {CCCD_MAP(MCS_SEEKING_SPEED)},
    {CCCD_MAP(MCS_PLAYING_ORDER)},
    {CCCD_MAP(MCS_MEDIA_STATE)},
    {CCCD_MAP(MCS_CONTROL_POINT)},
    {CCCD_MAP(MCS_CONTROL_POINT_OPCODE_SUPPORTED)},
#endif // ENABLE_MCS == 1

    {CCCD_MAP(GMCS_PLAYER_NAME)},
    {CCCD_MAP(GMCS_TRACK_CHANGED)},
    {CCCD_MAP(GMCS_TRACK_TITLE)},
    {CCCD_MAP(GMCS_TRACK_DURATION)},
    {CCCD_MAP(GMCS_TRACK_POSITION)},
    {CCCD_MAP(GMCS_PLAYBACK_SPEED)},
    {CCCD_MAP(GMCS_SEEKING_SPEED)},
    {CCCD_MAP(GMCS_PLAYING_ORDER)},
    {CCCD_MAP(GMCS_MEDIA_STATE)},
    {CCCD_MAP(GMCS_CONTROL_POINT)},
    {CCCD_MAP(GMCS_CONTROL_POINT_OPCODE_SUPPORTED)},

#if (ENABLE_TBS == 1)
    {CCCD_MAP(TBS_BEARER_PROVIDER_NAME_DESCRIPTION)},
    {CCCD_MAP(TBS_BEARER_TECHNOLOGY_DESCRIPTION)},
    {CCCD_MAP(TBS_BEARER_URI_SCHEMES_DESCRIPTION)},
    {CCCD_MAP(TBS_BEARER_SIGNAL_STRENGTH_DESCRIPTION)},
    {CCCD_MAP(TBS_BEARER_LIST_CURRENT_CALL_DESCRIPTION)},
    {CCCD_MAP(TBS_STATUS_FLAG_DESCRIPTION)},
    {CCCD_MAP(TBS_INCOMING_TG_CALLER_ID_DESCRIPTION)},
    {CCCD_MAP(TBS_CALL_STATE_DESCRIPTION)},
    {CCCD_MAP(TBS_CALL_CONTROL_POINT_DESCRIPTION)},
    {CCCD_MAP(TBS_TERMINATION_REASON_DESCRIPTION)},
    {CCCD_MAP(TBS_INCOMING_CALL_DESCRIPTION)},
    {CCCD_MAP(TBS_CALL_FRIENDLY_NAME_DESCRIPTION)},
#endif // ENABLE_TBS = 1

    {CCCD_MAP(GTBS_BEARER_PROVIDER_NAME)},
    {CCCD_MAP(GTBS_BEARER_TECHNOLOGY)},
    {CCCD_MAP(GTBS_BEARER_URI_SCHEMES)},
    {CCCD_MAP(GTBS_BEARER_SIGNAL_STRENGTH)},
    {CCCD_MAP(GTBS_BEARER_LIST_CURRENT_CALL)},
    {CCCD_MAP(GTBS_INCOMING_TG_URI)},
    {CCCD_MAP(GTBS_STATUS_FLAG)},
    {CCCD_MAP(GTBS_CALL_STATE)},
    {CCCD_MAP(GTBS_CALL_CONTROL_POINT)},
    {CCCD_MAP(GTBS_TERMINATION_REASON)},
    {CCCD_MAP(GTBS_INCOMING_CALL)},
    {CCCD_MAP(GTBS_CALL_FRIENDLY_NAME)},
};

/* C99-compatible compile-time assertion */
#define compile_time_assert(expr, line) typedef char compile_time_assertion_##line##_fail[(expr) ? 1 : -1]

compile_time_assert(sizeof(lepl_cccd_map) / sizeof(lepl_cccd_map[0]) == LEPL_MAX_CCCD_TO_STORE, __LINE__);


int lepl_get_cccd_index( uint16_t handle, uint8_t type)
{
    int index = sizeof(lepl_cccd_map) / sizeof(lepl_cccd_map[0]);
    const gatt_intf_cccd_map_t *p_map = lepl_cccd_map;
    while (index--)
    {
        if (p_map->handle[type] == handle)
        {
            return index = (p_map - lepl_cccd_map);
        }
        p_map++;
    }
    return -1;
}

uint16_t lepl_get_clcb_cccd(lepl_clcb_t *p_clcb, uint16_t handle, gatt_intf_cccd_map_handle_type_t type)
{
    int index = lepl_get_cccd_index(handle, type);
    if (index == -1)
    {
        return 0;
    }

    int byte_index = (index * 2) / 32;
    int byte_offset = (index * 2) % 32;

    WICED_BT_TRACE("[%s] handle %d cccd[%d] = (0x%x >> %d) & 3 = 0x%x",
                   __FUNCTION__,
                   handle,
                   byte_index,
                   p_clcb->cccd_bits[byte_index],
                   byte_offset,
                   (p_clcb->cccd_bits[byte_index] >> byte_offset) & 0x3);

    return (p_clcb->cccd_bits[byte_index] >> byte_offset) & 0x3;
}

void lepl_set_clcb_cccd(lepl_clcb_t *p_clcb, uint16_t handle, uint16_t value)
{
    int index = lepl_get_cccd_index(handle, GATT_INTF_DESCRIPTOR_HANDLE_INDEX);
    if (index == -1)
    {
        return;
    }

    int byte_index = (index * 2) / 32;
    int byte_offset = (index * 2) % 32;

    {
        uint32_t current_value = p_clcb->cccd_bits[byte_index];
        uint32_t final_value = (current_value & ~(0x3 << byte_offset)) | ((value & 0x3) << byte_offset);
        WICED_BT_TRACE("[%s] handle %d cccd[%d] = (0x%x | %d << %d) = 0x%x",
                       __FUNCTION__,
                       handle,
                       byte_index,
                       current_value,
                       value & 0x3,
                       byte_offset,
                       final_value);
    }

    p_clcb->cccd_bits[byte_index] &= ~(0x3 << byte_offset);
    p_clcb->cccd_bits[byte_index] |= (value & 0x3) << byte_offset;
    return;
}


uint16_t lepl_handle_get_cccd_value_cb(uint16_t conn_id, uint16_t handle)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (!p_clcb)
        return 0;
    return lepl_get_clcb_cccd(p_clcb, handle, GATT_INTF_VALUE_HANDLE_INDEX);
}

wiced_bt_gatt_status_t lepl_handle_tmap_gmap_read_request(lepl_clcb_t *p_clcb,
                                                          uint16_t handle,
                                                          uint8_t *p_data,
                                                          uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t *p_data_start = p_data;

    switch (handle)
    {
    case HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE_VALUE: // 0x0262, 610
    {
        UINT16_TO_STREAM(p_data, LEPL_TMAP_ROLE);
    }
    break;

#if GMAP_SERVER_ENABLED
    case HDLS_GMAP_ROLE_VALUE: // 0x0272, 626
    {
        UINT8_TO_STREAM(p_data, LEPL_GMAP_ROLE);
    }
    break;
    case HDLS_GMAP_UGG_FEATURES_VALUE: // 0x0274, 628
    {
        UINT8_TO_STREAM(p_data, LEPL_GMAP_UGG_FEATURES);
    }
    break;
    case HDLS_GMAP_BGS_FEATURES_VALUE: // 0x0276, 630
    {
        UINT8_TO_STREAM(p_data, LEPL_GMAP_BGS_FEATURES);
    }
    break;
#endif
    default:
        status = WICED_BT_GATT_INVALID_HANDLE;
        break;
    }

    *p_len = p_data - p_data_start;
    return status;
}

/* Set to media player appearance */
#define LEPL_GENERIC_ACCESS_APPEARANCE (0xA << 6) /* Media Player Tag */

wiced_bt_gatt_status_t lepl_generic_access_handle_read_request(lepl_clcb_t *p_clcb,
                                                          uint16_t handle,
                                                          uint8_t *p_data,
                                                          uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t *p_data_start = p_data;

    switch (handle)
    {
        case HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME_VALUE:
        {
            uint32_t name_len = strlen((const char *)lepl_cfg_settings.device_name);
            int to_send = MIN(*p_len, name_len);
            memcpy(p_data, lepl_cfg_settings.device_name, to_send);
            p_data += to_send;
        }
        break;
        case HDLC_GATT_GENERIC_ACCESS_APPEARANCE_VALUE:
        {
            UINT16_TO_STREAM(p_data, LEPL_GENERIC_ACCESS_APPEARANCE);
        }
        break;
        case HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM_VALUE:
        {
            UINT16_TO_STREAM(p_data, lepl_cfg_settings.p_ble_cfg->p_ble_scan_cfg->conn_min_interval);
            UINT16_TO_STREAM(p_data, lepl_cfg_settings.p_ble_cfg->p_ble_scan_cfg->conn_max_interval);
            UINT16_TO_STREAM(p_data, lepl_cfg_settings.p_ble_cfg->p_ble_scan_cfg->conn_latency);
            UINT16_TO_STREAM(p_data, lepl_cfg_settings.p_ble_cfg->p_ble_scan_cfg->conn_supervision_timeout);
        }
        break;
        default:
            break;
    }
    *p_len = p_data - p_data_start;
    return status;
}

#define LEPL_DEVICE_MANUFACTURER_NAME "Infineon"
#define LEPL_DEVICE_MODEL_NUMBER "LEPL-REF"
#define LEPL_DEVICE_FIRMWARE_REVISION "1.0.0"
#define LEPL_DEVICE_SOFTWARE_REVISION "1.0.0"

uint8_t lepl_read_string_into_buffer(uint8_t *p_data, uint16_t *p_len, const char *string)
{
    uint32_t name_len = strlen((const char *)string);
    int to_send = MIN(*p_len, name_len);
    memcpy(p_data, string, to_send);
    return to_send;
}

wiced_bt_gatt_status_t lepl_handle_device_info_read_request(lepl_clcb_t *p_clcb,
                                                            uint16_t handle,
                                                            uint8_t *p_data,
                                                            uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t *p_data_start = p_data;
    switch (handle)
    {
    case HDLC_DIS_MANUFACTURER_NAME_STRING_VALUE:
        p_data += lepl_read_string_into_buffer(p_data, p_len, LEPL_DEVICE_MANUFACTURER_NAME);
        break;
    case HDLC_DIS_MODEL_NUMBER_STRING_VALUE:
        p_data += lepl_read_string_into_buffer(p_data, p_len, LEPL_DEVICE_MODEL_NUMBER);
        break;
    case HDLC_DIS_FIRMWARE_REVISION_STRING_VALUE:
        p_data += lepl_read_string_into_buffer(p_data, p_len, LEPL_DEVICE_FIRMWARE_REVISION);
        break;
    case HDLC_DIS_SOFTWARE_REVISION_STRING_VALUE:
        p_data += lepl_read_string_into_buffer(p_data, p_len, LEPL_DEVICE_SOFTWARE_REVISION);
        break;
        default:
            break;
    }
    *p_len = p_data - p_data_start;
    return status;
}

wiced_bt_gatt_status_t lepl_generic_attribute_handle_read_request(lepl_clcb_t *p_clcb,
                                                            uint16_t handle,
                                                            uint8_t *p_data,
                                                            uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t *p_data_start = p_data;
    switch (handle)
    {
    case HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_DB_HASH_VALUE:
    {
        ARRAY_TO_STREAM(p_data, g_lepl_gatt_cb.db_hash, sizeof(g_lepl_gatt_cb.db_hash));
    }break;
    case HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CLIENT_FEATURES_VALUE:
    {
        ARRAY_TO_STREAM(p_data, p_clcb->csf, sizeof(p_clcb->csf));
    }break;
    case HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES_VALUE:
    {
        UINT8_TO_STREAM(p_data, 0); // No server features supported
    }break;
    default:
        break;
    }
    *p_len = p_data - p_data_start;
    return status;
}


wiced_bt_gatt_status_t app_handle_gatt_read_request(lepl_clcb_t *p_clcb,
                                                    uint16_t handle,
                                                    uint8_t *p_data,
                                                    uint16_t *p_len)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    int cccd_index = lepl_get_cccd_index(handle, GATT_INTF_DESCRIPTOR_HANDLE_INDEX);
    const uint8_t *p_data_start = p_data;

    WICED_BT_TRACE("[%s] %d", __FUNCTION__, handle);

    if (cccd_index != -1)
    {
        uint16_t cccd = lepl_get_clcb_cccd(p_clcb, handle, GATT_INTF_DESCRIPTOR_HANDLE_INDEX);
        UINT16_TO_STREAM(p_data, cccd);
        *p_len = p_data - p_data_start;
        return WICED_BT_GATT_SUCCESS;
    }

    if (handle >= HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED_VALUE &&
        handle <= HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES_VALUE)
    {
        status = lepl_generic_attribute_handle_read_request(p_clcb, handle, p_data, p_len);
    }
    else  if (handle >= HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME_VALUE &&
        handle <= HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM_VALUE)
    {
        status = lepl_generic_access_handle_read_request(p_clcb, handle, p_data, p_len);
    }
    else if (handle >= HDLC_GMCS_PLAYER_NAME_VALUE && handle <= HDLC_GMCS_CONTENT_CONTROL_ID_VALUE)
    {
        status = lepl_mcs_handle_read_request(p_clcb, handle, p_data, p_len);
    }
    else if (handle >= HDLC_GTBS_BEARER_PROVIDER_NAME_VALUE && handle <= HDLC_GTBS_CALL_FRIENDLY_NAME_VALUE)
    {
        status = lepl_tbs_handle_gatt_read_request(p_clcb, handle, p_data, p_len);
    }
    else if (handle >= HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE_VALUE &&
             handle <= HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE_VALUE)
    {
        status = lepl_handle_tmap_gmap_read_request(p_clcb, handle, p_data, p_len);
    }
    else if (handle >= HDLC_DIS_MANUFACTURER_NAME_STRING_VALUE && handle <= HDLC_DIS_SOFTWARE_REVISION_STRING_VALUE)
    {
         status = lepl_handle_device_info_read_request(p_clcb, handle, p_data, p_len);
    }

    return status;
}

wiced_bt_gatt_status_t app_handle_gatt_write_request(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_write_req_t *p_write_req)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    int cccd_index = lepl_get_cccd_index(p_write_req->handle, GATT_INTF_DESCRIPTOR_HANDLE_INDEX);
    uint8_t *p_data = p_write_req->p_val;
    lepl_local_service_data_t *p_local = &g_lepl_gatt_cb.local_service_data;
    int len_to_write = p_write_req->val_len;

    if (cccd_index != -1)
    {
        if (len_to_write != 2)
        {
            return WICED_BT_GATT_INVALID_ATTR_LEN;
        }

        uint16_t cccd;
        STREAM_TO_UINT16(cccd, p_data);
        lepl_set_clcb_cccd(p_clcb, p_write_req->handle, cccd);
        return WICED_BT_GATT_SUCCESS;
    }

    switch (p_write_req->handle)
    {
    case HDLC_GMCS_TRACK_POSITION_VALUE:
    {
        if (len_to_write != 4)
        {
            return WICED_BT_GATT_INVALID_ATTR_LEN;
        }
        STREAM_TO_UINT32(p_local->gmcs.track_position, p_data);
    }
    break;
    case HDLC_GMCS_PLAYBACK_SPEED_VALUE:
    {
        if (len_to_write != 2)
        {
            return WICED_BT_GATT_INVALID_ATTR_LEN;
        }
        if (!p_local->gmcs.track_selected)
            return WICED_BT_GATT_ERROR;

        STREAM_TO_UINT16(p_local->gmcs.playback_speed, p_data);
    }
    break;
    case HDLC_GMCS_PLAYING_ORDER_VALUE:
    {
        uint8_t playing_order;
        if (len_to_write != 1)
        {
            return WICED_BT_GATT_INVALID_ATTR_LEN;
        }
        STREAM_TO_UINT8(playing_order, p_data);
        if ((playing_order < GA_LIB_MCS_SINGLE_ONCE) || (playing_order > GA_LIB_MCS_SHUFFLE_REPEAT))
        {
            status = WICED_BT_GATT_INVALID_PDU;
        }
        else
        {
            p_local->gmcs.playing_order = playing_order;
        }
    }
    break;
    case HDLC_GMCS_CONTROL_POINT_VALUE:
    {
        status = lepl_gmcs_handle_write_cp(conn_id, p_data, len_to_write);
    }
    break;

    case HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE:
        break;
    case HDLC_GTBS_CALL_CONTROL_POINT_VALUE:
        status = lepl_gtbs_handle_write_cp(conn_id, p_data, len_to_write);
        break;
    default:
        break;
    }

    return status;
}

wiced_bt_gatt_status_t app_handle_gatt_read_by_type_request(lepl_clcb_t *p_clcb,
                                                            wiced_bt_gatt_attribute_request_t *p_att_req, uint16_t *p_err_handle)
{
    wiced_bt_gatt_read_by_type_t *p_read_req = &p_att_req->data.read_by_type;
    uint16_t attr_handle = p_read_req->s_handle;
    uint8_t value_len = 0;
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t *p_data = gatt_intf_method_get_buffer(p_att_req->len_requested);
    int used = 0;

    WICED_BT_TRACE("[%s] code 0x%x 0x%x - 0x%x uuid %A len %d",
                   __FUNCTION__,
                   p_att_req->opcode,
                   p_read_req->s_handle,
                   p_read_req->e_handle,
                   &p_read_req->uuid.uu,
                   p_read_req->uuid.len,
                   p_att_req->len_requested);
    if (!p_data)
    {

        return WICED_BT_GATT_NO_RESOURCES;
    }
    /* Read by type returns all attributes of the specified type, between the start and end handles */
    while (1)
    {
        uint16_t attr_len = 0;
        uint8_t attr[512]; // stack buffer set to the largest size

        attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle, p_read_req->e_handle, &p_read_req->uuid);

        if (attr_handle == 0)
            break;

        status = app_handle_gatt_read_request(p_clcb, attr_handle, attr, &attr_len);
        if (status != WICED_BT_GATT_SUCCESS)
        {
            gatt_intf_method_free_buffer(p_data);
            *p_err_handle = attr_handle;
            return status;
        }

        {
            int filled = wiced_bt_gatt_put_read_by_type_rsp_in_stream(p_data + used,
                                                                      p_att_req->len_requested - used,
                                                                      &value_len,
                                                                      attr_handle,
                                                                      attr_len,
                                                                      attr);
            if (filled == 0)
            {
                break;
            }
            used += filled;
        }

        /* Increment starting handle for next search to one past current */
        attr_handle++;
    }

    if (used)
    {
        status = wiced_bt_gatt_server_send_read_by_type_rsp(p_att_req->conn_id,
                                                            p_att_req->opcode,
                                                            value_len,
                                                            used,
                                                            p_data,
                                                            (wiced_bt_gatt_app_context_t)gatt_intf_method_free_buffer);
        if (WICED_BT_GATT_SUCCESS != status)
        {
            gatt_intf_method_free_buffer(p_data);
        }
    }
    else
    {
        status = WICED_BT_GATT_ATTRIBUTE_NOT_FOUND;
        gatt_intf_method_free_buffer(p_data);
    }

    return status;
}

wiced_bt_gatt_status_t app_handle_gatt_attribute_request(wiced_bt_gatt_attribute_request_t *p_att_req, uint16_t *p_err_handle)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(p_att_req->conn_id);

    WICED_BT_TRACE("[%s] opcode %d", __FUNCTION__, p_att_req->opcode);

    if (!p_clcb)
    {
        return WICED_BT_GATT_ERROR;
    }
    *p_err_handle = 0;
    switch (p_att_req->opcode)
    {
    case GATT_REQ_MTU:
        /* Exchange MTU Request */
        {
            int my_mtu = MIN(p_att_req->data.remote_mtu, lepl_ble_cfg.ble_max_rx_pdu_size);
            wiced_bt_gatt_server_send_mtu_rsp(p_att_req->conn_id, p_att_req->data.remote_mtu, my_mtu);
        }
        break;
    case GATT_REQ_READ:
    case GATT_REQ_READ_BLOB:
    {
        uint8_t buf[512];
        /* Read Request and Read Blob Request are handled in the common gatt handler after invoking
         * the profile specific handler
         */
        uint16_t read_len = sizeof(buf);
        status = app_handle_gatt_read_request(p_clcb, p_att_req->data.read_req.handle, buf, &read_len);

        *p_err_handle = p_att_req->data.read_req.handle;
        if (status == WICED_BT_GATT_SUCCESS)
        {
            gatt_intf_send_read_response(p_att_req, status, buf, read_len);
        }
    }
    break;
    case GATT_REQ_WRITE:
    case GATT_CMD_WRITE:
    {
        status = app_handle_gatt_write_request(p_att_req->conn_id, p_clcb, &p_att_req->data.write_req);
        *p_err_handle = p_att_req->data.write_req.handle;
        if (status == WICED_BT_GATT_SUCCESS && (p_att_req->opcode == GATT_REQ_WRITE))
        {
            wiced_bt_gatt_server_send_write_rsp(p_att_req->conn_id,
                                                p_att_req->opcode,
                                                p_att_req->data.write_req.handle);
        }
    }
    break;
    case GATT_HANDLE_VALUE_IND:
    {
        // Handle Value Indication is handled in the common gatt handler after invoking the profile specific handler
        // Send queued packets if any
    }
    break;
    case GATT_REQ_READ_BY_TYPE:
    {
        // Read By Type Request is handled in the common gatt handler after invoking the profile specific handler
        status = app_handle_gatt_read_by_type_request(p_clcb, p_att_req, p_err_handle);

    }break;
    default:
        break;
    }
    return status;
}

void lepl_app_sec_bond(lepl_clcb_t *p_clcb)
{
    wiced_bt_device_link_keys_t dev_link_keys;
    memcpy(dev_link_keys.bd_addr, p_clcb->conn_addr, BD_ADDR_LEN);
    WICED_BT_TRACE("[%s] for %B\n", __FUNCTION__, p_clcb->conn_addr);
    if (lepl_nvram_read_keys(&dev_link_keys))
    {
        wiced_bt_ble_sec_action_type_t encryption_type = BTM_BLE_SEC_ENCRYPT;
        wiced_bt_dev_set_encryption(p_clcb->conn_addr, BT_TRANSPORT_LE, &encryption_type);
    }
    else
    {
        wiced_bt_dev_sec_bond(p_clcb->conn_addr, p_clcb->addr_type, BT_TRANSPORT_LE, 0, NULL);
    }
}

typedef struct
{
    gatt_intf_service_range_t *p_service;
    void (*app_handle_characteristics_value)(uint16_t conn_id,
                                             lepl_clcb_t *p_clcb,
                                             wiced_bt_gatt_optype_t op,
                                             wiced_bt_gatt_status_t status,
                                             wiced_bt_gatt_data_t *p_gatt_data);
} app_handle_char_val_t;

void app_handle_received_characteristics_value(uint16_t conn_id,
                                               lepl_clcb_t *p_clcb,
                                               wiced_bt_gatt_optype_t op,
                                               wiced_bt_gatt_status_t status,
                                               wiced_bt_gatt_data_t *p_gatt_data)
{
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    const app_handle_char_val_t handle_char_val[] = {
        {&p_peer->service_handles[LEPL_PACS], lepl_pacs_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_ASCS], lepl_ascs_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_VCS], lepl_vcs_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_CSIS], lepl_csis_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_MICS], lepl_mics_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_HAS], lepl_has_handle_received_characteristics_value},
        //{&p_peer->service_handles[LEPL_IAS], lepl_ias_handle_read_complete},
        {&p_peer->service_handles[LEPL_GMAP], lepl_gmap_handle_received_characteristics_value},
        {&p_peer->service_handles[LEPL_MICS_AICS], lepl_mics_aics_handle_received_characteristics_value},
    };
    const app_handle_char_val_t *p_cmplt = handle_char_val;
    int limit = sizeof(handle_char_val) / sizeof(handle_char_val[0]);

    WICED_BT_TRACE("[%s] conn_id %d hdl %d sts 0x%x", __FUNCTION__, conn_id, p_gatt_data->handle, status);

    while (limit--)
    {
        const gatt_intf_service_range_t *p_service = p_cmplt->p_service;
        if ((p_service->start_handle <= p_gatt_data->handle) && (p_gatt_data->handle <= p_service->end_handle))
        {
            p_cmplt->app_handle_characteristics_value(conn_id, p_clcb, op, status, p_gatt_data);
        }
        p_cmplt++;
    }

    return;
}

#if 0
wiced_result_t app_handle_notification_evt(uint16_t conn_id, lepl_clcb_t *p_clcb, wiced_bt_gatt_data_t *p_gatt_data)
{
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    uint8_t *p_read_buf = p_gatt_data->p_data;
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    WICED_BT_TRACE("[%s] hdl %d", __FUNCTION__, p_gatt_data->handle);

    if (p_peer->gmcs_service.start_handle <= p_gatt_data->handle &&
        p_peer->gmcs_service.end_handle >= p_gatt_data->handle)
    {
        gmcs_app_handle_read_complete(p_clcb, status, p_gatt_data);
    }
    else if (p_peer->gtbs_service.start_handle <= p_gatt_data->handle &&
             p_peer->gtbs_service.end_handle >= p_gatt_data->handle)
    {
        gtbs_app_handle_read_complete(p_clcb, status, p_gatt_data);
    }
    else if (p_peer->gmap_service.start_handle <= p_gatt_data->handle &&
             p_peer->gmap_service.end_handle >= p_gatt_data->handle)
    {
        gmap_app_handle_read_complete(p_clcb, status, p_gatt_data);
    }
    return WICED_SUCCESS;
}
#endif

void app_handle_gatt_operation_complete(wiced_bt_gatt_operation_complete_t *p_op_cplt)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(p_op_cplt->conn_id);
    WICED_BT_TRACE("[%s] op %d clcb 0x%x p_op 0x%x",
                   __FUNCTION__,
                   p_op_cplt->op,
                   p_clcb,
                   p_clcb ? p_clcb->p_op : NULL);

    if (!p_clcb)
    {
        return;
    }

    if (GATTC_OPTYPE_CONFIG_MTU == p_op_cplt->op)
    {
        lepl_set_gatt_state_mtu_exchanged(p_clcb);

        /* TODO: if bonded, get GATT Db info from NVRAM and start encrption, else start bonding*/
        if (p_clcb->b_is_central)
        {
            lepl_app_sec_bond(p_clcb);
        }
    }
    else if (GATTC_OPTYPE_READ_HANDLE == p_op_cplt->op)
    {
        /* Read response is handled in the common gatt handler after invoking the profile specific handler */
        app_handle_received_characteristics_value(p_op_cplt->conn_id,
                                                  p_clcb,
                                                  p_op_cplt->op,
                                                  p_op_cplt->status,
                                                  &p_op_cplt->response_data.att_value);

        if (p_op_cplt->response_data.att_value.p_data)
        {
            gatt_intf_method_free_buffer(p_op_cplt->response_data.att_value.p_data);
        }
    }
    else if (GATTC_OPTYPE_WRITE_NO_RSP == p_op_cplt->op || GATTC_OPTYPE_WRITE_WITH_RSP == p_op_cplt->op)
    {
        /* Write response is handled in the common gatt handler after invoking the profile specific handler */
    }
    else if ((GATTC_OPTYPE_NOTIFICATION == p_op_cplt->op) || (GATTC_OPTYPE_INDICATION == p_op_cplt->op))
    {
        /* Notification confirmation is handled in the common gatt handler after invoking the profile specific handler */
        app_handle_received_characteristics_value(p_op_cplt->conn_id,
                                                  p_clcb,
                                                  p_op_cplt->op,
                                                  p_op_cplt->status,
                                                  &p_op_cplt->response_data.att_value);

        if (GATTC_OPTYPE_INDICATION == p_op_cplt->op)
        {
            /* Indication confirmation is handled in the common gatt handler after invoking the profile specific handler */
            wiced_bt_gatt_client_send_indication_confirm(p_op_cplt->conn_id, p_op_cplt->response_data.handle);
        }
    }
    else
    {
        WICED_BT_TRACE("[%s] unhandled op %d", __FUNCTION__, p_op_cplt->op);
    }

    if (p_clcb->p_op)
    {
        gatt_intf_handle_gatt_operation_complete(p_op_cplt, p_clcb->p_op);
    }
}

static char *gatt_event_name[] = {
    "GATT_CONNECTION_STATUS_EVT",      /* 0 */
    "GATT_OPERATION_CPLT_EVT",         /* 1 */
    "GATT_DISCOVERY_RESULT_EVT",       /* 2 */
    "GATT_DISCOVERY_CPLT_EVT",         /* 3 */
    "GATT_ATTRIBUTE_REQUEST_EVT",      /* 4 */
    "GATT_CONGESTION_EVT",             /* 5 */
    "GATT_GET_RESPONSE_BUFFER_EVT",    /* 6 */
    "GATT_APP_BUFFER_TRANSMITTED_EVT", /* 7 */
};

char * gatt_event_name_by_code(uint8_t code)
{
    if (code < sizeof(gatt_event_name) / sizeof(gatt_event_name[0]))
    {
        return gatt_event_name[code];
    }
    return "unknown";
}

wiced_bt_gatt_status_t lepl_gatt_cback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_ed)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    WICED_BT_TRACE("[%s] event [0x%x, %s] max_heap %d\n",
                   __FUNCTION__,
                   event,
                   gatt_event_name_by_code(event),
                   wiced_bt_get_largest_heap_buffer(p_lea_default_heap));

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
    {
        if (p_ed->connection_status.connected && (p_ed->connection_status.reason == 0))
        {
            lepl_gatt_handle_connection(&p_ed->connection_status);
        }
        else
        {
            lepl_gatt_handle_disconnection(&p_ed->connection_status);
        }
    }
    break;
    case GATT_OPERATION_CPLT_EVT:
    {
        app_handle_gatt_operation_complete(&p_ed->operation_complete);
    }
    break;
    case GATT_DISCOVERY_CPLT_EVT:
    {
        lepl_clcb_t *p_clcb = NULL;
        p_clcb = lepl_gatt_get_clcb_by_conn_id(p_ed->operation_complete.conn_id);

        if (p_clcb && p_clcb->p_discovery_ctx)
        {
            gatt_intf_on_service_discovery_complete(p_clcb->p_discovery_ctx, &p_ed->discovery_complete);

            if (p_clcb->p_discovery_ctx)
            {
                if (gatt_intf_is_service_discovery_complete(p_clcb->p_discovery_ctx))
                {
                    gatt_intf_free_service_discovery_ctx(p_clcb->p_discovery_ctx);
                    p_clcb->p_discovery_ctx = NULL;
                }
            }
        }
    }
    break;
    case GATT_DISCOVERY_RESULT_EVT:
    {
        lepl_clcb_t *p_clcb = NULL;
        p_clcb = lepl_gatt_get_clcb_by_conn_id(p_ed->operation_complete.conn_id);
        WICED_BT_TRACE("[%s]", gatt_event_name[event]);
        p_clcb = lepl_gatt_get_clcb_by_conn_id(p_ed->operation_complete.conn_id);
        if (p_clcb && p_clcb->p_discovery_ctx)
        {
            gatt_intf_on_service_discovery_result(p_clcb->p_discovery_ctx, &p_ed->discovery_result);
        }
    }
    break;
    case GATT_ATTRIBUTE_REQUEST_EVT:
    {
        uint16_t err_handle = 0;
        wiced_bt_gatt_attribute_request_t *p_req = &p_ed->attribute_request;

        status = app_handle_gatt_attribute_request(p_req, &err_handle);
        if (status != WICED_BT_GATT_SUCCESS)
        {
            int send_err_rsp = 1;

            switch (p_req->opcode)
            {
            case GATT_HANDLE_VALUE_NOTIF:
            case GATT_HANDLE_VALUE_IND:
            case GATT_HANDLE_VALUE_CONF:
            case GATT_CMD_WRITE:
            case GATT_CMD_SIGNED_WRITE:
                send_err_rsp = 0;
                break;
            default:
                break;
            }
            if (send_err_rsp)
            {
                wiced_bt_gatt_server_send_error_rsp(p_req->conn_id, p_req->opcode, err_handle, status);
            }
        }
    }
    break;
    case GATT_CONGESTION_EVT:
    {
        lepl_clcb_t *p_clcb = p_clcb = g_lepl_gatt_cb.clcb;
        for (int i = 0; i < LEPL_MAX_CONNECTIONS; i++)
        {
            if (p_clcb->in_use && p_clcb->p_op)
            {
                gatt_intf_execute_handle_operations(p_clcb->p_op);
                WICED_BT_TRACE("[%s] conn_id %d congested %d",
                               __FUNCTION__,
                               p_ed->congestion.conn_id,
                               p_ed->congestion.congested);
                break;
            }
        }
    }
    break;
    case GATT_GET_RESPONSE_BUFFER_EVT:
    {
        wiced_bt_gatt_buffer_request_t *p_req = &p_ed->buffer_request;
        if (p_req->len_requested)
        {
            p_req->buffer.p_app_rsp_buffer = gatt_intf_method_get_buffer(p_req->len_requested);
            p_req->buffer.p_app_ctxt = (wiced_bt_gatt_app_context_t)gatt_intf_method_free_buffer;
        }

        if (!p_req->buffer.p_app_rsp_buffer)
            GATT_INTERFACE_TRACE_CRIT("[%s] get ptr %x ctx %x len %d",
                                      __FUNCTION__,
                                      p_req->buffer.p_app_rsp_buffer,
                                      p_req->buffer.p_app_ctxt,
                                      p_req->len_requested);
    }
    break;
    case GATT_APP_BUFFER_TRANSMITTED_EVT:
    {
        void (*pfn_free)(uint8_t *ptr) = (void (*)(uint8_t *))p_ed->buffer_xmitted.p_app_ctxt;

        if (pfn_free && p_ed->buffer_xmitted.p_app_data)
        {
            pfn_free(p_ed->buffer_xmitted.p_app_data);
        }
        GATT_INTERFACE_TRACE("[%s] free ptr %x ctx %x",
                             __FUNCTION__,
                             p_ed->buffer_xmitted.p_app_data,
                             p_ed->buffer_xmitted.p_app_ctxt);
    }
    break;

    default:
        WICED_BT_TRACE("Unknown event [0x%x]", event);
        break;
    }

    return status;
}

void lepl_free_discovery_ctx(lepl_clcb_t *p_clcb)
{
    if (p_clcb->p_discovery_ctx)
    {
        gatt_intf_free_service_discovery_ctx(p_clcb->p_discovery_ctx);
        p_clcb->p_discovery_ctx = NULL;
    }
}

void lepl_cache_discovery_results_and_cleanup(lepl_clcb_t *p_clcb,
                                              gatt_intf_discovery_result_t *p_result,
                                              gatt_intf_service_range_t *p_service_range,
                                              gatt_intf_characteristic_handles_t *p_handles)
{
    gatt_intf_characteristic_handles_result_t *p_char_handles = p_result->p_char_handles;
    gatt_intf_print_handles(p_result);

    memcpy(p_service_range, &p_result->service_range, sizeof(gatt_intf_service_range_t));
    for (int i = 0; i < p_result->max_characteristic_handles; i++, p_handles++, p_char_handles++)
    {
        memcpy(p_handles, &p_char_handles->handles, sizeof(gatt_intf_characteristic_handles_t));
    }

    lepl_gatt_free_discovery_ctx(p_clcb);
}

void on_read_remote_cmpl(uint16_t conn_id, wiced_bt_gatt_status_t status, void *p_user_data)
{
    lepl_clcb_t *p_clcb = (lepl_clcb_t *)p_user_data;

    WICED_BT_TRACE("[%s] conn_id %d status 0x%x", __FUNCTION__, conn_id, status);

    {
        gatt_intf_free_operation_handle_list(p_clcb->p_op);
        p_clcb->p_op = NULL;
    }

    lepl_set_gatt_state_read_characteristics(p_clcb);
    app_rpc_send_app_status(conn_id, p_clcb->identity_bd_address, HCI_CONTROL_MISC_APP_STATE_READY, status);

    if (status == WICED_BT_GATT_SUCCESS )
    {
        wiced_bt_db_hash_t null_db_hash = {0};

        if (memcmp(p_clcb->db_hash, null_db_hash, sizeof(wiced_bt_db_hash_t)) == 0)
        {
            p_clcb->db_hash[0] = 1; // set a non-zero value to indicate that the db hash is valid
        }
    }

    if (p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle)
    {
        lepl_csis_handle_gatt_connection(conn_id);
    }
}

wiced_result_t lepl_read_remote_characteristics(lepl_clcb_t *p_clcb)
{
    uint8_t vcs_read_chars[] = {GA_LIB_VCS_CHARACTERISTIC_VOLUME_STATE};
    uint8_t mics_read_chars[] = {GA_LIB_MICS_CHARACTERISTIC_MUTE_STATE};
    uint8_t has_read_chars[] = {
        GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_FEATURES,           //0
        GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_ACTIVE_PRESET_INDEX //2
    };
    uint8_t mics_aics_read_chars[] = {
        GA_LIB_AICS_CHARACTERISTIC_INPUT_STATE,             //0
        GA_LIB_AICS_CHARACTERISTIC_GAIN_SETTING_PROPERTIES, //1
        GA_LIB_AICS_CHARACTERISTIC_INPUT_STATUS,            //3
        GA_LIB_AICS_CHARACTERISTIC_INPUT_DESCRIPTION        //5
    };
    //uint8_t bass_read_chars[] = {};
    uint8_t num_ascs_chars = LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS;
    uint8_t num_pacs_chars =
        GA_LIB_PACS_CHARACTERISTIC_MAX_UNIQUE + LEPL_PACS_MAX_SNK_CHARACTERISTICS + LEPL_PACS_MAX_SRC_CHARACTERISTICS;
    uint8_t num_csis_chars = GA_LIB_CSIS_CHARACTERISTIC_MAX;
    uint8_t num_vcs_chars = sizeof(vcs_read_chars) / sizeof(vcs_read_chars[0]);
    uint8_t num_mics_chars = sizeof(mics_read_chars) / sizeof(mics_read_chars[0]);
    uint8_t num_has_chars = sizeof(has_read_chars) / sizeof(has_read_chars[0]);
    uint8_t num_mics_aics_chars = sizeof(mics_aics_read_chars) / sizeof(mics_aics_read_chars[0]);
    uint8_t num_bass_chars = LEPL_MAX_BASS_RCV_STATE_SUPPORTED;

    uint8_t total_chars = num_ascs_chars + num_pacs_chars + num_csis_chars + num_vcs_chars + num_mics_chars +
                          num_has_chars + num_mics_aics_chars + num_bass_chars;

    p_clcb->p_op = gatt_intf_alloc_read_handle_list(p_clcb->conn_id, total_chars, on_read_remote_cmpl, p_clcb);

    if (p_clcb->p_op == NULL)
    {
        return WICED_BT_NO_RESOURCES;
    }

    int pos = 0;
    for (int i = 0; i < num_ascs_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char =
            &p_clcb->peer_profiles.ascs[i + GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }
    for (int i = 0; i < num_pacs_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.pacs[i];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }
    for (int i = 0; i < num_csis_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.csis[i];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }

    for (int i = 0; i < num_vcs_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.vcs[vcs_read_chars[i]];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }
    for (int i = 0; i < num_mics_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.mics[mics_read_chars[i]];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }
    for (int i = 0; i < num_has_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.has[has_read_chars[i]];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }

    for (int i = 0; i < num_mics_aics_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char = &p_clcb->peer_profiles.mics_aics[0][mics_aics_read_chars[i]];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }
    for (int i = 0; i < num_bass_chars; i++)
    {
        gatt_intf_characteristic_handles_t *p_char =
            &p_clcb->peer_profiles.bass[i + GA_LIB_BASS_CHARACTERISTIC_MAX_UNIQUE];
        if (p_char->value_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, p_char, pos++);
        }
    }

    app_rpc_send_app_status(p_clcb->conn_id,
                        p_clcb->identity_bd_address,
                        HCI_CONTROL_MISC_APP_STATE_INITING,
                        HCI_CONTROL_MISC_APP_STATE_INIT_READING);
    return gatt_intf_execute_handle_operations(p_clcb->p_op);
}

void on_enable_notification_cmpl(uint16_t conn_id, wiced_bt_gatt_status_t status, void *p_ctx)
{
    lepl_clcb_t *p_clcb = (lepl_clcb_t *)p_ctx;

    WICED_BT_TRACE("[%s] conn_id %d status 0x%x", __FUNCTION__, conn_id, status);

    if (p_clcb->p_op)
    {
        gatt_intf_free_operation_handle_list(p_clcb->p_op);
        p_clcb->p_op = NULL;
    }

    lepl_set_gatt_state_enabled_notifications(p_clcb);
    lepl_read_remote_characteristics(p_clcb);
}

wiced_result_t lepl_enable_app_notifications(lepl_clcb_t *p_clcb)
{
    uint8_t ascs_chars[] = {GA_LIB_ASCS_CHARACTERISTIC_ASE_CONTROL_POINT};
    uint8_t pacs_chars[] = {GA_LIB_PACS_CHARACTERISTIC_AVAILABILE_AUDIO_CONTEXTS};
    uint8_t csis_chars[] = {
        GA_LIB_CSIS_CHARACTERISTIC_SIRK, //0
        GA_LIB_CSIS_CHARACTERISTIC_SIZE, //1
        GA_LIB_CSIS_CHARACTERISTIC_LOCK, //2
        GA_LIB_CSIS_CHARACTERISTIC_RANK, //3
    };
    uint8_t vcs_chars[] = {GA_LIB_VCS_CHARACTERISTIC_VOLUME_STATE};
    uint8_t mics_chars[] = {GA_LIB_MICS_CHARACTERISTIC_MUTE_STATE};
    uint8_t has_chars[] = {
        GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_FEATURES,             //0
        GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_PRESET_CONTROL_POINT, //1
        GA_LIB_HAS_CHARACTERISTIC_HEARING_AID_ACTIVE_PRESET_INDEX   //2
    };
    uint8_t bass_chars[] = {
        GA_LIB_BASS_CHARACTERISTIC_BROADCAST_AUDIO_SCAN_CONTROL_POINT, //0
    };
    uint8_t mics_aics_chars[] = {
        GA_LIB_AICS_CHARACTERISTIC_INPUT_STATE,        //0
        GA_LIB_AICS_CHARACTERISTIC_INPUT_CONTROL_POINT //1
    };

    uint8_t num_ascs_chars = sizeof(ascs_chars) / sizeof(ascs_chars[0]);
    uint8_t num_pacs_chars = sizeof(pacs_chars) / sizeof(pacs_chars[0]);
    uint8_t num_csis_chars = sizeof(csis_chars) / sizeof(csis_chars[0]);
    uint8_t num_vcs_chars = sizeof(vcs_chars) / sizeof(vcs_chars[0]);
    uint8_t num_mics_chars = sizeof(mics_chars) / sizeof(mics_chars[0]);
    uint8_t num_has_chars = sizeof(has_chars) / sizeof(has_chars[0]);
    uint8_t num_bass_chars = sizeof(bass_chars) / sizeof(bass_chars[0]);
    uint8_t num_mics_aics = sizeof(mics_aics_chars) / sizeof(mics_aics_chars[0]);

    uint16_t total_chars = num_ascs_chars + num_pacs_chars + num_csis_chars + num_vcs_chars + num_mics_chars +
                           num_has_chars + num_bass_chars + num_mics_aics;
    total_chars += LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS;
    total_chars += LEPL_MAX_BASS_RCV_STATE_SUPPORTED;

    p_clcb->p_op =
        gatt_intf_alloc_notification_handle_list(p_clcb->conn_id, total_chars, on_enable_notification_cmpl, p_clcb);
    if (p_clcb->p_op == NULL)
    {
        return WICED_BT_NO_RESOURCES;
    }

    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    int pos = 0;
    for (int i = 0; i < num_ascs_chars; i++)
    {
        if (p_peer->ascs[ascs_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->ascs[ascs_chars[i]], pos++);
        }
    }
    for (int i = 0; i < (LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS); i++)
    {
        int k = i + GA_LIB_ASCS_CHARACTERISTIC_MAX_UNIQUE;
        if (p_peer->ascs[k].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->ascs[k], pos++);
        }
    }
    for (int i = 0; i < num_pacs_chars; i++)
    {
        if (p_peer->pacs[pacs_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->pacs[pacs_chars[i]], pos++);
        }
    }
    for (int i = 0; i < num_csis_chars; i++)
    {
        if (p_peer->csis[csis_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->csis[csis_chars[i]], pos++);
        }
    }
    for (int i = 0; i < num_vcs_chars; i++)
    {
        if (p_peer->vcs[vcs_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->vcs[vcs_chars[i]], pos++);
        }
    }
    for (int i = 0; i < num_mics_chars; i++)
    {
        if (p_peer->mics[mics_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->mics[mics_chars[i]], pos++);
        }
    }
    for (int i = 0; i < num_has_chars; i++)
    {
        if (p_peer->has[has_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->has[has_chars[i]], pos++);
        }
    }
    for (int i = 0; i < num_bass_chars; i++)
    {
        if (p_peer->bass[bass_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->bass[bass_chars[i]], pos++);
        }
    }
    for (int i = 0; i < LEPL_MAX_BASS_RCV_STATE_SUPPORTED; i++)
    {
        int k = i + GA_LIB_BASS_CHARACTERISTIC_MAX_UNIQUE;
        if (p_peer->bass[k].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->bass[k], pos++);
        }
    }
    for (int i = 0; i < num_mics_aics; i++)
    {
        if (p_peer->mics_aics[0][mics_aics_chars[i]].descriptor_handle)
        {
            gatt_intf_add_characteristic_to_list(p_clcb->p_op, &p_peer->mics_aics[0][mics_aics_chars[i]], pos++);
        }
    }

    app_rpc_send_app_status(p_clcb->conn_id,
                    p_clcb->identity_bd_address,
                    HCI_CONTROL_MISC_APP_STATE_INITING,
                    HCI_CONTROL_MISC_APP_STATE_INIT_ENABLING);

    return gatt_intf_execute_handle_operations(p_clcb->p_op);
}

void lepl_gatt_handle_discovery_complete(lepl_clcb_t *p_clcb, wiced_bt_gatt_status_t status)
{
    app_rpc_send_app_status(p_clcb->conn_id, p_clcb->identity_bd_address, HCI_CONTROL_MISC_APP_STATE_DISCOVERY_COMPLETE, status);
    WICED_BT_TRACE("[%s] status %d", __FUNCTION__, status);
    if (status)
    {
        return;
    }
    /* start ASE discovery (read all the ASE char on the peer to get the list of ASE ID's) */
    if (status == WICED_BT_GATT_SUCCESS)
    {
        lepl_set_gatt_state_discovery_complete(p_clcb);
    }

    // Read initial values of specific characteristics
    lepl_enable_app_notifications(p_clcb);
}

void on_has_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                               uint16_t conn_id,
                               wiced_bt_gatt_status_t status,
                               gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }
    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_HAS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->has);
        lepl_gatt_handle_discovery_complete(p_clcb, status);
    }
}

void on_gmap_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_GMAP];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->gmap);
        p_clcb->p_discovery_ctx = ga_lib_has_discover_service(p_clcb->conn_id, on_has_discovery_complete);
    }
}

void on_csis_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_CSIS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->csis);
        p_clcb->p_discovery_ctx = ga_lib_gmap_discover_service(p_clcb->conn_id, on_gmap_discovery_complete);
    }
}

void on_aics_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;

    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        int aics_instance = 0;
        gatt_intf_service_range_t *p_handles = &p_clcb->peer_profiles.service_handles[LEPL_MICS_AICS];
        for (; aics_instance < LEPL_MAX_MICS_AICS; aics_instance++, p_handles++)
        {
            if (p_result->service_range.start_handle == p_handles->start_handle)
            {
                lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_handles, p_peer->mics_aics[aics_instance]);
                WICED_BT_TRACE("[%s] AICS instance %d discovered", __FUNCTION__, aics_instance);
                aics_instance++;
                p_handles++;
                break;
            }
        }

        if (aics_instance < LEPL_MAX_MICS_AICS && (p_handles->start_handle != 0))
        {
            gatt_intf_service_range_t *p_handles =
                &p_clcb->peer_profiles.service_handles[LEPL_MICS_AICS + aics_instance];
            p_clcb->p_discovery_ctx = ga_lib_aics_discover_service(p_clcb->conn_id,
                                                                   on_aics_discovery_complete,
                                                                   p_handles->start_handle,
                                                                   p_handles->end_handle);
        }
        else
        {
            p_clcb->p_discovery_ctx = ga_lib_csis_discover_service(p_clcb->conn_id, on_csis_discovery_complete);
        }
    }
}


void on_mics_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;

    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_MICS];

        int aics_instance = 0;
        for (int i = 0; i < p_result->max_included_services; i++)
        {
            gatt_intf_included_service_types_t *p_incl_service = &p_result->p_inc_service_types[i];
            wiced_bt_uuid_t uuid = p_incl_service->p_inc_info->uuid;
            if ((uuid.uu.uuid16 == WICED_BT_UUID_AUDIO_INPUT_CONTROL) &&
                p_incl_service->inc_handles.service.start_handle)
            {
                p_peer->service_handles[LEPL_MICS_AICS + aics_instance].start_handle =
                    p_incl_service->inc_handles.service.start_handle;
                p_peer->service_handles[LEPL_MICS_AICS + aics_instance].end_handle =
                    p_incl_service->inc_handles.service.end_handle;
                aics_instance++;
            }
        }

        /* Store frees the discovery ctx */
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->mics);
        if (aics_instance)
        {
            gatt_intf_service_range_t *p_handles =
                &p_clcb->peer_profiles.service_handles[LEPL_MICS_AICS];

            p_clcb->p_discovery_ctx = ga_lib_aics_discover_service(p_clcb->conn_id,
                                                                   on_aics_discovery_complete,
                                                                   p_handles->start_handle,
                                                                   p_handles->end_handle);
        }
        else
        {
            p_clcb->p_discovery_ctx = ga_lib_csis_discover_service(p_clcb->conn_id, on_csis_discovery_complete);
        }
    }
}

void on_vcs_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                               uint16_t conn_id,
                               wiced_bt_gatt_status_t status,
                               gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;

    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_VCS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->vcs);
        p_clcb->p_discovery_ctx = ga_lib_mics_discover_service(p_clcb->conn_id, on_mics_discovery_complete, 2);
    }
}

void on_bass_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;

    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_BASS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->bass);
        p_clcb->p_discovery_ctx = ga_lib_vcs_discover_service(p_clcb->conn_id, on_vcs_discovery_complete, 0, 0);
    }
}

void on_pacs_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;

    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_PACS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->pacs);
        p_clcb->p_discovery_ctx = ga_lib_bass_discover_service(p_clcb->conn_id,
                                                               on_bass_discovery_complete,
                                                               LEPL_MAX_BASS_RCV_STATE_SUPPORTED);
    }
}

void on_ascs_discovery_complete(gatt_intf_service_discovery_ctx_t *p_sdc,
                                uint16_t conn_id,
                                wiced_bt_gatt_status_t status,
                                gatt_intf_discovery_result_t *p_result)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_peer_profiles_t *p_peer = &p_clcb->peer_profiles;
    if (!p_clcb)
    {
        WICED_BT_TRACE("[%s] No clcb found for conn_id %d", __FUNCTION__, conn_id);
        return;
    }

    if (status == WICED_BT_GATT_SUCCESS)
    {
        gatt_intf_service_range_t *p_service_range = &p_peer->service_handles[LEPL_ASCS];
        lepl_cache_discovery_results_and_cleanup(p_clcb, p_result, p_service_range, p_peer->ascs);
        p_clcb->p_discovery_ctx = ga_lib_pacs_discover_service(p_clcb->conn_id,
                                                               on_pacs_discovery_complete,
                                                               LEPL_PACS_MAX_SNK_CHARACTERISTICS,
                                                               LEPL_PACS_MAX_SRC_CHARACTERISTICS);
    }
}

void lepl_gatt_start_discovery(lepl_clcb_t *p_clcb)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    p_clcb->p_discovery_ctx = ga_lib_ascs_discover_service(p_clcb->conn_id,
                                                           on_ascs_discovery_complete,
                                                           LEPL_ASCS_MAX_SNK_CHARACTERISTICS,
                                                           LEPL_ASCS_MAX_SRC_CHARACTERISTICS);

    if (status)
        WICED_BT_TRACE_CRIT("[%s] status [%d] \n", __FUNCTION__, status);
}

wiced_bt_gatt_status_t lepl_gatt_init(int max_connections, int max_mtu)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;

    wiced_bt_gatt_server_enable_caching();

    gatt_status = wiced_bt_gatt_db_init(lepl_gatt_database, sizeof(lepl_gatt_database), g_lepl_gatt_cb.db_hash);
    if (WICED_BT_SUCCESS != gatt_status)
        return gatt_status;

    gatt_status = wiced_bt_gatt_register(lepl_gatt_cback);
    if (WICED_BT_SUCCESS != gatt_status)
        return gatt_status;

    gatt_intf_set_cccd_value_callback(lepl_handle_get_cccd_value_cb);

#ifdef SIMULATED_NVRAM
    lepl_nvram_init();
#endif
    lepl_cap_reset_audio_state();

    return gatt_status;
}

wiced_result_t lepl_gatt_disconnect(uint16_t conn_id)
{
    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    if (p_clcb && p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle)
    {
        lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(conn_id);
        WICED_BT_TRACE("[%s] Disconnecting conn_id %d from set of %d devices\n",
                       __FUNCTION__,
                       conn_id,
                       p_set ? p_set->set_size : 0);

        if (!p_set || (p_set->set_size == 0))
        {
            app_rpc_send_app_status(conn_id, p_clcb->identity_bd_address, HCI_CONTROL_MISC_APP_STATE_DISCONNECTING, 0);
            wiced_bt_gatt_disconnect(conn_id);
            WICED_BT_TRACE("[%s] Disconnecting conn_id %d\n", __FUNCTION__, conn_id);
            return WICED_SUCCESS;
        }

        for (int i = 0; i < p_set->set_size; i++)
        {
            if (p_set->member_list[i].is_used)
            {
                app_rpc_send_app_status(p_set->member_list[i].conn_id, p_set->member_list[i].address, HCI_CONTROL_MISC_APP_STATE_DISCONNECTING, 0);
                wiced_bt_gatt_disconnect(p_set->member_list[i].conn_id);
                WICED_BT_TRACE("[%s] Disconnecting conn_id %d\n", __FUNCTION__, p_set->member_list[i].conn_id);
            }
        }
    }
    else if (p_clcb)
    {
        app_rpc_send_app_status(p_clcb->conn_id, p_clcb->identity_bd_address, HCI_CONTROL_MISC_APP_STATE_DISCONNECTING, 0);
        wiced_bt_gatt_disconnect(p_clcb->conn_id);
        WICED_BT_TRACE("[%s] Disconnecting conn_id %d\n", __FUNCTION__, p_clcb->conn_id);
    }
    return WICED_SUCCESS;
}

wiced_result_t lepl_gatt_handle_disconnecting_state(void)
{
    wiced_result_t result = WICED_BT_ERROR;
    int index = 0;
    WICED_BT_TRACE("[%s]\n", __FUNCTION__);
    for (index = 0; index < LEPL_MAX_CONNECTIONS; index++)
    {
        lepl_clcb_t *p_clcb = &g_lepl_gatt_cb.clcb[index];

        if (p_clcb->disconnecting)
        {
            WICED_BT_TRACE("[%s] disconnecting %x\n", __FUNCTION__, p_clcb->conn_id);
            result = wiced_bt_gatt_disconnect(p_clcb->conn_id);
        }
    }

    return result;
}

#define UNICAST_SOURCE_EXT_ADV_HANDLE 1

void lepl_gatt_start_stop_adv(uint32_t b_start)
{
    wiced_result_t status;

#define AD_FLAG_SIZE 2
#define AUDIO_STREAM_CONTROL_SERVICE_SIZE 9
#define BASS_SOLICITATION_SIZE 4
#define ADV_NAME_SIZE 16
#define ADV_SIZE                                                                                                       \
    (AD_FLAG_SIZE + 1 + AUDIO_STREAM_CONTROL_SERVICE_SIZE + 1 + ADV_NAME_SIZE + 1 +                                    \
     BASS_SOLICITATION_SIZE) // +1 for length itself

    wiced_ble_ext_adv_duration_config_t duration_cfg;
    uint8_t data[ADV_SIZE] = {0};
    uint8_t *p_ext_adv_data = data;
    wiced_bt_dev_status_t sts;
    uint8_t addr_type = (lepl_cfg_settings.p_ble_cfg->rpa_refresh_timeout) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;

    WICED_BT_TRACE("[%s] %s adv\n", __FUNCTION__, b_start ? "start" : "stop");

    // Set ext adv params
    if (b_start)
    {
        wiced_ble_ext_adv_params_t params = {
            .event_properties = WICED_BLE_EXT_ADV_EVENT_PROPERTY_CONNECTABLE_ADV,
            .primary_adv_int_min = 40,
            .primary_adv_int_max = 40,
            .primary_adv_channel_map = (BTM_BLE_ADVERT_CHNL_37 | BTM_BLE_ADVERT_CHNL_38 | BTM_BLE_ADVERT_CHNL_39),
            .own_addr_type = addr_type,
            .peer_addr_type = addr_type,
            .peer_addr = {0, 0, 0, 0, 0, 0},
            .adv_filter_policy = BTM_BLE_ADV_POLICY_ACCEPT_CONN_AND_SCAN,
            .adv_tx_power = 0x7f,
            .primary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
            .secondary_adv_max_skip = 0,
            .secondary_adv_phy = WICED_BLE_EXT_ADV_PHY_1M,
            .adv_sid = 1,
            .scan_request_not = WICED_BLE_EXT_ADV_SCAN_REQ_NOTIFY_ENABLE,
            .primary_phy_opts = 0,
            .secondary_phy_opts = 0};

        wiced_ble_ext_adv_set_params(UNICAST_SOURCE_EXT_ADV_HANDLE, &params);

        if (addr_type == BLE_ADDR_RANDOM)
        {
            wiced_ble_ext_adv_set_random_address(UNICAST_SOURCE_EXT_ADV_HANDLE, g_lepl_gatt_cb.own_addr);
        }

        UINT8_TO_STREAM(p_ext_adv_data, AD_FLAG_SIZE);
        UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_ADVERT_TYPE_FLAG);
        UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED);

        UINT8_TO_STREAM(p_ext_adv_data, 1 + strlen((const char *)lepl_cfg_settings.device_name));
        UINT8_TO_STREAM(p_ext_adv_data, BTM_BLE_ADVERT_TYPE_NAME_COMPLETE);
        ARRAY_TO_STREAM(p_ext_adv_data,
                        lepl_cfg_settings.device_name,
                        strlen((const char *)lepl_cfg_settings.device_name));

        // Set adv data in LTV format
        sts = wiced_ble_ext_adv_set_adv_data(UNICAST_SOURCE_EXT_ADV_HANDLE, (p_ext_adv_data - data), data);
        WICED_BT_TRACE("[%s] sts %d [adv size %d]\n", __FUNCTION__, sts, (p_ext_adv_data - data));
    }

    duration_cfg.adv_handle = UNICAST_SOURCE_EXT_ADV_HANDLE;
    duration_cfg.adv_duration = 0;
    duration_cfg.max_ext_adv_events = 0;

    // Start adv
    status = wiced_ble_ext_adv_enable(b_start, 1, &duration_cfg);

    if (status == WICED_BT_SUCCESS)
    {
        lepl_set_current_ble_activity(b_start ? LEPL_BLE_ACTIVITY_ADVERTISING : LEPL_BLE_ACTIVITY_NONE);
    }

    WICED_BT_TRACE("[%s] status 0x%x", __FUNCTION__, status);
}
