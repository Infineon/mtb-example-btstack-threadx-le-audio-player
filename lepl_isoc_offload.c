/*
* $ Copyright Cypress Semiconductor $
*/

/* App Library includes */
#ifdef COMPONENT_audiomanager
#include "wiced_audio_manager.h"
#endif // COMPONENT_audiomanager
#ifdef COMPONENT_cyw9bt_audio5
#include "wiced_bt_codec_cs47l35.h"
#endif // COMPONENT_cyw9bt_audio5

/* BT Stack includes */
#include "wiced_bt_audio.h"
#include "wiced_bt_cfg.h"
#include "ga_lib_ascs.h"
#include "ga_lib_bap.h"
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
/* Application includes */
#include "lepl.h"

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
//#define ARIP_I2S_DATA_PATH_ID (1 << 5)
#define ARIP_I2S_DATA_PATH_ID 0x60

#define ARIP_MXTDM_0_DATA_PATH_ID 0x80 //4
#define ARIP_MXTDM_1_DATA_PATH_ID 0xC0 //6
#define ARIP_MXTDM_FIND_ALL_MASK 0x1f
#define HCI_OFFLOAD  0X00
#define ARIP_MXTDM_I2S 0xA0
#define HCI_OFFLOAD_1_chL_DATA_PATH_ID HCI_OFFLOAD             // 0000 0000
#define HCI_OFFLOAD_1_chR_DATA_PATH_ID (HCI_OFFLOAD | 1)       // 0000 0001
#define ARIP_MXTDM_I2S_1_chL_DATA_PATH_ID ARIP_MXTDM_I2S       // 0000 00A0
#define ARIP_MXTDM_I2S_1_chR_DATA_PATH_ID (ARIP_MXTDM_I2S | 1) // 0000 00A1
#define CODEC_SPECIFIC_CONFIGURATION_LEN 11


#ifndef AUDIO_TX_BUFFER_SIZE
#define AUDIO_TX_BUFFER_SIZE 14336
#endif // AUDIO_TX_BUFFER_SIZE

#ifndef AUDIO_CODEC_BUFFER_SIZE
#define AUDIO_CODEC_BUFFER_SIZE 0x2000
#endif //AUDIO_CODEC_BUFFER_SIZE

#ifndef AUDIO_TX_BUFFER_WATERMARK_LEVEL
#define AUDIO_TX_BUFFER_WATERMARK_LEVEL 70
#endif //AUDIO_TX_BUFFER_WATERMARK_LEVEL

#define AUDIO_ROUTE_UART 0x01
#define HS_DEFAULT_VOLUME 50


wiced_bool_t audio_started = WICED_FALSE;
static ga_lib_ascs_csc_t *p_stream_csc;
static int16_t stream_id;

#ifdef COMPONENT_audiomanager
extern int32_t audio_driver_config_frequency(int32_t sampling_rate,
                                             int32_t no_of_channels,
                                             int32_t bits_per_sample,
                                             am_audio_io_device_t sink,
                                             uint32_t stream_type);
extern void audio_driver_init_vol();
#endif // COMPONENT_audiomanager

extern void wiced_le_audio_stop(uint16_t conn_hdl, uint8_t is_cis, wiced_ble_isoc_data_path_bit_t direction);
extern wiced_result_t wiced_le_audio_enable(uint8_t enable);
extern wiced_result_t wiced_le_audio_start(uint16_t conn_hdl,
                                           uint16_t audio_route,
                                           wiced_ble_isoc_data_path_direction_t direction,
                                           wiced_ble_isoc_data_path_id_t data_path_id,
                                           uint8_t stereo,
                                           uint8_t is_cis,
                                           ga_lib_ascs_csc_t *p_csc);



wiced_bt_audio_config_buffer_t lepl_audio_buf_config = {
    .role = WICED_AUDIO_SOURCE_ROLE
#ifdef HS_SPK_ENABLED
          | WICED_HF_ROLE
#endif // HS_SPK_ENABLED
	,
    .audio_tx_buffer_size = AUDIO_TX_BUFFER_SIZE,
    .audio_codec_buffer_size = AUDIO_CODEC_BUFFER_SIZE,
    .audio_tx_buffer_watermark_level = AUDIO_TX_BUFFER_WATERMARK_LEVEL
};

void lepl_isoc_dhm_init(void)
{

    /* required to call this for ARIP/I2S configuration */
    wiced_result_t res = wiced_audio_buffer_initialize(lepl_audio_buf_config);
    WICED_BT_TRACE("wiced_audio_buffer_initialize res : %d \n", res);

}

void lepl_isoc_dhm_reinit(uint8_t device_role)
{
    if (lepl_audio_buf_config.role == device_role) return;

    if (device_role == WICED_AUDIO_SOURCE_ROLE)
    {
#ifdef COMPONENT_audiomanager
        if (wiced_am_stream_close(stream_id) != WICED_SUCCESS)
            WICED_BT_TRACE_CRIT("Err: wiced_am_stream_close\n");
#endif // COMPONENT_audiomanager

        lepl_audio_buf_config.role = device_role;
        lepl_audio_buf_config.audio_tx_buffer_size = AUDIO_TX_BUFFER_SIZE;
        lepl_audio_buf_config.audio_codec_buffer_size = AUDIO_CODEC_BUFFER_SIZE;
        lepl_audio_buf_config.audio_tx_buffer_watermark_level = AUDIO_TX_BUFFER_WATERMARK_LEVEL;
        wiced_result_t res = wiced_audio_buffer_initialize(lepl_audio_buf_config);
        WICED_BT_TRACE("wiced_audio_buffer_initialize res : %d \n", res);
    }
    else if (device_role == WICED_HF_ROLE)
    {
        lepl_isoc_dhm_stop_stream(0);
        lepl_isoc_dhm_disable_audio();
        lepl_audio_buf_config.role = device_role;
        lepl_audio_buf_config.audio_tx_buffer_size = 0x10000;
        lepl_audio_buf_config.audio_codec_buffer_size = 0x10000;
        lepl_audio_buf_config.audio_tx_buffer_watermark_level = 50;
        wiced_result_t res = wiced_audio_buffer_initialize(lepl_audio_buf_config);
        WICED_BT_TRACE("wiced_audio_buffer_initialize res : %d \n", res);
#ifdef COMPONENT_audiomanager
        wiced_am_init();
#endif // COMPONENT_audiomanager
    }
}

static uint32_t count_set_bits(uint32_t n)
{
    uint32_t count = 0;
    while (n)
    {
        n &= (n - 1);
        count++;
    }
    return count;
}

static wiced_result_t lepl_isoc_dhm_get_csc(uint8_t *p_csc_field, const ga_lib_ascs_csc_t *p_csc)
{
    uint8_t csc_sampling_freq = 0, frame_duration = 0;

    if (!p_csc) return WICED_BADARG;

    if (!p_csc->sampling_frequency_hz || !p_csc->frame_duration_us || !p_csc->octets_per_codec_frame) return WICED_ERROR;

    WICED_BT_TRACE("[%s] sampling frequency: %d frame_duration: %d octets_per_codec_frame: %d \n",
                   __FUNCTION__,
                   p_csc->sampling_frequency_hz,
                   p_csc->frame_duration_us,
                   p_csc->octets_per_codec_frame);

    switch (p_csc->sampling_frequency_hz)
    {
    case 8000:
    case 16000:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        csc_sampling_freq = 1 << ((p_csc->sampling_frequency_hz / 8000) - 1);
        break;

    default:
        WICED_BT_TRACE("[%s] Unsupported sampling frequency: %d\n", __FUNCTION__, p_csc->sampling_frequency_hz);
        return WICED_UNSUPPORTED;
        break;
    }

    switch (p_csc->frame_duration_us)
    {
    case 7500:
    case 10000:
        frame_duration = ga_lib_bap_get_frame_duration_index(p_csc->frame_duration_us) + 1;
        break;

    default:
        WICED_BT_TRACE("[%s] Unsupported frame duration: %d\n", __FUNCTION__, p_csc->frame_duration_us);
        return WICED_UNSUPPORTED;
        break;
    }

    uint8_t csc[] = {3,                                        // Length
                     BAP_CODEC_CONFIG_SAMPLING_FREQUENCY_TYPE, // Type
                     csc_sampling_freq,                        // (bit pos + 1) * 8khz
                     0,
                     2,                                    // Length
                     BAP_CODEC_CONFIG_FRAME_DURATION_TYPE, // Type
                     frame_duration,
                     3,                                            // Length
                     BAP_CODEC_CONFIG_OCTETS_PER_CODEC_FRAME_TYPE, // Type
                     p_csc->octets_per_codec_frame,
                     0};

    WICED_MEMCPY(p_csc_field, csc, CODEC_SPECIFIC_CONFIGURATION_LEN);

    return WICED_SUCCESS;
}

#ifndef CTLR_DELAY
#define CTLR_DELAY 35000
#endif
wiced_result_t lepl_isoc_dhm_setup_cis_datapath(lepl_ase_data_t *p_ase)
{
    uint8_t csc[CODEC_SPECIFIC_CONFIGURATION_LEN];
    int32_t numOfChannels = 0;
    wiced_result_t ret;
    uint16_t datapath_dir = p_ase->ase.data_path_dir;
    ga_lib_ascs_csc_t * p_csc = &p_ase->ase.ase_cfg.csc;

    ret = lepl_isoc_dhm_get_csc(csc, p_csc);
    if (ret) return ret;

    p_stream_csc = p_csc;

    if (p_csc->audio_channel_allocation)
    {
        numOfChannels = count_set_bits(p_csc->audio_channel_allocation);
        if (numOfChannels > 2)
        {
            return WICED_UNSUPPORTED;
        }
    }

    lepl_audio_mode_t audio_mode = lepl_cap_get_audio_mode();
    if (audio_mode == LEPL_AUDIO_MODE_IN_TRANSIT)
        audio_mode = lepl_cap_get_final_audio_mode();

    if (audio_mode == LEPL_AUDIO_MODE_MEDIA ||
        lepl_get_call_control_server_state() == CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE)
    {
        lepl_isoc_dhm_reinit(WICED_AUDIO_SOURCE_ROLE);
        wiced_le_audio_enable(1);
    }
    else
    {
        lepl_isoc_dhm_reinit(WICED_HF_ROLE);
#ifdef COMPONENT_audiomanager
        stream_id = audio_driver_config_frequency(p_csc->sampling_frequency_hz,
                                                  numOfChannels,
                                                  DEFAULT_BITSPSAM,
                                                  AM_HEADPHONES,
                                                  HFP);
#endif
    }

    wiced_ble_isoc_setup_data_path_info_t iso_audio_param_data= {0};
    uint8_t codec_id[5] = {0x06, 0x00, 0x00, 0x00, 0x00};

    iso_audio_param_data.isoc_conn_hdl = p_ase->cis_conn_handle;
    iso_audio_param_data.p_app_ctx = p_ase;
    iso_audio_param_data.data_path_dir = datapath_dir;
    iso_audio_param_data.controller_delay = CTLR_DELAY;
    iso_audio_param_data.csc_length = sizeof(csc);
    iso_audio_param_data.p_csc = csc;
    memcpy(iso_audio_param_data.codec_id, codec_id, sizeof(codec_id));
    switch (p_csc->audio_channel_allocation)
    {
    case BAP_AUDIO_LOCATION_FRONT_LEFT:

#ifdef HS_SPK_ENABLED
#ifdef NO_HCI_OFFLOAD
	iso_audio_param_data.data_path_id = ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID;
#else
	iso_audio_param_data.data_path_id = (lepl_audio_buf_config.role == WICED_AUDIO_SOURCE_ROLE) ? HCI_OFFLOAD_1_chL_DATA_PATH_ID
																	   : ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID;
#endif // NO_HCI_OFFLOAD
#else
        iso_audio_param_data.data_path_id = (lepl_audio_buf_config.role == WICED_AUDIO_SOURCE_ROLE)
                                                ? HCI_OFFLOAD_1_chL_DATA_PATH_ID
                                                : ARIP_MXTDM_I2S_1_chL_DATA_PATH_ID;
#endif
        wiced_ble_isoc_configure_data_path(datapath_dir, iso_audio_param_data.data_path_id);
        break;
    case BAP_AUDIO_LOCATION_FRONT_RIGHT:
#ifdef HS_SPK_ENABLED
#ifdef NO_HCI_OFFLOAD
	iso_audio_param_data.data_path_id = ARIP_MXTDM_LE_AUDIO_CH_R_DATA_PATH_ID;
#else
    iso_audio_param_data.data_path_id = (lepl_audio_buf_config.role == WICED_AUDIO_SOURCE_ROLE) ? HCI_OFFLOAD_1_chR_DATA_PATH_ID
																: ARIP_MXTDM_LE_AUDIO_CH_R_DATA_PATH_ID;
#endif // NO_HCI_OFFLOAD
#else
        iso_audio_param_data.data_path_id = (lepl_audio_buf_config.role == WICED_AUDIO_SOURCE_ROLE)
                                                ? HCI_OFFLOAD_1_chR_DATA_PATH_ID
                                                                : ARIP_MXTDM_I2S_1_chR_DATA_PATH_ID;
#endif // HS_SPK_ENABLED

        wiced_ble_isoc_configure_data_path(datapath_dir, iso_audio_param_data.data_path_id);
        break;
    case BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT:
        if (lepl_audio_buf_config.role == WICED_AUDIO_SOURCE_ROLE)
        {
#ifdef HS_SPK_ENABLED
#ifdef NO_HCI_OFFLOAD
           iso_audio_param_data.data_path_id = (ARIP_MXTDM_TDM_1 | ARIP_MXTDM_FIND_ALL_MASK);
           wiced_ble_isoc_configure_data_path(datapath_dir, ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID);
           wiced_ble_isoc_configure_data_path(datapath_dir, ARIP_MXTDM_LE_AUDIO_CH_R_DATA_PATH_ID);
#else
           iso_audio_param_data.data_path_id = (HCI_OFFLOAD | ARIP_MXTDM_FIND_ALL_MASK);
           wiced_ble_isoc_configure_data_path(datapath_dir, HCI_OFFLOAD_1_chL_DATA_PATH_ID);
           wiced_ble_isoc_configure_data_path(datapath_dir, HCI_OFFLOAD_1_chR_DATA_PATH_ID);

#endif // NO_HCI_OFFLOAD
#else
           iso_audio_param_data.data_path_id = (HCI_OFFLOAD | ARIP_MXTDM_FIND_ALL_MASK);
           wiced_ble_isoc_configure_data_path(datapath_dir, HCI_OFFLOAD_1_chL_DATA_PATH_ID);
           wiced_ble_isoc_configure_data_path(datapath_dir, HCI_OFFLOAD_1_chR_DATA_PATH_ID);
#endif // HS_SPK_ENABLED
        }
        else
        {
#ifdef HS_SPK_ENABLED
#ifdef NO_HCI_OFFLOAD
            iso_audio_param_data.data_path_id = (ARIP_MXTDM_TDM_1 | ARIP_MXTDM_FIND_ALL_MASK);
           wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID);
           wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_LE_AUDIO_CH_L_DATA_PATH_ID);
#else
            iso_audio_param_data.data_path_id = (ARIP_MXTDM_TDM_1 | ARIP_MXTDM_FIND_ALL_MASK);
            wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_I2S_1_chL_DATA_PATH_ID);
            wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_I2S_1_chR_DATA_PATH_ID);
#endif // NO_HCI_OFFLOAD
#else
            iso_audio_param_data.data_path_id = (ARIP_MXTDM_I2S | ARIP_MXTDM_FIND_ALL_MASK);
            wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_I2S_1_chL_DATA_PATH_ID);
            wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, ARIP_MXTDM_I2S_1_chR_DATA_PATH_ID);
#endif
        }
        break;
    default:
        break;
    }

    // setup ISO data path and LC3 codec for INPUT from controller or OUTPUT to controller
    ret = wiced_ble_isoc_setup_data_path(&iso_audio_param_data);
    WICED_BT_TRACE("[%s] Datapath setup res 0x%x", __FUNCTION__, ret);
    return ret;
}

wiced_result_t lepl_isoc_dhm_setup_bis_datapath(uint16_t conn_hdl, ga_lib_ascs_csc_t *p_csc)
{
    uint8_t csc[CODEC_SPECIFIC_CONFIGURATION_LEN];
    uint32_t numOfChannels = 0;
    wiced_result_t ret;

    ret = lepl_isoc_dhm_get_csc(csc, p_csc);
    if (ret != WICED_SUCCESS)
        return ret;

    p_stream_csc = p_csc;

	if(p_csc->audio_channel_allocation == 0){
		p_csc->audio_channel_allocation = 1;
	}
    if (p_csc->audio_channel_allocation)
    {
        numOfChannels = count_set_bits(p_csc->audio_channel_allocation);
        if (numOfChannels > 2)
        {
            return WICED_UNSUPPORTED;
        }
    }
    lepl_isoc_dhm_reinit(WICED_AUDIO_SOURCE_ROLE);
    wiced_le_audio_enable(1);

    wiced_ble_isoc_setup_data_path_info_t iso_bis_audio_param_data = {0};
    uint8_t codec_id[5] = {0x06, 0x00, 0x00, 0x00, 0x00};

    iso_bis_audio_param_data.p_app_ctx = NULL;
    iso_bis_audio_param_data.isoc_conn_hdl = conn_hdl;
    iso_bis_audio_param_data.data_path_dir = WICED_BLE_ISOC_DPD_INPUT;
    iso_bis_audio_param_data.data_path_id = (HCI_OFFLOAD | ARIP_MXTDM_FIND_ALL_MASK);
    iso_bis_audio_param_data.controller_delay = CTLR_DELAY;
    iso_bis_audio_param_data.csc_length = sizeof(csc);
    iso_bis_audio_param_data.p_csc = csc;
    memcpy(iso_bis_audio_param_data.codec_id, codec_id, sizeof(codec_id));
    switch (p_csc->audio_channel_allocation)
    {
    case BAP_AUDIO_LOCATION_FRONT_LEFT:
        wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, HCI_OFFLOAD_1_chL_DATA_PATH_ID);
        iso_bis_audio_param_data.data_path_id = HCI_OFFLOAD_1_chL_DATA_PATH_ID;
        break;
    case BAP_AUDIO_LOCATION_FRONT_RIGHT:
        wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, HCI_OFFLOAD_1_chR_DATA_PATH_ID);
        iso_bis_audio_param_data.data_path_id = HCI_OFFLOAD_1_chR_DATA_PATH_ID;
        break;
    case BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT:
        wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, HCI_OFFLOAD_1_chL_DATA_PATH_ID);
        wiced_ble_isoc_configure_data_path(WICED_BLE_ISOC_DPD_INPUT, HCI_OFFLOAD_1_chR_DATA_PATH_ID);
        break;
    default:
        break;

    }

    ret = wiced_ble_isoc_setup_data_path(&iso_bis_audio_param_data);
    WICED_BT_TRACE("[%s] Datapath setup 0x%x", __FUNCTION__, ret);
    if (ret != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

void lepl_isoc_dhm_start_cis_stream(uint16_t conn_hdl, wiced_ble_isoc_data_path_direction_t dir_bit)
{
    if (lepl_audio_buf_config.role == WICED_HF_ROLE)
    {
#ifdef COMPONENT_audiomanager
        if (WICED_SUCCESS != wiced_am_stream_start(stream_id))
        {
            WICED_BT_TRACE_CRIT("wiced_am_stream_start failed\n");
        }
#endif // COMPONENT_audiomanager

        audio_driver_init_vol();
        audio_driver_set_volume(50);
    }
    else
    {
        if (audio_started == WICED_TRUE) return;

#ifndef NO_HCI_OFFLOAD
#ifndef AUDIO_TRANSCODING

        lepl_rpc_send_audio_started_event();
		wiced_le_audio_start(
			conn_hdl,
			AUDIO_ROUTE_UART,
			WICED_BLE_ISOC_DPD_INPUT,
			WICED_BLE_ISOC_DPID_HCI,
			TRUE, //Wiced HCI host is expected to send  L/R interleaved PCM data for stereo & mono config
			TRUE,
			p_stream_csc);
#endif // !AUDIO_TRANSCODING
#endif // !NO_HCI_OFFLOAD
        audio_started = WICED_TRUE;
    }
}

void lepl_isoc_dhm_start_bis_stream(uint16_t conn_hdl)
{
    if (lepl_audio_buf_config.role == WICED_HF_ROLE)
    {
#ifdef COMPONENT_audiomanager
        if (WICED_SUCCESS != wiced_am_stream_start(stream_id))
        {
            WICED_BT_TRACE_CRIT("wiced_am_stream_start failed\n");
        }
#endif

        audio_driver_init_vol();
        audio_driver_set_volume(HS_DEFAULT_VOLUME);
        return;
    }
    else
    {

        if (audio_started == WICED_TRUE) return;

#ifndef AUDIO_TRANSCODING

        lepl_rpc_send_audio_started_event();
        wiced_le_audio_start(
            conn_hdl,
            AUDIO_ROUTE_UART,
            WICED_BLE_ISOC_DPD_INPUT,
            WICED_BLE_ISOC_DPID_HCI,
            TRUE, //Wiced HCI host is expected to send  L/R interleaved PCM data for stereo & mono config
            FALSE,
            p_stream_csc);
#endif // !AUDIO_TRANSCODING
        audio_started = WICED_TRUE;
    }
}

void lepl_isoc_dhm_stop_stream(uint16_t isoc_conn_hdl)
{
    if (lepl_audio_buf_config.role == WICED_HF_ROLE)
    {
#ifdef COMPONENT_audiomanager
        if (WICED_SUCCESS != wiced_am_stream_close(stream_id))
        {
            WICED_BT_TRACE_CRIT("wiced_am_stream_start failed\n");
        }
#endif // COMPONENT_audiomanager
    }
    else
    {
        if (audio_started == WICED_FALSE) return;
#ifndef AUDIO_TRANSCODING
        wiced_le_audio_stop(isoc_conn_hdl, WICED_TRUE, WICED_BLE_ISOC_DPD_INPUT_BIT);
        app_rpc_send_data(HCI_CONTROL_LE_AUDIO_EVENT_STOPPED, NULL, 0);
#endif // !AUDIO_TRANSCODING
        audio_started = WICED_FALSE;
    }
}

wiced_bool_t lepl_isoc_dhm_remove_cis_datapath(lepl_ase_data_t *p_ase)
{
    wiced_result_t res = WICED_ERROR;
    wiced_ble_isoc_data_path_bit_t dir = (p_ase->ase.data_path_dir == WICED_BLE_ISOC_DPD_INPUT)
                                             ? WICED_BLE_ISOC_DPD_INPUT_BIT
                                             : WICED_BLE_ISOC_DPD_OUTPUT_BIT;

    res = wiced_ble_isoc_remove_data_path(p_ase->cis_conn_handle, dir, p_ase);
    if (res)
    {
        WICED_BT_TRACE_CRIT("[%s] res %d\n", __FUNCTION__, res);
        return WICED_FALSE;
    }
    return WICED_TRUE;
}

void lepl_isoc_dhm_remove_bis_datapath(uint16_t *conn_hdl_list, uint8_t bis_count)
{
    for (int i = 0; i < bis_count; i++)
    {
        if (!wiced_ble_isoc_is_bis_created(conn_hdl_list[i])) continue;
        wiced_result_t ret = wiced_ble_isoc_remove_data_path(conn_hdl_list[i], WICED_BLE_ISOC_DPD_INPUT_BIT, NULL);
        if (ret)
        {
            WICED_BT_TRACE_CRIT("[%s] ret %d\n", __FUNCTION__, ret);
        }
    }
}

void lepl_isoc_dhm_disable_audio(void)
{
    wiced_le_audio_enable(0);
}

void lepl_ccs_isoc_handle_ringtone_to_convo(lepl_ase_data_t *p_ase)
{
    lepl_isoc_dhm_remove_cis_datapath(p_ase);
    // HFP supports mono audio
    if (count_set_bits(p_ase->ase.ase_cfg.csc.audio_channel_allocation) > 1)
    {
        p_ase->ase.ase_cfg.csc.audio_channel_allocation = BAP_AUDIO_LOCATION_FRONT_LEFT;
    }
    lepl_isoc_dhm_setup_cis_datapath(p_ase);
}
