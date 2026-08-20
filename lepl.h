/*
 * $ Copyright Cypress Semiconductor $
 */

/** @file lepl.h
 *
 * @brief LE Audio Player (LEPL) - Main Header File
 *
 * This file contains the data structures and function prototypes for the LE Audio Player (LEPL) module.
 **/

#ifndef __LEPL_H__
#define __LEPL_H__

/* BT Stack includes */
#include "wiced_bt_cfg.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_types.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "wiced_bt_stack_platform.h"

/* App Library includes */
#include "audio_driver.h"
#include "ga_lib_aics.h"
#include "ga_lib_ascs.h"
#include "ga_lib_bap.h"
#include "ga_lib_bass.h"
#include "ga_lib_csis.h"
#include "ga_lib_gmap.h"
#include "ga_lib_has.h"
#include "ga_lib_ias.h"
#include "ga_lib_mcs.h"
#include "ga_lib_mics.h"
#include "ga_lib_pacs.h"
#include "ga_lib_tbs.h"
#include "ga_lib_tmap.h"
#include "ga_lib_vcs.h"
#include "gatt_interface.h"
#include "le_audio_rpc.h"
#include <stdlib.h>
#include <stdbool.h>

/* Application includes */
#include "lepl_nvram.h"

#define ARIP_MXTDM_PAIR MXTDM_PAIR_1     /**< 0 - MXTDM0, 1 - MXTDM1 */
#define ARIP_MXTDM_MODE MXTDM_MODE_TDM   /**< 0 - MXTDM_TDM, 1 - MXTDM_I2S */
#define ARIP_MXTDM_ROLE MXTDM_BUS_SLAVE  /**< 0 - Slave, 1 - Master */
#define ARIP_MXTDM_TDM_CH_NUM 4          /**< Total number of slots/channels, minimum - 2 */
#define ARIP_MXTDM_A2DP_LEFT_CH 0        /**< Left channel for A2DP audio */
#define ARIP_MXTDM_A2DP_RIGHT_CH 1       /**< Right channel for A2DP audio */
#define ARIP_MXTDM_HFP_CH 2              /**< Bi-direction on same channel */
#define ARIP_MXTDM_LE_LEFT_CH 0          /**< Left channel for LE audio */
#define ARIP_MXTDM_LE_RIGHT_CH 1         /**< Right channel for LE audio */

#define LEPL_TBS_BEARER_NAME_MAX_SIZE 50   /**< Max Bearer name size supported*/
#define LEPL_TBS_BEARER_UCI_MAX_SIZE 50    /**< Max Uniform Caller Identifier size supported*/
#define LEPL_TBS_BEARER_URI_MAX_SIZE 50    /**< Max Uniform Resource Intentifier size supported*/
#define LEPL_TBS_RM_CALLERID_MAX_SIZE 50   /**< Max size of uri prefix and remote caller id*/
#define LEPL_TBS_TG_CALLERID_MAX_SIZE 50   /**< Max size of target caller id*/
#define LEPL_TBS_FRIENDLY_NAME_MAX_SIZE 50 /**< Max size of call friendly name*/
#define LEPL_TBS_BEARER_MAX_CALL_COUNT 1   /**< Max call count suppoorted */

// For A2DP + LE Case set to 10
// For A2DP + HFP Case set to 6
#define LE_CIG_SYNC_DELAY 10

/*
Data_Path_ID' => (Interface << 5) + slot
Bits 5:7 = hardware interface
Bits 0:6 = slot, for TDM bus slot = slot, for I2S slot 0 = left and slot 1 = right.
Hardware interfaces available are:
    0 = HCI  (illegal value for Configure_Data_Path)
    1 = ARIP_I2S master only
    2 = PCM2, I2S master only (H2 only, PCM is removed in H1)
    3 = MXTDM_0 in I2S mode (H1 only)
    4 = MXTDM_0 in TDM mode(H1 only)
    5 = MXTDM_1 in I2S mode (H1 only)
    6 = MXTDM_1 in TDM mode (H1 only)
*/
#define ARIP_MXTDM_TDM_1 0xC0   /**<  6 << 5 TDM1 peripheral in TDM mode, encoded as shown above */

#define ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID                                                                          \
    (ARIP_MXTDM_TDM_1 | ARIP_MXTDM_LE_LEFT_CH)   /**< Interface -6, channel - 0 1100 0000 */
#define ARIP_MXTDM_LE_AUDIO_CH_R_DATA_PATH_ID                                                                          \
    (ARIP_MXTDM_TDM_1 | ARIP_MXTDM_LE_RIGHT_CH) /**< Interface -6, channel - 1 1100 0001 */

#define ARIP_MXTDM_TDM_TX_CH_MAP 0x0F /**< TDM TX channel map */
#define ARIP_MXTDM_TDM_RX_CH_MAP 0x0F /**< TDM RX channel map */
#define HAP_ENABLED 1 /**< HAP feature enabled */
#define TMAP_ENABLED 1 /**< TMAP feature enabled */

#define LEPL_MAX_COORDINATED_SET 1 /**< Maximum number of coordinated sets supported */
#define LEPL_MAX_CONNECTIONS 2     /**< Maximum number of simultaneous connections */

#ifndef APP_CONN_INTERVAL_MIN
#define APP_CONN_INTERVAL_MIN WICED_BT_CFG_DEFAULT_CONN_MIN_INTERVAL
#endif

#ifndef APP_CONN_INTERVAL_MAX
#define APP_CONN_INTERVAL_MAX WICED_BT_CFG_DEFAULT_CONN_MAX_INTERVAL
#endif

#ifndef APP_BLE_MAX_RX_PDU_SIZE
#define APP_BLE_MAX_RX_PDU_SIZE 512
#endif

#define LEPL_MAX_ASES                                                                                                  \
    (LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS) /* Maximum number of ASEs supported */

#define MAX_URI_LEN 50 /**< Maximum length of URI supported */
#define MAX_FRIENDLY_NAME_LEN 50 /**< Maximum length of friendly name supported */

#define IS_JOIN_SUPPORTED(x) ((x) & GA_LIB_TBS_FEATURE_BIT_JOIN) /**< Check if Join feature is supported */
#define IS_HOLD_SUPPORTED(x) ((x) & GA_LIB_TBS_FEATURE_BIT_LOCAL_HOLD) /**< Check if Hold feature is supported */
#define IS_INBAND_RINGTONE_SUPPORTED(x) ((x) & GA_LIB_TBS_FEATURE_BIT_INBAND_RINGTONE) /**< Check if Inband Ringtone feature is supported */
#define IS_SILENT_MODE_SUPPORTED(x) ((x) & GA_LIB_TBS_FEATURE_BIT_SILENT_MODE) /**< Check if Silent Mode feature is supported */

#define MAX_CIS_CONN 2 /**< Maximum number of CIS connections supported */
#define LEPL_MAX_SDU_SIZE 240 /**< Maximum SDU size handled by LEPL */
#define MAX_DESCRIPTION 20 /**< Maximum length of VOCS/AICS description supported */
#define LEPL_HAS_MAX_PRESET_RECORDS 5 /**< Maximum number of preset records that can be stored */
#define LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH 40 /**< Maximum length of preset record name that can be stored */

#define HFP_CONNECTED (0) /**< Macro for HFP connection */
#define A2DP_CONNECTED (1) /**< Macro for A2DP connection */
#define AVRCP_CONNECTED (2) /**< Macro for AVRCP connection */
#define ALL_CONNECTED                                                                                                  \
    ((1 << HFP_CONNECTED) | (1 << A2DP_CONNECTED) | (1 << AVRCP_CONNECTED)) /**< Macro for all connections */

#define LEPL_TMAP_ROLE                                                                                                 \
    (TMAP_ROLE_CALL_GATEWAY | TMAP_ROLE_UNICAST_MEDIA_SENDER |                                                         \
     TMAP_ROLE_BROADCAST_MEDIA_SENDER) /* TMAP role supported by the application */

#define LEPL_GMAP_ROLE (GMAP_ROLE_UNICAST_GAME_GATEWAY) /* GMAP role supported */
#define LEPL_GMAP_UGG_FEATURES (UGG_MULTIPLEX_FEATURE_SUPPORTED) /* GMAP UGG features supported */
#define LEPL_GMAP_BGS_FEATURES (0)                               /* GMAP BGS features supported */

#define LEPL_CSIS_SET_MEMBER_DISCOVERY_TIMEOUT_IN_SEC 10 /**< CSIS timeout to discover the set members set at 10 seconds */

#define LEPL_APPEARANCE_GENERIC_AUDIO_SOURCE 0X0880    /**< Device appearance as Generic Audio Source */

/** BLE Activity state for the LEPL application. This enum is used to track the current BLE activity state of the application*/
typedef enum
{
    LEPL_BLE_ACTIVITY_NONE = 0,     /**< No BLE activity */
    LEPL_BLE_ACTIVITY_ADVERTISING,  /**< Advertising */
    LEPL_BLE_ACTIVITY_SCANNING,     /**< Scanning */
    LEPL_BLE_ACTIVITY_CONNECTING,   /**< Connection in progress */
} lepl_ble_activity_state_t;

enum
{
    HDLS_GATT_GENERIC_ATTRIBUTE_SERVICE = 0x1,                                        // 0x0001 , 1
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED,                                      // 0x0002 , 2
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED_VALUE,                                // 0x0003 , 3
    HDLD_GATT_GENERIC_ATTRIBUTE_SERVICE_CHANGED_CLIENT_CONFIGURATION,                 // 0x0004 , 4
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_DB_HASH,                                      // 0x0005 , 5
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_DB_HASH_VALUE,                                // 0x0006 , 6
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CLIENT_FEATURES,                              // 0x0007 , 7
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_CLIENT_FEATURES_VALUE,                        // 0x0008 , 8
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES,                              // 0x0009 , 9
    HDLC_GATT_GENERIC_ATTRIBUTE_SERVICE_SERVER_FEATURES_VALUE,                        // 0x000A , 10

    HDLS_GATT_GENERIC_ACCESS_SERVICE = 0x20,                                          // 0x0020 , 32
    HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME,                                             // 0x0021 , 33
    HDLC_GATT_GENERIC_ACCESS_DEVICE_NAME_VALUE,                                       // 0x0022 , 34
    HDLC_GATT_GENERIC_ACCESS_APPEARANCE,                                              // 0x0023 , 35
    HDLC_GATT_GENERIC_ACCESS_APPEARANCE_VALUE,                                        // 0x0024 , 36
    HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM,                              // 0x0025 , 37
    HDLC_GATT_GENERIC_ACCESS_PREFERRED_CONNECTION_PARAM_VALUE,                        // 0x0026 , 38

#if (ENABLE_MCS == 1)
    HDLS_MCS = 0x90,                                                                  // 0x0090 , 144
    HDLC_MCS_MEDIA_PLAYER_NAME,                                                       // 0x0091 , 145
    HDLC_MCS_MEDIA_PLAYER_NAME_VALUE,                                                 // 0x0092 , 146
    HDLD_MCS_PLAYER_NAME_DESCRIPTION_CLIENT_CONFIGURATION,                            // 0x0093 , 147
    HDLC_MCS_MEDIA_TRACK_CHANGED,                                                     // 0x0094 , 148
    HDLC_MCS_MEDIA_TRACK_CHANGED_VALUE,                                               // 0x0095 , 149
    HDLD_MCS_TRACK_CHANGED_DESCRIPTION_CLIENT_CONFIGURATION,                          // 0x0096 , 150
    HDLC_MCS_MEDIA_TRACK_TITLE,                                                       // 0x0097 , 151
    HDLC_MCS_MEDIA_TRACK_TITLE_VALUE,                                                 // 0x0098 , 152
    HDLD_MCS_TRACK_TITLE_DESCRIPTION_CLIENT_CONFIGURATION,                            // 0x0099 , 153
    HDLC_MCS_MEDIA_TRACK_DURATION,                                                    // 0x009A , 154
    HDLC_MCS_MEDIA_TRACK_DURATION_VALUE,                                              // 0x009B , 155
    HDLD_MCS_TRACK_DURATION_DESCRIPTION_CLIENT_CONFIGURATION,                         // 0x009C , 156
    HDLC_MCS_MEDIA_TRACK_POSITION,                                                    // 0x009D , 157
    HDLC_MCS_MEDIA_TRACK_POSITION_VALUE,                                              // 0x009E , 158
    HDLD_MCS_TRACK_POSITION_DESCRIPTION_CLIENT_CONFIGURATION,                         // 0x009F , 159
    HDLC_MCS_PLAYBACK_SPEED,                                                          // 0x00A0 , 160
    HDLC_MCS_PLAYBACK_SPEED_VALUE,                                                    // 0x00A1 , 161
    HDLD_MCS_PLAYBACK_SPEED_DESCRIPTION_CLIENT_CONFIGURATION,                         // 0x00A2 , 162
    HDLC_MCS_SEEKING_SPEED,                                                           // 0x00A3 , 163
    HDLC_MCS_SEEKING_SPEED_VALUE,                                                     // 0x00A4 , 164
    HDLD_MCS_SEEKING_SPEED_DESCRIPTION_CLIENT_CONFIGURATION,                          // 0x00A5 , 165
    HDLC_MCS_PLAYING_ORDER,                                                           // 0x00A6 , 166
    HDLC_MCS_PLAYING_ORDER_VALUE,                                                     // 0x00A7 , 167
    HDLD_MCS_PLAYING_ORDER_DESCRIPTION_CLIENT_CONFIGURATION,                          // 0x00A8 , 168
    HDLC_MCS_PLAYING_ORDER_SUPPORTED,                                                 // 0x00A9 , 169
    HDLC_MCS_PLAYING_ORDER_SUPPORTED_VALUE,                                           // 0x00AA , 170
    HDLC_MCS_MEDIA_STATE,                                                             // 0x00AB , 171
    HDLC_MCS_MEDIA_STATE_VALUE,                                                       // 0x00AC , 172
    HDLD_MCS_MEDIA_STATE_DESCRIPTION_CLIENT_CONFIGURATION,                            // 0x00AD , 173
    HDLC_MCS_MEDIA_CONTROL_POINT,                                                     // 0x00AE , 174
    HDLC_MCS_MEDIA_CONTROL_POINT_VALUE,                                               // 0x00AF , 175
    HDLD_MCS_CONTROL_POINT_DESCRIPTION_CLIENT_CONFIGURATION,                          // 0x00B0 , 176
    HDLC_MCS_MEDIA_CONTROL_POINT_OPCODE_SUPPORTED,                                    // 0x00B1 , 177
    HDLC_MCS_MEDIA_CONTROL_POINT_OPCODE_SUPPORTED_VALUE,                              // 0x00B2 , 178
    HDLD_MCS_CONTROL_POINT_OPCODE_SUPPORTED_DESCRIPTION_CLIENT_CONFIGURATION,         // 0x00B3 , 179
    HDLC_MCS_CONTENT_CONTROL_ID,                                                      // 0x00B4 , 180
    HDLC_MCS_CONTENT_CONTROL_ID_VALUE,                                                // 0x00B5 , 181
#endif                                                                                // ENABLE_MCS == 1

    HDLS_GMCS = 0x150,                                                                // 0x0150 , 336
    HDLC_GMCS_PLAYER_NAME,                                                            // 0x0151 , 337
    HDLC_GMCS_PLAYER_NAME_VALUE,                                                      // 0x0152 , 338
    HDLD_GMCS_PLAYER_NAME_CLIENT_CONFIGURATION,                                       // 0x0153 , 339
    HDLC_GMCS_TRACK_CHANGED,                                                          // 0x0154 , 340
    HDLC_GMCS_TRACK_CHANGED_VALUE,                                                    // 0x0155 , 341
    HDLD_GMCS_TRACK_CHANGED_CLIENT_CONFIGURATION,                                     // 0x0156 , 342
    HDLC_GMCS_TRACK_TITLE,                                                            // 0x0157 , 343
    HDLC_GMCS_TRACK_TITLE_VALUE,                                                      // 0x0158 , 344
    HDLD_GMCS_TRACK_TITLE_CLIENT_CONFIGURATION,                                       // 0x0159 , 345
    HDLC_GMCS_TRACK_DURATION,                                                         // 0x015A , 346
    HDLC_GMCS_TRACK_DURATION_VALUE,                                                   // 0x015B , 347
    HDLD_GMCS_TRACK_DURATION_CLIENT_CONFIGURATION,                                    // 0x015C , 348
    HDLC_GMCS_TRACK_POSITION,                                                         // 0x015D , 349
    HDLC_GMCS_TRACK_POSITION_VALUE,                                                   // 0x015E , 350
    HDLD_GMCS_TRACK_POSITION_CLIENT_CONFIGURATION,                                    // 0x015F , 351
    HDLC_GMCS_PLAYBACK_SPEED,                                                         // 0x0160 , 352
    HDLC_GMCS_PLAYBACK_SPEED_VALUE,                                                   // 0x0161 , 353
    HDLD_GMCS_PLAYBACK_SPEED_CLIENT_CONFIGURATION,                                    // 0x0162 , 354
    HDLC_GMCS_SEEKING_SPEED,                                                          // 0x0163 , 355
    HDLC_GMCS_SEEKING_SPEED_VALUE,                                                    // 0x0164 , 356
    HDLD_GMCS_SEEKING_SPEED_CLIENT_CONFIGURATION,                                     // 0x0165 , 357
    HDLC_GMCS_PLAYING_ORDER,                                                          // 0x0166 , 358
    HDLC_GMCS_PLAYING_ORDER_VALUE,                                                    // 0x0167 , 359
    HDLD_GMCS_PLAYING_ORDER_CLIENT_CONFIGURATION,                                     // 0x0168 , 360
    HDLC_GMCS_PLAYING_ORDER_SUPPORTED,                                                // 0x0169 , 361
    HDLC_GMCS_PLAYING_ORDER_SUPPORTED_VALUE,                                          // 0x016A , 362
    HDLC_GMCS_MEDIA_STATE,                                                            // 0x016B , 363
    HDLC_GMCS_MEDIA_STATE_VALUE,                                                      // 0x016C , 364
    HDLD_GMCS_MEDIA_STATE_CLIENT_CONFIGURATION,                                       // 0x016D , 365
    HDLC_GMCS_CONTROL_POINT,                                                          // 0x016E , 366
    HDLC_GMCS_CONTROL_POINT_VALUE,                                                    // 0x016F , 367
    HDLD_GMCS_CONTROL_POINT_CLIENT_CONFIGURATION,                                     // 0x0170 , 368
    HDLC_GMCS_CONTROL_POINT_OPCODE_SUPPORTED,                                         // 0x0171 , 369
    HDLC_GMCS_CONTROL_POINT_OPCODE_SUPPORTED_VALUE,                                   // 0x0172 , 370
    HDLD_GMCS_CONTROL_POINT_OPCODE_SUPPORTED_CLIENT_CONFIGURATION,                    // 0x0173 , 371
    HDLC_GMCS_CONTENT_CONTROL_ID,                                                     // 0x0174 , 372
    HDLC_GMCS_CONTENT_CONTROL_ID_VALUE,                                               // 0x0175 , 373

#if (ENABLE_TBS == 1)
    HDLS_TBS = 0x190,                                                                 // 0x0190 , 400
    HDLC_TBS_BEARER_PROVIDER_NAME,                                                    // 0x0191 , 401
    HDLC_TBS_BEARER_PROVIDER_NAME_VALUE,                                              // 0x0192 , 402
    HDLD_TBS_BEARER_PROVIDER_NAME_CLIENT_CONFIGURATION,                               // 0x0193 , 403
    HDLC_TBS_BEARER_UCI,                                                              // 0x0194 , 404
    HDLC_TBS_BEARER_UCI_VALUE,                                                        // 0x0195 , 405
    HDLC_TBS_BEARER_TECHNOLOGY,                                                       // 0x0196 , 406
    HDLC_TBS_BEARER_TECHNOLOGY_VALUE,                                                 // 0x0197 , 407
    HDLD_TBS_BEARER_TECHNOLOGY_CLIENT_CONFIGURATION,                                  // 0x0198 , 408
    HDLC_TBS_BEARER_URI_SCHEMES,                                                      // 0x0199 , 409
    HDLC_TBS_BEARER_URI_SCHEMES_VALUE,                                                // 0x019A , 410
    HDLD_TBS_BEARER_URI_SCHEMES_CLIENT_CONFIGURATION,                                 // 0x019B , 411
    HDLC_TBS_BEARER_SIGNAL_STRENGTH,                                                  // 0x019C , 412
    HDLC_TBS_BEARER_SIGNAL_STRENGTH_VALUE,                                            // 0x019D , 413
    HDLD_TBS_BEARER_SIGNAL_STRENGTH_CLIENT_CONFIGURATION,                             // 0x019E , 414
    HDLC_TBS_BEARER_SIG_STR_REPORTING_INTERVAL,                                       // 0x019F , 415
    HDLC_TBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE,                                 // 0x0200 , 416
    HDLC_TBS_BEARER_LIST_CURRENT_CALL,                                                // 0x0201 , 417
    HDLC_TBS_BEARER_LIST_CURRENT_CALL_VALUE,                                          // 0x0202 , 418
    HDLD_TBS_BEARER_LIST_CURRENT_CALL_CLIENT_CONFIGURATION,                           // 0x0203 , 419
    HDLC_TBS_CONTENT_CONTROL_ID,                                                      // 0x0204 , 420
    HDLC_TBS_CONTENT_CONTROL_ID_VALUE,                                                // 0x0205 , 421
    HDLC_TBS_STATUS_FLAG,                                                             // 0x0206 , 422
    HDLC_TBS_STATUS_FLAG_VALUE,                                                       // 0x0207 , 423
    HDLD_TBS_STATUS_FLAG_CLIENT_CONFIGURATION,                                        // 0x0208 , 424
    HDLC_TBS_INCOMING_TG_URI,                                                         // 0x0209 , 425
    HDLC_TBS_INCOMING_TG_URI_VALUE,                                                   // 0x020A , 426
    HDLD_TBS_INCOMING_TG_CALLER_ID_CLIENT_CONFIGURATION,                              // 0x020B , 427
    HDLC_TBS_CALL_STATE,                                                              // 0x020C , 428
    HDLC_TBS_CALL_STATE_VALUE,                                                        // 0x020D , 429
    HDLD_TBS_CALL_STATE_CLIENT_CONFIGURATION,                                         // 0x020E , 430
    HDLC_TBS_CALL_CONTROL_POINT,                                                      // 0x020F , 431
    HDLC_TBS_CALL_CONTROL_POINT_VALUE,                                                // 0x0210 , 432
    HDLD_TBS_CALL_CONTROL_POINT_CLIENT_CONFIGURATION,                                 // 0x0211 , 433
    HDLC_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,                                      // 0x0212 , 434
    HDLC_TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE_VALUE,                                // 0x0213 , 435
    HDLC_TBS_TERMINATION_REASON,                                                      // 0x0214 , 436
    HDLC_TBS_TERMINATION_REASON_VALUE,                                                // 0x0215 , 437
    HDLD_TBS_TERMINATION_REASON_CLIENT_CONFIGURATION,                                 // 0x0216 , 438
    HDLC_TBS_INCOMING_CALL,                                                           // 0x0217 , 439
    HDLC_TBS_INCOMING_CALL_VALUE,                                                     // 0x0218 , 430
    HDLD_TBS_INCOMING_CALL_CLIENT_CONFIGURATION,                                      // 0x0219 , 431
    HDLC_TBS_CALL_FRIENDLY_NAME,                                                      // 0x021A , 432
    HDLC_TBS_CALL_FRIENDLY_NAME_VALUE,                                                // 0x021B , 433
    HDLD_TBS_CALL_FRIENDLY_NAME_CLIENT_CONFIGURATION,                                 // 0x021C , 434
#endif                                                                                // ENABLE_TBS = 1

    HDLS_GTBS = 0x220,                                                                // 0x0220 , 544
    HDLC_GTBS_BEARER_PROVIDER_NAME,                                                   // 0x0221 , 545
    HDLC_GTBS_BEARER_PROVIDER_NAME_VALUE,                                             // 0x0222 , 546
    HDLD_GTBS_BEARER_PROVIDER_NAME_CLIENT_CONFIGURATION,                              // 0x0223 , 547
    HDLC_GTBS_BEARER_UCI,                                                             // 0x0224 , 548
    HDLC_GTBS_BEARER_UCI_VALUE,                                                       // 0x0225 , 549
    HDLC_GTBS_BEARER_TECHNOLOGY,                                                      // 0x0226 , 550
    HDLC_GTBS_BEARER_TECHNOLOGY_VALUE,                                                // 0x0227 , 551
    HDLD_GTBS_BEARER_TECHNOLOGY_CLIENT_CONFIGURATION,                                 // 0x0228 , 552
    HDLC_GTBS_BEARER_URI_SCHEMES,                                                     // 0x0229 , 553
    HDLC_GTBS_BEARER_URI_SCHEMES_VALUE,                                               // 0x022A , 554
    HDLD_GTBS_BEARER_URI_SCHEMES_CLIENT_CONFIGURATION,                                // 0x022B , 555
    HDLC_GTBS_BEARER_SIGNAL_STRENGTH,                                                 // 0x022C , 556
    HDLC_GTBS_BEARER_SIGNAL_STRENGTH_VALUE,                                           // 0x022D , 557
    HDLD_GTBS_BEARER_SIGNAL_STRENGTH_CLIENT_CONFIGURATION,                            // 0x022E , 558
    HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL,                                      // 0x022F , 559
    HDLC_GTBS_BEARER_SIG_STR_REPORTING_INTERVAL_VALUE,                                // 0x0230 , 560
    HDLC_GTBS_BEARER_LIST_CURRENT_CALL,                                               // 0x0231 , 561
    HDLC_GTBS_BEARER_LIST_CURRENT_CALL_VALUE,                                         // 0x0232 , 562
    HDLD_GTBS_BEARER_LIST_CURRENT_CALL_CLIENT_CONFIGURATION,                          // 0x0233 , 563
    HDLC_GTBS_CONTENT_CONTROL_ID,                                                     // 0x0234 , 564
    HDLC_GTBS_CONTENT_CONTROL_ID_VALUE,                                               // 0x0235 , 565
    HDLC_GTBS_INCOMING_TG_URI,                                                        // 0x0236 , 566
    HDLC_GTBS_INCOMING_TG_URI_VALUE,                                                  // 0x0237 , 567
    HDLD_GTBS_INCOMING_TG_URI_CLIENT_CONFIGURATION,                                   // 0x0238 , 568
    HDLC_GTBS_STATUS_FLAG,                                                            // 0x0239 , 569
    HDLC_GTBS_STATUS_FLAG_VALUE,                                                      // 0x023A , 570
    HDLD_GTBS_STATUS_FLAG_CLIENT_CONFIGURATION,                                       // 0x023B , 571
    HDLC_GTBS_CALL_STATE,                                                             // 0x023C , 572
    HDLC_GTBS_CALL_STATE_VALUE,                                                       // 0x023D , 573
    HDLD_GTBS_CALL_STATE_CLIENT_CONFIGURATION,                                        // 0x023E , 574
    HDLC_GTBS_CALL_CONTROL_POINT,                                                     // 0x023F , 575
    HDLC_GTBS_CALL_CONTROL_POINT_VALUE,                                               // 0x0240 , 576
    HDLD_GTBS_CALL_CONTROL_POINT_CLIENT_CONFIGURATION,                                // 0x0241 , 577
    HDLC_GTBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE,                                     // 0x0242 , 578
    HDLC_GTBS_CALL_CONTROL_POINT_OPTIONAL_OPCODE_VALUE,                               // 0x0243 , 579
    HDLC_GTBS_TERMINATION_REASON,                                                     // 0x0244 , 580
    HDLC_GTBS_TERMINATION_REASON_VALUE,                                               // 0x0245 , 581
    HDLD_GTBS_TERMINATION_REASON_CLIENT_CONFIGURATION,                                // 0x0246 , 582
    HDLC_GTBS_INCOMING_CALL,                                                          // 0x0247 , 583
    HDLC_GTBS_INCOMING_CALL_VALUE,                                                    // 0x0248 , 584
    HDLD_GTBS_INCOMING_CALL_CLIENT_CONFIGURATION,                                     // 0x0249 , 585
    HDLC_GTBS_CALL_FRIENDLY_NAME,                                                     // 0x024A , 586
    HDLC_GTBS_CALL_FRIENDLY_NAME_VALUE,                                               // 0x024B , 587
    HDLD_GTBS_CALL_FRIENDLY_NAME_CLIENT_CONFIGURATION,                                // 0x024C , 588

    HDLS_TMAP = 0x260,                                                                // 0x0260 , 608
    HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE,                                     // 0x0261 , 609
    HDLC_TMAP_TELEPHONE_MEDIA_AUDIO_PROFILE_ROLE_VALUE,                               // 0x0262 , 610

#if GMAP_SERVER_ENABLED
    HDLS_GMAP = 0x300,                                                                // 0x0300 , 768
    HDLC_GMAP_ROLE,                                                                   // 0x0301 , 769
    HDLC_GMAP_ROLE_VALUE,                                                             // 0x0302 , 770
    HDLC_GMAP_UGG_FEATURES,                                                           // 0x0303 , 771
    HDLC_GMAP_UGG_FEATURES_VALUE,                                                     // 0x0304 , 772
    HDLC_GMAP_BGS_FEATURES,                                                           // 0x0305 , 773
    HDLC_GMAP_BGS_FEATURES_VALUE,                                                     // 0x0306 , 774
#endif

    HDLS_DIS = 0x310,                                                                 // 0x0310 , 784
    HDLC_DIS_MANUFACTURER_NAME_STRING,                                                // 0x0311 , 785
    HDLC_DIS_MANUFACTURER_NAME_STRING_VALUE,                                          // 0x0312 , 786
    HDLC_DIS_MODEL_NUMBER_STRING,                                                     // 0x0313 , 787
    HDLC_DIS_MODEL_NUMBER_STRING_VALUE,                                               // 0x0314 , 788
    HDLC_DIS_FIRMWARE_REVISION_STRING,                                                // 0x0315 , 789
    HDLC_DIS_FIRMWARE_REVISION_STRING_VALUE,                                          // 0x0316 , 790
    HDLC_DIS_SOFTWARE_REVISION_STRING,                                                // 0x0317 , 791
    HDLC_DIS_SOFTWARE_REVISION_STRING_VALUE,                                          // 0x0318 , 792


};

/*
* @brief Audio mode to track the current audio mode of the application.
*/
typedef enum
{
    LEPL_AUDIO_MODE_NONE,       // 0    /**< No audio mode */
    LEPL_AUDIO_MODE_BROADCAST,  // 1    /**< Broadcast audio mode */
    LEPL_AUDIO_MODE_MEDIA,      // 2    /**< Media audio mode */
    LEPL_AUDIO_MODE_MIC,        // 3    /**< Microphone audio mode */
    LEPL_AUDIO_MODE_BIDIRMIC,   // 4    /**< Bidirectional microphone audio mode */
    LEPL_AUDIO_MODE_CALL,       // 5    /**< Call audio mode */
    LEPL_AUDIO_MODE_IN_TRANSIT  // 6    /**< In transit audio mode */
} lepl_audio_mode_t;

/*
* @brief Call type
*/
typedef enum
{
    INCOMING_CALL = 0,  /**< Incoming call */
    OUTGOING_CALL = 1   /**< Outgoing call */
} lepl_tbs_call_type_t;

/*
* @brief Call state to track and notify the current call state of originated calls
*/
typedef enum
{
    TBS_ACTION_CALL_ACCEPTED = 0x00,          /**< Accept the call locally. */
    TBS_ACTION_CALL_TERMINATED = 0x01,        /**< Terminate the call */
    TBS_ACTION_CALL_HELD = 0x02,              /**< Locally hold the call */
    TBS_ACTION_CALL_RETRIEVED = 0x03,         /**< Retrieve hold the call */
    TBS_ACTION_CALL_PLACED = 0x04,            /**< Place the call */
    TBS_ACTION_CALL_JOINED = 0x05,            /**< Join multiple calls */
    TBS_ACTION_CALL_REMOTELY_HELD = 0x06,     /**< Remotely hold the call */
    TBS_ACTION_CALL_REM_REMOTELY_HELD = 0x07, /**< Remove Remotely hold the call */
    TBS_ACTION_CALL_ALERT = 0x08,             /**< Set the call to Alerting state */
} lepl_tbs_action_t;

/*
* @brief Call control server state
*/
typedef enum
{
    CALL_CONTROL_SERVER_STATE_IDLE,                    // 0  /**< Idle state */
    CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE,         // 1  /**< In-band ringtone state */
    CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE_CONVO,   // 2  /**< In-band ringtone conversation state */
    CALL_CONTROL_SERVER_STATE_CONVO                    // 3  /**< Conversation state */
} lepl_ccs_states_t;

/*
* @brief RFCOMM port count and SCN for the SPP profile.
*/
enum
{
    OFU_SPP_RFCOMM_PORT_COUNT = 1,  /**< SPP RFCOMM port count */
    OFU_SPP_RFCOMM_SCN = 2,         /**< SPP RFCOMM SCN */
};

/*
* @brief CSIS lock state current CSIS lock state of coordinated set.
*/
typedef enum
{
    CSIS_UNLOCKED,           /**< Unlocked */
    CSIS_LOCK_IN_PROGRESS,   /**< CSIS lock in progress */
    CSIS_LOCKED,             /**< Locked */
    CSIS_UNLOCK_IN_PROGRESS, /**< CSIS unlock in progress */
} lepl_csis_lock_state_t;

/**
* @brief Codec configuration parameters
*/
typedef struct
{
    uint16_t min_data_per_frame; /**< Min data per codec frame */
    uint16_t max_data_per_frame; /**< Max data per codec frame */
    uint16_t sf;                 /**< Sampling frequency */
    uint8_t frame_duration;      /**< Frame duration */
    uint8_t audio_ch_count;      /**< Audio channel count */
    uint8_t frame_per_sdu;       /**< Frame per sdu */
    uint16_t octet_per_frame;    /**< Octects per frame */
    uint16_t blocks_per_sdu;     /**< Blocks per sdu */
} le_audio_utils_pacs_t;

/*
* @brief Audio configuration parameters.
*/
typedef struct
{
    uint32_t sampling_frequency;           /**< Sampling frequency */
    uint32_t frame_duration;               /**< Frame duration */
    uint16_t octets_per_codec_frame;       /**< Octets per codec frame */
    uint32_t sdu_interval;                 /**< SDU interval */
    uint8_t channel_count;                 /**< Channel count */
    uint8_t target_latency;                /**< Target latency */
    wiced_ble_isoc_phy_t phy;              /**< PHY */
    uint16_t mtl;                          /**< Maximum Transmission Latency */
    uint8_t rtn;                           /**< Retransmission Number */
    uint32_t presentation_delay;           /**< Presentation delay */
} lepl_audio_config_t;

/*
* @brief Unicast stream configuration parameters.
*/
typedef struct
{
    uint8_t num_cis; /**< Number of CIS */
    ga_lib_bap_context_type_t context_type; /**< Context type */
    wiced_ble_isoc_framing_t framing; /**< Framing */
    lepl_audio_config_t in_cfg; /**< Stream configurations for data path direction input (sending/outgoing stream) */
    lepl_audio_config_t out_cfg; /**< Stream configurations for data path direction output (Receiving/incoming stream) */
} lepl_unicast_stream_config_t;

/*
* @brief Audio mode transition information
*/
typedef struct
{
    lepl_audio_mode_t initial_mode; /**< Initial audio mode */
    lepl_audio_mode_t final_mode;   /**< Final audio mode */
} lepl_audio_mode_transit_info_t;

/*
* @brief struct to hold the ASE info.
*/
typedef struct
{
    ga_lib_ascs_ase_t ase;
    uint16_t acl_conn_handle;           /**< ACL connection handle */
    uint16_t cis_conn_handle;           /**< CIS connection handle */
    uint16_t gatt_value_handle;         /**< GATT value handle for this ASE */
    uint32_t data_path_established : 1; /**< data path established ?*/
} lepl_ase_data_t;

/*
* @brief Audio state information
*/
typedef struct
{
    lepl_audio_mode_t current_mode;                       /**< Current audio mode */
    lepl_audio_mode_t paused_mode;                        /**< Paused audio mode */
    lepl_audio_mode_transit_info_t transit_info;          /**< Audio mode transition information */
    lepl_unicast_stream_config_t paused_strm_config;      /**< Paused stream configuration */
    lepl_unicast_stream_config_t current_strm_config;     /**< Current stream configuration */
    lepl_ase_data_t *p_lepl_ase_list[MAX_CIS_CONN *
                                     2];                  /**< List of ASEs for the current state, max of 2 for input and 2 for output */
    uint8_t is_releasing;                                 /**< Is the audio state releasing? */
} lepl_audio_state_info_t;

/*
* @brief Volume control service data
*/
typedef struct
{
    ga_lib_vcs_volume_state_t volume_state;        /**< volume information */
    ga_lib_volume_flag_val_t volume_flag;          /**< volume persistence flag */
} lepl_vcs_data_t;

#define MAX_MEDIA_PLAYER_NAME_LEN 20 /**< Maximum length of media player name */
#define MAX_MEDIA_TRACK_TITLE_LEN 20 /**< Maximum length of media track title */

/*
* @brief Media control service data
*/
typedef struct
{
    uint8_t track_selected;                            /**< Track Selected */
    char player_name[MAX_MEDIA_PLAYER_NAME_LEN];       /**< Media Player Name */
    char track_title[MAX_MEDIA_TRACK_TITLE_LEN];       /**< Track Title */
    int32_t track_duration;                            /**< Track Duration */
    int32_t track_position;                            /**< Track Position */
    int current_track_number;                          /**< Track Number */
    int8_t playback_speed;                             /**< Playback Speed */
    int8_t seeking_speed;                              /**< Seeking Speed */
    ga_lib_mcs_playing_order_t playing_order;          /**< Playing Order */
    uint16_t playing_order_supported;                  /**< Playing Order Supported bit field */
    ga_lib_mcs_state_t media_state;                    /**< Media State */
    uint32_t media_control_opcodes_supported;          /**< Media Control Supported Opcodes */
    uint8_t content_control_id;                        /**< Content Control ID */
} lepl_mcs_t;

/*
* @brief Coordinated Set Identification Service data
*/
typedef struct
{
    ga_lib_csis_sirk_data_t sirk_data; /**< set identity resolving key */
    uint8_t size;                      /**< number of devices in the coordinated set */
    uint8_t rank;                      /**< rank of the device in the coordinated set */
    ga_lib_csis_lock_val_t lock;       /**< lock state of the coordinated set */
} lepl_csis_data_t;

/*
* @brief Call state data
*/
typedef struct
{
    uint8_t in_use;                                      /**< Is the call state data in use? */
    lepl_tbs_call_type_t call_type;                      /**< Call type: incoming or outgoing */
    ga_lib_tbs_current_call_t call;                      /**< Current call information */
    char uri[LEPL_TBS_RM_CALLERID_MAX_SIZE];             /**< call uri (variable length) */
    char friendly_name[LEPL_TBS_FRIENDLY_NAME_MAX_SIZE]; /**< call friendly name (variable length) */
} lepl_tbs_call_state_data_t;

/*
 * @brief Telephone bearer service data
 */
typedef struct
{
    uint8_t current_call_id;                                                    /**< Current call id */
    uint8_t num_calls;                                                          /**< Number of calls */
    uint8_t is_caller_id_invalid_enabled;                                       /**< Is caller ID invalid enabled? */
    uint8_t content_control_id;                                                 /**< Content control id */
    char bearer_provider_name[LEPL_TBS_BEARER_NAME_MAX_SIZE];                   /**< Bearer provider name */
    char bearer_uci[LEPL_TBS_BEARER_UCI_MAX_SIZE];                              /**< Bearer UCI */
    char bearer_uri[LEPL_TBS_BEARER_URI_MAX_SIZE];                              /**< Bearer URI */
    uint8_t bearer_technology;                                                  /**< Bearer technology */
    uint8_t bearer_signal_strength;                                             /**< Bearer signal strength */
    uint8_t prev_bearer_signal_strength;                                        /**< Previous bearer signal strength */
    uint8_t bearer_signal_strength_reporting_interval;                          /**< Bearer signal strength reporting interval */
    uint16_t bearer_status_flag;                                                /**< Bearer status flag */
    uint16_t ccp_supported_opcode;                                              /**< CCP supported opcode */
    uint8_t latest_incoming_remote_call_id;                                     /**< Latest incoming remote call id */
    lepl_tbs_call_state_data_t call_state_data[LEPL_TBS_BEARER_MAX_CALL_COUNT]; /**< Call state data */
    char incoming_tg_caller_id[LEPL_TBS_TG_CALLERID_MAX_SIZE];                  /**< Incoming TG caller id */
    lepl_ccs_states_t ccs_state;                                                /**< Call control server state */
} lepl_tbs_t;

/*
* @brief Audio input control service data
*/
typedef struct
{
    ga_lib_aics_input_state_t input_state;           /**< Audio Input State */
    ga_lib_aics_gain_settings_params_t gain_setting; /**< Audio Input Gain setting */
    ga_lib_aics_input_type_t input_type;             /**< Audio Input Type */
    ga_lib_aics_input_status_t input_status;         /**< Audio Input Status */
    char description[MAX_DESCRIPTION];               /**< Audio Input descritpion */
    uint8_t description_len;
} lepl_aics_data_t;

/*
* @brief Microphone control service data
*/
typedef struct
{
    uint8_t mute_state; /**< Microphone mute state */
} lepl_mics_data_t;

/*
* @brief Hearing Aid preset record structure
*/
typedef struct
{
    uint8_t preset_index;                                  /**< Preset index */
    uint8_t properties;                                    /**< Preset properties */
    uint8_t name_len;                                      /**< Preset name length */
    char name[LEPL_HAS_MAX_PRESET_RECORD_NAME_LENGTH + 1]; /**< Preset name */
} lepl_has_preset_rec_t;

/*
* @brief Hearing aid service data
*/
typedef struct
{
    uint8_t hearing_aid_features;                                       /**< Hearing aid features */
    uint8_t active_preset_index;                                        /**< Active preset index */
    lepl_has_preset_rec_t preset_rec_list[LEPL_HAS_MAX_PRESET_RECORDS]; /**< Preset record list */
} lepl_has_t;

/*
* @brief Gaming and media profile data
*/
typedef struct
{
    ga_lib_gmap_role_t gmap_role; /**< Gaming and media profile role */
    uint8_t ugg_features;         /**< UGG features */
    uint8_t ugt_features;         /**< UGT features */
    uint8_t bgs_feature;          /**< BGS feature */
    uint8_t bgr_feature;          /**< BGR feature */
} lepl_gmap_t;


/*
* @brief Connection link control block data
*/
typedef struct
{
    uint32_t in_use : 1;                                                                         /**< In use flag */
    uint32_t connected : 1;                                                                      /**< Connected flag */
    uint32_t mtu_exchanged : 1;                                                                  /**< MTU exchanged flag */
    uint32_t discovery_complete : 1;                                                             /**< Service discovery complete flag */
    uint32_t enabled_notifications : 1;                                                          /**< Notifications enabled flag */
    uint32_t read_characteristics : 1;                                                           /**< Characteristics read flag */
    uint32_t ready : 1;                                                                          /**< Ready flag */
    uint32_t disconnecting : 1;                                                                  /**< Disconnecting flag */

    uint16_t conn_id;                                                                            /**< Connection ID */
    uint16_t acl_handle;                                                                         /**< ACL handle */
    uint8_t addr_type;                                                                           /**< Peer Address type */
    uint8_t identity_address_type;                                                               /**< Identity address type */
    wiced_bt_device_address_t identity_bd_address;                                               /**< Identity BD address */
    wiced_bt_device_address_t conn_addr;                                                         /**< Connection address */
    wiced_bool_t b_is_central;                                                                   /**< Central role flag */
    lepl_peer_profiles_t peer_profiles;                                                          /**< Peer profiles */
    ga_lib_pacs_data_t pacs;                                                                     /**< PACS data */
    lepl_vcs_data_t vcs;                                                                         /**< VCS data */
    lepl_csis_data_t csis;                                                                       /**< CSIS data */
    lepl_mics_data_t mics;                                                                       /**< MICS data */
    lepl_aics_data_t mics_aics[LEPL_MAX_MICS_AICS];                                              /**< MICS AICS data */
    lepl_has_t has;                                                                              /**< HAS data */
    lepl_gmap_t gmap;                                                                            /**< GMAP data */
    lepl_ase_data_t ases[LEPL_ASCS_MAX_SNK_CHARACTERISTICS + LEPL_ASCS_MAX_SRC_CHARACTERISTICS]; /**< ASE data */
    uint16_t conn_interval;                                                                      /**< Connection interval */
    gatt_intf_service_discovery_ctx_t *p_discovery_ctx;                                          /**< service discovery context */
    gatt_intf_operation_t *p_op;                                                                 /**< handle list operation context */
    uint32_t cccd_bits[LEPL_MAX_DWORD_TO_STORE_CCCD(LEPL_MAX_CCCD_TO_STORE)];                    /**< CCCD bits */
    wiced_bt_db_hash_t db_hash;                                                                  /**< Database hash */
    wiced_bt_gatt_client_supported_features_t csf;                                               /**< Client supported features */
    uint8_t waiting_for_vcs_notif;                                                               /**< To track if the app is waiting for VCS notification */
    uint8_t waiting_for_mics_notif;                                                              /**< To track if the app is waiting for MICS notification */
    uint8_t waiting_for_aics_notif[LEPL_MAX_MICS_AICS];                                          /**< To track if the app is waiting for AICS notification */
} lepl_clcb_t;

/*
* @brief Coordinated set member data.
*/
typedef struct
{
    uint8_t is_used;                      /**< In use flag */
    wiced_bt_device_address_t address;    /**< Device address */
    uint8_t addr_type;                    /**< Address type */
    uint16_t conn_id;                     /**< Connection ID */
    lepl_clcb_t *p_clcb;                  /**< Reference to the connection link control block */
    uint8_t rank;                         /**< Member Rank in Coordinated Set */
    lepl_csis_lock_state_t lock_state;    /**< Lock state */
} lepl_coordinated_set_member_t;

/*
* @brief Coordinated set data.
*/
typedef struct
{
    ga_lib_csis_sirk_t sirk;                                            /**< Set identity resolving key */
    uint8_t sirk_type;                                                  /**< SIRK type */
    uint8_t set_size;                                                   /**< Set size */
    uint8_t num_devices;                                                /**< Number of discovered devices */
    wiced_timer_t scan_timer;                                           /**< Scan timer */
    lepl_csis_lock_state_t group_lock_state;                            /**< Group lock state */
    lepl_coordinated_set_member_t member_list[LEPL_MAX_CONNECTIONS];    /**< List of coordinated set members */
    ga_lib_vcs_volume_state_t set_volume_state;                         /**< Set volume state */
    lepl_mics_data_t set_mics_state;                                    /**< Set MICS data */
    ga_lib_aics_input_state_t set_aics_input_state[LEPL_MAX_MICS_AICS]; /**< Set AICS input state */
} lepl_cordinated_set_t;

/**< Ordered Access complete callback to be set by the application, this will be invoked once the ordered access procedure is complete */
typedef wiced_bt_gatt_status_t (*lepl_csis_ordered_access_procedure_cmpl_cb_t)(lepl_clcb_t *p_clcb,
                                                                               void *p_data,
                                                                               wiced_bt_gatt_status_t status);

/**< Lock procedure complete callback to be set by the application, this will be invoked once the lock procedure is complete */
typedef wiced_bt_gatt_status_t (*lepl_csis_lock_procedure_cmpl_cb_t)(lepl_clcb_t *p_clcb,
                                                                     void *p_data,
                                                                     wiced_bt_gatt_status_t status);

/*
* @brief Local service data
*/
typedef struct
{
    lepl_mcs_t gmcs; /**< GMCS data */
    lepl_tbs_t gtbs; /**< GTBS data */
} lepl_local_service_data_t;

/*
* @brief GATT control block data
*/
typedef struct
{
    wiced_bt_device_address_t own_addr;                             /**< Own device address */
    lepl_local_service_data_t local_service_data;                   /**< Local service data */
    wiced_bt_db_hash_t db_hash;                                     /**< Database hash */
    lepl_clcb_t clcb[LEPL_MAX_CONNECTIONS];                         /**< Connection link control blocks */
    // A set is just a collection of items, whereas a group is a set paired with an operation
    lepl_cordinated_set_t csis_set[LEPL_MAX_COORDINATED_SET];       /**< Coordinated sets */
    lepl_audio_state_info_t audio_state;                            /**< Audio state information */
#if GMAP_SERVER_ENABLED
    lepl_gmap_t gmap_data;                                          /**< GMAP data */
#endif
    lepl_ble_activity_state_t ble_activity;                         /**< BLE activity state */
    wiced_bt_device_address_t connecting_peer_addr;                 /**< Address of the device connecting to */
    uint8_t enable_uuid_filter;                                     /**< Enable UUID filter for scanning to filter advertisement with ASCS UUIDs */
} lepl_gatt_cb_t;

extern wiced_bt_cfg_settings_t lepl_cfg_settings;
extern lepl_gatt_cb_t g_lepl_gatt_cb;
#ifdef ENABLE_LC3_PLUS
extern bool lc3_hrmode;
#endif



/*================================================ GATT ==========================================================*/
/*
* @brief Sets the current BLE activity state
*
*  @param[in] new_state: The new BLE activity state to set.
*/
void lepl_set_current_ble_activity(lepl_ble_activity_state_t new_state);

/*
* @brief Get the current BLE activity state
*
* @return The current BLE activity state.
*/
lepl_ble_activity_state_t lepl_get_current_ble_activity(void);

/*
* @brief Initialize the GATT for the LEPL application.
*
* @param[in] max_connections: The maximum number of connections to support.
* @param[in] max_mtu: The maximum MTU size to support.
* @return The status of the GATT initialization.
*/
wiced_bt_gatt_status_t lepl_gatt_init(int max_connections, int max_mtu);

/*
* @brief Start the GATT discovery for the given connection link control block.
*
* @param[in] p_clcb: Reference to the connection link control block for which to start discovery.
*/
void lepl_gatt_start_discovery(lepl_clcb_t *p_clcb);

/*
* @brief Start or stop scanning.
*
* @param[in] start: 1 to start scanning, 0 to stop scanning.
* @param[in] enable_uuid_filter: 1 to enable UUID filter ASCS UUID, 0 to disable UUID filter.
* @return The status of the scan operation.
*/
wiced_result_t lepl_gatt_start_stop_scan(uint32_t start, uint8_t enable_uuid_filter);

/*
* @brief Disconnect the GATT connection
*
* @param[in] conn_id: The connection ID of the GATT connection to disconnect.
* @return The status of the GATT disconnect operation.
*/
wiced_result_t lepl_gatt_disconnect(uint16_t conn_id);

/*
* @brief Disconnects all the GATT connections which are in disconnecting state.
*
* @return The status of the GATT disconnect operation.
*/
wiced_result_t lepl_gatt_handle_disconnecting_state();

/*
* @brief Start or stop advertising .
*
* @param[in] b_start: 1 to start advertising, 0 to stop advertising.
*/
void lepl_gatt_start_stop_adv(uint32_t b_start);

/*
* @brief Get the connection link control block for given Bluetooth address.
*
* @param[in] p_bd_addr: The Bluetooth address of the peer device.
* @return Reference to the connection link control block for the given Bluetooth address, or NULL if not found.
*/
lepl_clcb_t *lepl_gatt_get_clcb(uint8_t *p_bd_addr);

/*
* @brief Get the connection link control block by connection ID.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @return Reference to the connection link control block for the given connection ID, or NULL if not found.
*/
lepl_clcb_t *lepl_gatt_get_clcb_by_conn_id(uint16_t conn_id);

/*
* @brief Get the connection link control block by ACL handle.
*
* @param[in] acl_conn_handle: The ACL connection handle of the GATT connection.
* @return Reference to the connection link control block for the given ACL handle, or NULL if not found.
*/
lepl_clcb_t *lepl_gatt_get_clcb_by_conn_handle(uint16_t acl_conn_handle);

/*
* @brief Start or stop scanning.
*
* @param[in] start: 1 to start scanning, 0 to stop scanning.
* @param[in] p_cback: Reference to the callback function to handle scan results.
* @return WICED_SUCCESS if the scanning operation was started/stopped successfully, otherwise an error code.
*/
wiced_result_t lepl_start_stop_scan(uint32_t start, wiced_ble_ext_scan_result_cback_t *p_cback);

/*
* @brief Set the connection options
*
* @param[in] mask: The PHY mask to set for the connection options.
* @param[in] p_out: Pointer to the connection configuration PHY options to set.
*/

void app_set_connection_options(wiced_ble_ext_adv_phy_mask_t mask, wiced_ble_ext_conn_cfg_phy_options_t *p_out);

/*
* @brief Initiate ACL connection to a remote device.
*
* @param[in] addr_type: The address type of the remote device (public or random).
* @param[in] bd_addr: The Bluetooth device address of the remote device.
* @return WICED_SUCCESS if the connection was initiated successfully, otherwise an error code.
*/
wiced_result_t app_create_connection(uint8_t addr_type, wiced_bt_device_address_t bd_addr);

/*
* @brief Send HCI Request to update connection parameters.
*
* @param[in] bd_addr: The Bluetooth device address of the remote device for which to update connection parameters.
*/
void lepl_update_conn_param(wiced_bt_device_address_t bd_addr);

/*
* @brief Enable application notifications for the given connection link control block.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @return WICED_SUCCESS if the notifications were enabled successfully, otherwise an error code.
*/
wiced_result_t lepl_enable_app_notifications(lepl_clcb_t *p_clcb);


/*
* @brief Read NVRAM data for the device and populate the connection link control block with the read data.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @return The NVRAM ID of the read data, or 0 if the read operation failed.
*/
uint16_t lepl_read_device_from_nvram(lepl_clcb_t *p_clcb);

/*
* @brief Read remote characteristics for the device and populate the connection link control block with the read data.
*
* @param[in] p_clcb: Reference to the connection link control block.
*/
wiced_result_t lepl_read_remote_characteristics(lepl_clcb_t *p_clcb);

/*
* @brief Save the device data to NVRAM for the given connection link control block.
*
* @param[in] p_clcb: Reference to the connection link control block.
*/
void lepl_save_device_data_to_nvram(lepl_clcb_t *p_clcb);

/*
* @brief Bluetooth management callback function for the LEPL application. This function is called by the Bluetooth stack to notify the application of various Bluetooth events.
*
* @param[in] event: The Bluetooth management event.
* @param[in] p_event_data: Pointer to the event data associated with the event.
* @return WICED_SUCCESS if the event was handled successfully, otherwise an error code.
*/
wiced_result_t lepl_btm_cback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data);
/*================================================================================================================*/


/*=================================================== RPC ========================================================*/
/*
* @brief Initialize the RPC.
*
* @param[in] app_instance: The application instance to initialize the RPC for.
*/
void lepl_rpc_init(uint8_t app_instance);

/*
* @brief Send the PACS records to the client control.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_data: Pointer to the PACS records data to send.
* @param[in] val_len: The length of the PACS records data.
* @param[in] is_sink: 1 if the PACS records are for sink, 0 if for source.
*/
void lepl_rpc_send_pacs_records(uint16_t conn_id, uint8_t *p_data, uint16_t val_len, uint8_t is_sink);

/*
* @brief Send the audio started event to the client control.
*/
void lepl_rpc_send_audio_started_event();

/*
* @brief Send CSIS lock state to the client control.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] lock_state: The lock state to send (0 for unlocked, 1 for locked).
*/
void lepl_rpc_send_csis_lock_state(uint16_t conn_id, uint8_t lock_state);

/*
* @brief Start the voice capture/ MIC streaming.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_audio_config: Reference to the unicast stream configuration for voice capture.
*/
void lepl_start_voice_capture(uint16_t conn_id, lepl_unicast_stream_config_t *p_audio_config);

/*
* @brief Send MIC state to client control.
*
* @param[in] state: The MIC state to send (0 for inactive, 1 for active).
*/
void lepl_rpc_send_mic_state(uint8_t state);

/*
* @brief Start or stop the bidirectional microphone/ Conversational stream.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] start: 1 to start the bidirectional microphone, 0 to stop it.
* @param[in] p_stream_config: Reference to the unicast stream configuration for the bidirectional microphone.
*/
void lepl_start_stop_bidir_mic(uint16_t conn_id, uint8_t start, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Send the conversation stream state to client control.
*
* @param[in] state: The conversation stream state to send (0 for inactive, 1 for active).
*/
void lepl_rpc_send_convo_stream_state(uint8_t state);

/*
* @brief Send link keys to the client control.
*
* @param[in] nvram_id: The NVRAM ID for the link keys.
* @param[in] p_key_data: Pointer to the paired device key data to send.
*/
void lepl_rpc_send_link_keys(uint16_t nvram_id, lepl_nvram_paired_device_key_t *p_key_data);

/*
* @brief Send the identity resolving key to the client control.
*
* @param[in] p_id_keys: Pointer to the local identity keys to send.
*/
void lepl_rpc_send_identity_resolving_key(wiced_bt_local_identity_keys_t *p_id_keys);

/*
* @brief Disconnect the device.
*
* @param[in] conn_handle: The connection handle of the device to disconnect.
* @return WICED_SUCCESS if the device was disconnected successfully, otherwise an error code.
*/
wiced_result_t lepl_disconnect_device(uint16_t conn_handle);
/*================================================================================================================*/


/*******************************PROFILE SPECIFIC FUNCTIONS*********************************************************/


/*=============================================== ASCS ===========================================================*/
/*
* @brief Get the remote ASE data by ASE ID and connection link control block.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] ase_id: The ASE ID for which to get the remote ASE data.
* @return Reference to the remote ASE data for the given ASE ID and connection link control block, or NULL if not found.
*/
lepl_ase_data_t *lepl_get_remote_ase_data_by_ase_id(lepl_clcb_t *p_clcb, uint8_t ase_id);

/*
* @brief Get the remote ASE data by type, connection link control block, and start index.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] type:   ASE type
* @param[in] p_start_index: Pointer to the start index for the search.
* @return Reference to the remote ASE data for the given type and connection link control block, or NULL if not found.
*/
lepl_ase_data_t *lepl_get_remote_ase(lepl_clcb_t *p_clcb, ga_lib_ascs_characteristics_t type, uint8_t *p_start_index);

/*
* @brief Compare the codec parameters against supported capabilities of peer device.
*
* @param[in] p_pac: Pointer to the PACS record of the peer device.
* @param[in] p_codec_arg: Pointer to the codec configuration parameters to compare.
* @return WICED_TRUE if the codec parameters match the supported capabilities of the peer device, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_ascs_compare_codec_param(le_audio_utils_pacs_t *p_pac, ga_lib_ascs_config_codec_args_t *p_codec_arg);

/*
* @brief Sends a write command for the ASCS control point to configure the codec for a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_ase: Reference to the ASE data for which to configure the codec.
* @param[in] p_codec_arg: Reference to the codec configuration arguments.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_config_codec(lepl_clcb_t *p_clcb,
                                                          ga_lib_ascs_ase_t *p_ase,
                                                          ga_lib_ascs_config_codec_args_t *p_codec_arg);

/*
* @brief Sends a write command for the ASCS control point to configure the QoS for a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data for which to configure the QoS.
* @param[in] p_qos_arg: Reference to the QoS configuration arguments.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_config_qos(lepl_clcb_t *p_clcb,
                                                        lepl_ase_data_t *p_lepl_ase,
                                                        ga_lib_ascs_config_qos_args_t *p_qos_arg);

/*
* @brief Sends a write command for the ASCS control point to enable a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data to enable.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_enable(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Sends a write command for the ASCS control point to update the metadata for a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data for which to update the metadata.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_update_metadata(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Sends a write command for the ASCS control point to disable a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data to disable.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_disable(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Sends a write command for the ASCS control point to release a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data to release.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_release(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Sends a write command for the ASCS control point to start the receiver for a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data for which to start the receiver.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_receiver_start_ready(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Sends a write command for the ASCS control point to stop the receiver for a given ASE.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_lepl_ase: Reference to the ASE data for which to stop the receiver.
* @return WICED_SUCCESS if the write command was sent successfully, otherwise an error code.
*/
wiced_result_t lepl_ascs_write_control_point_receiver_stop_ready(lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase);

/*
* @brief Handle the received value for ASCS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_ascs_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data);
/*================================================================================================================*/


/*========================================= CAP ==================================================================*/
/*
* @brief Set the next audio mode and switch to transit mode
*
* @param[in] state: The next audio mode to set.
* @param[in] p_stream_config: Reference to the unicast stream configuration for the next audio mode.
*/
void lepl_cap_set_next_audio_mode(lepl_audio_mode_t state, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Get the current audio mode.
*
* @return The current audio mode.
*/
lepl_audio_mode_t lepl_cap_get_audio_mode(void);

/*
* @brief Get the final audio mode.
*
* @return The final audio mode application will switch after completing transit operation.
*/
lepl_audio_mode_t lepl_cap_get_final_audio_mode(void);

/*
* @brief Reset the audio state.
*/
void lepl_cap_reset_audio_state(void);

/*
* @brief Update the audio mode.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] state: The ASCS state.
*/
void lepl_cap_update_audio_mode(uint16_t conn_id, ga_lib_ascs_state_t state);

/*
* @brief Handles the ASCS error response.
*
* @param[in] conn_id: The connection ID of the GATT connection.
*/
void lepl_cap_handle_ascs_error_response(uint16_t conn_id);

/*
* @brief Handles the coordinated set disconnection.
*/
void lepl_cap_handle_set_disconnection(void);

/*
* @brief Cleanup the ASEs for the given ACL handle after disconnection.
*
* @param[in] acl_handle: The ACL connection handle for which to cleanup the ASEs.
*/
void lepl_cap_cleanup_ases_for_acl_handle(uint16_t acl_handle);

/*
* @brief Start media streaming.
*
* @param[in] p_set: Reference to the coordinated set.
* @param[in] p_stream_config: Reference to the unicast stream configuration for media streaming.
* @return WICED_SUCCESS if the media streaming was started successfully, otherwise an error code.
*/
wiced_result_t lepl_cap_start_media_streaming(lepl_cordinated_set_t *p_set,
                                              lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Start the conversation streaming.
*
* @param[in] p_set: Reference to the coordinated set.
* @param[in] p_stream_config: Reference to the unicast stream configuration for conversation streaming.
* @return WICED_SUCCESS if the conversation streaming was started successfully, otherwise an error code.
*/
wiced_result_t lepl_cap_start_conv_streaming(lepl_cordinated_set_t *p_set,
                                             lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Start the microphone streaming.
*
* @param[in] p_set: Reference to the coordinated set.
* @param[in] p_stream_config: Reference to the unicast stream configuration for microphone streaming.
* @return WICED_SUCCESS if the microphone streaming was started successfully, otherwise an error code.
*/
wiced_result_t lepl_cap_start_mic_streaming(lepl_cordinated_set_t *p_set,
                                            lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Stop the media streaming.
*
* @param[in] p_set: Reference to the coordinated set.
*/
void lepl_cap_stop_media_streaming(lepl_cordinated_set_t *p_set);

/*
* @brief Stop the conversation streaming.
*
* @param[in] p_set: Reference to the coordinated set.
*/
void lepl_cap_stop_conv_streaming(lepl_cordinated_set_t *p_set);

/*
* @brief Stop the microphone streaming.
*
* @param[in] p_set: Reference to the coordinated set.
*/
void lepl_cap_stop_mic_streaming(lepl_cordinated_set_t *p_set);

/*
* @brief Stop the Unicast streaming.
*
* @param[in] conn_id: The connection ID of the GATT connection.
*/
wiced_result_t lepl_cap_stop_streaming(uint16_t conn_id);

/*
* @brief Resume the streaming after a pause or interruption.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] mode: The audio mode to resume.
* @param[in] p_stream_config: Reference to the unicast stream configuration for resuming streaming.
*/
void lepl_cap_resume_streaming(uint16_t conn_id, lepl_audio_mode_t mode, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Handle the set member reconnection.
*
* @param[in] conn_id: The connection ID of the GATT connection.
*/
void lepl_cap_handle_set_member_reconnection(uint16_t conn_id);

/*
* @brief Start the Unicast streaming.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_stream_config: Reference to the unicast stream configuration for streaming.
* @return WICED_SUCCESS if the streaming was started successfully, otherwise an error code.
*/
wiced_result_t lepl_cap_start_streaming(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Handles the ASCS state change.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_ase: Reference to the ASE data.
*/
void lepl_cap_ascs_state_update(uint16_t conn_id, lepl_clcb_t *p_clcb,lepl_ase_data_t *p_ase);

/*
* @brief Set the volume.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] opcode: The volume control opcode.
* @param[in] abs_vol: The absolute volume value.
*/
void lepl_cap_vcp_set_volume(uint16_t conn_id, ga_lib_vcs_volume_control_opcodes_t opcode, uint8_t abs_vol);

/*
* @brief Set the speaker mute state.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] mute_state: The mute state to set (0 for unmute, 1 for mute).
*/
void lepl_cap_vcp_set_mute_state(uint16_t conn_id, ga_lib_mute_val_t mute_state);

/*
* @brief Set the microphone mute state.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] mute: The mute state to set (0 for unmute, 1 for mute).
*/
void lepl_cap_micp_mute(uint16_t conn_id, uint8_t mute);

/*
* @brief Handle the VCS notification for volume state changes.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_vol: Reference to the volume state data.
*/
void lepl_cap_handle_vcs_notification(lepl_clcb_t *p_clcb, ga_lib_vcs_volume_state_t *p_vol);

/*
* @brief Set the individual microphone mute state.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] instance: The instance of the microphone to mute/unmute.
* @param[in] mute: The mute state to set (0 for unmute, 1 for mute).
*/
void lepl_cap_micp_aics_mute(uint16_t conn_id, uint32_t instance, uint8_t mute);

/*
* @brief Set the individual microphone audio input gain.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] instance: The instance of the microphone to set the gain for.
* @param[in] opcode: The gain control opcode.
* @param[in] input_gain: The input gain value to set.
*/
void lepl_cap_micp_aics_set_gain(uint16_t conn_id, uint32_t instance, uint8_t opcode, int8_t input_gain);
/*================================================================================================================*/


/*=============================================== CSIS ===========================================================*/
/*
* @brief Start or stop the coordinated set member discovery.
*
* @param[in] p_set: Reference to the coordinated set.
* @param[in] start_scan: 1 to start discovery, 0 to stop discovery.
* @return WICED_SUCCESS if the discovery operation was started/stopped successfully, otherwise an error code.
*/
wiced_result_t lepl_start_stop_set_member_discovery(lepl_cordinated_set_t *p_set, uint8_t start_scan);

/*
* @brief Check if the SIRK is zero.
*
* @param[in] sirk: Reference to the SIRK to check.
* @return 1 if the SIRK is zero, otherwise 0.
*/
uint8_t lepl_if_sirk_zero(uint8_t *sirk);

/*
* @brief Check if the device belongs to the coordinated set.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] sirk: Reference to the SIRK to check.
* @return WICED_TRUE if the device belongs to the coordinated set, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_csis_device_belongs_to_coordinated_set(uint16_t conn_id,
                                                         lepl_clcb_t *p_clcb,
                                                         ga_lib_csis_sirk_t sirk);

/*
* @brief Handle the GATT disconnection if device is a member of coordinated set.
*
* @param[in] address: The Bluetooth address of the disconnected device.
*/
void lepl_csis_handle_gatt_disconnection(wiced_bt_device_address_t address);

/*
* @brief Handle the GATT connection if device is a member of coordinated set.
*
* @param[in] conn_id: The connection ID of the GATT connection.
*/
void lepl_csis_handle_gatt_connection(uint16_t conn_id);

/*
* @brief Get the coordinated set.
*
* @param[in] conn_id: The connection ID of the GATT connection ( device is a member of coordinated set).
* @return Reference to the coordinated set, or NULL if not found.
*/
lepl_cordinated_set_t *lepl_csis_get_cordinated_set(uint16_t conn_id);

/*
* @brief Set the lock state of coordinated set.
*
* @param[in] conn_id: The connection ID of the GATT connection ( device is a member of coordinated set).
* @param[in] lock: The lock state to set (0 for unlocked, 1 for locked).
* @return WICED_SUCCESS if the lock state was set successfully, otherwise an error code.
*/
wiced_result_t lepl_csis_set_lock(uint16_t conn_id, uint8_t lock);

/*
* @brief Check if the coordinated set identification service is supported.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @return WICED_TRUE if the device supports CSIS, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_csis_check_is_csis_supported(lepl_clcb_t *p_clcb);

/*
* @brief Handle the received value for CSIS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_csis_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data);
/*================================================================================================================*/


/*================================================== GMAP ========================================================*/
#if GMAP_SERVER_ENABLED
/*
* @brief Initialize the GMAP for the LEPL application. This function is used to initialize the GMAP for the LEPL application.
*/
void lepl_gmap_init();
#endif

/*
* @brief Handle the received value for GMAP characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_gmap_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_data);
/*================================================================================================================*/


/*================================================== HAP =========================================================*/
/*
* @brief Send write request to read preset records to HAS server.
*
* @param[in] conn_id: The connection ID of the GATT connection.
*/
void lepl_rpc_has_cp_read_preset_records(uint16_t conn_id);

/*
* @brief Send write request to set active preset to HAS server.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] opcode: The opcode for the write request.
* @param[in] preset_index: The index of the preset to set as active.
*/
void lepl_rpc_has_cp_set_active_preset(uint16_t conn_id, uint8_t opcode, uint8_t preset_index);

/*
* @brief  Send write request to set the preset name.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] preset_index: The index of the preset for which to set the name.
* @param[in] p_name: Pointer to the preset name string to set.
*/
void lepl_rpc_has_cp_set_preset_name(uint16_t conn_id, uint8_t preset_index, char *p_name);

/*
* @brief Handle the received value for HAS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_has_handle_received_characteristics_value(uint16_t conn_id,
                                                    lepl_clcb_t *p_clcb,
                                                    wiced_bt_gatt_optype_t op,
                                                    wiced_bt_gatt_status_t status,
                                                    wiced_bt_gatt_data_t *p_gatt_data);
/*================================================================================================================*/


/*============================================= ISOC  ============================================================*/
/*
* @brief Setup the BIS data path.
*
* @param[in] conn_hdl: BIS connection handle.
* @param[in] p_csc: Reference to the ASC configuration.
* @return WICED_SUCCESS if the BIS data path was setup successfully, otherwise an error code.
*/
wiced_result_t lepl_isoc_dhm_setup_bis_datapath(uint16_t conn_hdl, ga_lib_ascs_csc_t *p_csc);

/*
* @brief Setup the CIS data path for given ASE.
*
* @param[in] p_ase: Reference to the ASE data for which to setup the CIS data path.
* @return WICED_SUCCESS if the CIS data path was setup successfully, otherwise an error code.
*/
wiced_result_t lepl_isoc_dhm_setup_cis_datapath(lepl_ase_data_t *p_ase);

/*
* @brief Remove the CIS data path for given ASE.
*
* @param[in] p_ase: Reference to the ASE data for which to remove the CIS data path.
* @return WICED_TRUE if the CIS data path was removed successfully, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_isoc_dhm_remove_cis_datapath(lepl_ase_data_t *p_ase);

/*
* @brief Remove the BIS data path for connection handle list.
*
* @param[in] conn_hdl_list: List of connection handles for which to remove the BIS data path.
* @param[in] bis_count: Number of BIS connections to remove.
*/
void lepl_isoc_dhm_remove_bis_datapath(uint16_t *conn_hdl_list, uint8_t bis_count);

/*
* @brief Start the CIS stream for given connection handle and stream direction.
*
* @param[in] conn_hdl: CIS connection handle.
* @param[in] dir: Stream direction (input or output).
*/
void lepl_isoc_dhm_start_cis_stream(uint16_t conn_hdl, wiced_ble_isoc_data_path_direction_t dir);

/*
* @brief Stop the BIS stream for connection handle.
*
* @param[in] conn_hdl: BIS connection handle.
*/
void lepl_isoc_dhm_start_bis_stream(uint16_t conn_hdl);

/*
* @brief Stop the ISO stream.
*
* @param[in] isoc_conn_hdl: ISO connection handle.
*/
void lepl_isoc_dhm_stop_stream(uint16_t isoc_conn_hdl);

/*
* @brief Initialize the ISO module.
*/
void lepl_isoc_init();

/*
* @brief Initialize the ISO data handler module.
*/
void lepl_isoc_dhm_init(void);

/*
* @brief Disable the audio .
*/
void lepl_isoc_dhm_disable_audio(void);
/*================================================================================================================*/


/*============================================= MCS ==============================================================*/
/*
* @brief Play the media.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_audio_config: Reference to the unicast stream configuration for media playback.
* @return WICED_SUCCESS if the media playback was started successfully, otherwise an error code.
*/
wiced_result_t lepl_mcs_play(uint16_t conn_id, lepl_unicast_stream_config_t *p_audio_config);

/*
* @brief Pause the media.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @return WICED_SUCCESS if the media playback was paused successfully, otherwise an error code.
*/
wiced_result_t lepl_mcs_pause(uint16_t conn_id);

/*
* @brief Check if the media is streaming.
*
* @return WICED_TRUE if the media is streaming, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_mcs_is_streaming();

/*
* @brief Initialize the media control service data.
*/
void lepl_mcs_initialize_data();

/*
* @brief Update the media state.
*
* @param[in] state: The new media state to set.
* @return The result of the media state update operation.
*/
ga_lib_mcs_result_t lepl_media_control_service_update_state(ga_lib_mcs_state_t state);

/*
* @brief Handle read request for GMCS/MCS control point.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] handle: The handle of the characteristic to read.
* @param[in] p_data: Pointer to the buffer to store the read data.
* @param[in] p_len: Pointer to the length of the read data.
* @return WICED_SUCCESS if the read request was handled successfully, otherwise an error code.
*/
wiced_bt_gatt_status_t lepl_mcs_handle_read_request(lepl_clcb_t *p_clcb,
                                                    uint16_t handle,
                                                    uint8_t *p_data,
                                                    uint16_t *p_len);

/*
* @brief Handle write request/command for GMCS/MCS control point.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_data: Pointer to the data to write.
* @param[in] len_to_write: The length of the data to write.
* @return WICED_SUCCESS if the write request was handled successfully, otherwise an error code.
*/
wiced_bt_gatt_status_t lepl_gmcs_handle_write_cp(uint16_t conn_id, uint8_t *p_data, uint16_t len_to_write);
/*================================================================================================================*/


/*=================================================== MICS =======================================================*/
/*
* @brief Set the mute state for the microphones.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] mute: The mute state to set (0 for unmute, 1 for mute).
* @return WICED_SUCCESS if the mute state was set successfully, otherwise an error code.
*/
wiced_result_t lepl_mics_set_mute(uint16_t conn_id, uint8_t mute);

/*
* @brief Set the mute state for individual microphone.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] instance: The instance of the microphone to set the mute state for.
* @param[in] mute: The mute state to set (0 for unmute, 1 for mute).
* @return WICED_SUCCESS if the mute state was set successfully, otherwise an error code.
*/
wiced_result_t lepl_mics_aics_set_mute(uint16_t conn_id, uint32_t instance, uint8_t mute);

/*
* @brief Set the input gain for individual microphone.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] instance: The instance of the microphone to set the gain for.
* @param[in] opcode: The gain control opcode.
* @param[in] input_gain: The input gain value to set.
* @return WICED_SUCCESS if the input gain was set successfully, otherwise an error code.
*/
wiced_result_t lepl_mics_aics_set_mute(uint16_t conn_id, uint32_t instance, uint8_t mute);

/*
* @brief Set the input gain for individual microphone.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] instance: The instance of the microphone to set the gain for.
* @param[in] opcode: The gain control opcode.
* @param[in] input_gain: The input gain value to set.
* @return WICED_SUCCESS if the input gain was set successfully, otherwise an error code.
*/
wiced_result_t lepl_mics_aics_set_gain(uint16_t conn_id, uint32_t instance, uint8_t opcode, int8_t input_gain);

/*
* @brief Handle the received value for MICS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_mics_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data);

/*
* @brief Handle the received value for AICS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_mics_aics_handle_received_characteristics_value(uint16_t conn_id,
                                                          lepl_clcb_t *p_clcb,
                                                          wiced_bt_gatt_optype_t op,
                                                          wiced_bt_gatt_status_t status,
                                                          wiced_bt_gatt_data_t *p_gatt_data);

/*
* @brief Handle the received notification for AICS input state.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] instance: The instance of the AICS characteristic for which the notification was received.
* @param[in] p_input: Reference to the AICS input state data received in the notification.
*/
void lepl_cap_handle_mics_aics_notification(lepl_clcb_t *p_clcb, uint8_t instance, ga_lib_aics_input_state_t *p_input);

/*
* @brief Handle the received notification for MICS  mute state.
*
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] p_mics: Reference to the MICS data received in the notification.
*/
void lepl_cap_handle_mics_notification(lepl_clcb_t *p_clcb, lepl_mics_data_t *p_mics);
    /*================================================================================================================*/


/*===================================================== PACS =====================================================*/
/*
* @brief Verify the context type against supported capabilities of peer device.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] ase_type: The ASE type (sink or source).
* @param[in] req_context: The requested context type to verify.
* @return WICED_TRUE if the requested context type is supported by the peer device, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_pacs_verify_context_type(uint16_t conn_id, uint8_t ase_type, uint16_t req_context);

/*
* @brief Verify the audio location against supported capabilities of peer device.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] ase_type: The ASE type (sink or source).
* @param[in] req: The requested audio location to verify.
* @return WICED_TRUE if the requested audio location is supported by the peer device, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_pacs_verify_audio_location(uint16_t conn_id, uint8_t ase_type, ga_lib_pacs_audio_location_t req);

/*
* @brief Verify the codec parameters against supported capabilities of peer device.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] ase_type: The ASE type (sink or source).
* @param[in] p_codec_config: Pointer to the codec configuration parameters to verify.
* @return WICED_TRUE if the codec parameters are supported by the peer device, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_pacs_verify_codec(uint16_t conn_id,
                                    uint8_t ase_type,
                                    ga_lib_ascs_config_codec_args_t *p_codec_config);

/*
* @brief Handle the received value for PACS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_pacs_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data);
/*================================================================================================================*/


/*=================================================== TBS ========================================================*/
/*
* @brief Check if the peer supports ringtone .
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @return WICED_TRUE if the peer supports ringtone, otherwise WICED_FALSE.
*/
wiced_bool_t lepl_ccs_pacs_does_peer_support_ringtone(uint16_t conn_id);

/*
* @brief Set the incoming remote call parameters.
*
* @param[in] p_tbs: Reference to the telephone bearer service data.
* @param[in] uri_scheme: The URI scheme of the incoming call.
* @param[in] friendly_name: The friendly name of the incoming call.
*/
void lepl_tbs_set_incoming_remote_call(lepl_tbs_t *p_tbs, char *uri_scheme, char *friendly_name);

/*
* @brief Set the incoming remote call.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_audio_config: Reference to the unicast stream configuration for the incoming remote call.
* @return WICED_SUCCESS if the incoming remote call was set successfully, otherwise an error code.
*/
wiced_result_t lepl_rpc_ccs_set_incoming_remote_call(uint16_t conn_id, lepl_unicast_stream_config_t *p_audio_config);

/*
* @brief Terminate the call.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] call_id: The call ID of the call to terminate.
* @param[in] termination_reason: The reason for terminating the call.
* @return The result of the call termination operation.
*/
ga_lib_tbs_call_operation_result_t lepl_tbs_terminate_call(uint16_t conn_id,
                                                               uint8_t call_id,
                                                               uint8_t termination_reason);

/*
* @brief Set the call state to remotely hold.
*
* @param[in] call_id: The call ID of the call to set to remotely hold.
*/
void lepl_rpc_ccs_set_remote_hold_call(uint8_t call_id);

/*
* @brief Remove the call from remotely held state.
*
* @param[in] call_id: The call ID of the call to remove from remotely held state.
*/
void lepl_rpc_ccs_set_retrieve_remote_hold_call(uint8_t call_id);

/*
* @brief Initialize the TBS data.
*/
void lepl_tbs_initialize_data();

/*
* @brief Get the call control server state.
*
* @return The current call control server state.
*/
lepl_ccs_states_t lepl_get_call_control_server_state();

/*
* @brief Handle the transition from ringtone to conversation state.
*
* @param[in] p_ase: Reference to the ASE data for which to handle the transition.
*/
void lepl_ccs_isoc_handle_ringtone_to_convo(lepl_ase_data_t *p_ase);

/*
* @brief Start the streaming conversation.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_stream_config: Reference to the unicast stream configuration for the conversation.
*/
void lepl_ccs_start_streaming_convo(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Start the inband ringtone.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_stream_config: Reference to the unicast stream configuration for the inband ringtone.
*/
void lepl_ccs_start_inband_ringtone(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config);

/*
* @brief Get the active call ID.
*
* @return The active call ID, or 0 if there is no active call.
*/
uint8_t lepl_ccs_get_active_call_id(void);

/*
* @brief Handle the received characteristics value for the LEPL application. This function is used to handle the received characteristics value for the LEPL application.
*/
wiced_bt_gatt_status_t lepl_tbs_handle_gatt_read_request(lepl_clcb_t *p_clcb,
                                                         uint16_t handle,
                                                         uint8_t *p_data,
                                                         uint16_t *p_len);

/*
* @brief Handle write request/command for GTBS/TBS control point.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_data: Pointer to the data to write.
* @param[in] len_to_write: The length of the data to write.
* @return WICED_SUCCESS if the write request was handled successfully, otherwise an error code.
*/
wiced_result_t lepl_gtbs_handle_write_cp(uint16_t conn_id, uint8_t *p_data, uint16_t len_to_write);
/*================================================================================================================*/




/*============================================= VCS ==============================================================*/
/*
* @brief Set the volume.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] opcode: The volume control opcode.
* @param[in] abs_vol: The absolute volume value to set.
* @return WICED_SUCCESS if the volume was set successfully, otherwise an error code.
*/
wiced_result_t lepl_vcs_set_volume(uint16_t conn_id, ga_lib_vcs_volume_control_opcodes_t opcode, uint8_t abs_vol);

/*
* @brief Set the mute state.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] mute_state: The mute state to set (0 for unmute, 1 for mute).
* @return WICED_SUCCESS if the mute state was set successfully, otherwise an error code.
*/
wiced_result_t lepl_vcs_set_mute_state(uint16_t conn_id, ga_lib_mute_val_t mute_state);

/*
* @brief Handle the received value for VCS characteristics.
*
* @param[in] conn_id: The connection ID of the GATT connection.
* @param[in] p_clcb: Reference to the connection link control block.
* @param[in] op: The GATT operation type (read or write).
* @param[in] status: The GATT status of the operation.
* @param[in] p_gatt_data: Reference to the GATT data received.
*/
void lepl_vcs_handle_received_characteristics_value(uint16_t conn_id,
                                                    lepl_clcb_t *p_clcb,
                                                    wiced_bt_gatt_optype_t op,
                                                    wiced_bt_gatt_status_t status,
                                                    wiced_bt_gatt_data_t *p_gatt_data);
/*================================================================================================================*/


/******************************************************************************************************************/


#if defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
#define LE_PL_BR_CONFIG_DATA_SIZE 32 /**< Broadcast configuration data size */

/*
* @brief Transcoding data structure.
*/
typedef struct
{
    uint8_t broadcast_config[LE_PL_BR_CONFIG_DATA_SIZE]; /**< Broadcast configuration data */
    uint16_t conn_id;                                    /**< Connection ID */
    uint8_t broadcast_config_len;                        /**< Broadcast configuration length */
    uint8_t is_unicast;                                  /**< Is unicast flag */
    uint8_t in_use;                                      /**< In use flag */
} le_pl_transcoding_data_t;

extern le_pl_transcoding_data_t le_pl_transcoding_data;

/*
* @brief Start or stop the transcoding streaming.
*
* @param[in] start: 1 to start transcoding streaming, 0 to stop it.
*/
void lepl_transcoding_start_streaming(wiced_bool_t start);

/*
* @brief Save the broadcast configuration.
*
* @param[in] p_data: Pointer to the broadcast configuration data to save.
* @param[in] data_len: The length of the broadcast configuration data.
*/
void lepl_transcoding_save_broadcast_config(uint8_t *p_data, uint32_t data_len);
#endif // defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)


extern wiced_bt_cfg_settings_t lepl_cfg_settings;
#endif
