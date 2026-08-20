/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"
#ifdef HS_SPK_ENABLED
void lepl_cap_update_cig_sync(uint16_t delay_limit);
#endif // HS_SPK_ENABLED

// Helper to always get a set (real or temporary single-device)
static lepl_cordinated_set_t *get_cordinated_set(uint16_t conn_id, lepl_cordinated_set_t *temp_set)
{
    lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(conn_id);

    WICED_BT_TRACE("[%s] p_set 0x%x", __FUNCTION__, p_set);
    if (!p_set)
    {
        WICED_BT_TRACE("[%s] temp set used for conn_id %d", __FUNCTION__, conn_id);
        temp_set->member_list[0].conn_id = conn_id;
        temp_set->member_list[0].rank = 1;
        temp_set->num_devices = 1;
        temp_set->set_size = 1;
        return temp_set;
    }
    return p_set;
}

static void set_volume_cb(uint16_t conn_id, void *context)
{
    struct
    {
        ga_lib_vcs_volume_control_opcodes_t opcode;
        uint8_t abs_vol;
    } *p = context;
    lepl_vcs_set_volume(conn_id, p->opcode, p->abs_vol);
}

static void set_mute_cb(uint16_t conn_id, void *context)
{
    ga_lib_mute_val_t mute_state = *(ga_lib_mute_val_t *)context;
    lepl_vcs_set_mute_state(conn_id, mute_state);
}

static void set_micp_mute_cb(uint16_t conn_id, void *context)
{
    uint8_t mute = *(uint8_t *)context;
    lepl_mics_set_mute(conn_id, mute);
}

static void set_aics_mute_cb(uint16_t conn_id, void *context)
{
    struct
    {
        uint32_t instance;
        uint8_t mute;
    } *p = context;
    lepl_mics_aics_set_mute(conn_id, p->instance, p->mute);
}

static void set_aics_gain_cb(uint16_t conn_id, void *context)
{
    struct
    {
        uint32_t instance;
        uint8_t opcode;
        int8_t input_gain;
    } *p = context;
    lepl_mics_aics_set_gain(conn_id, p->instance, p->opcode, p->input_gain);
}

static void for_each_device(lepl_cordinated_set_t *p_set, void (*func)(uint16_t conn_id, void *context), void *context)
{
    for (int i = 0; i < p_set->set_size; i++)
    {
        func(p_set->member_list[i].conn_id, context);
    }
}

// VCS
void lepl_cap_vcp_set_volume(uint16_t conn_id, ga_lib_vcs_volume_control_opcodes_t opcode, uint8_t abs_vol)
{
    WICED_BT_TRACE("[%s] conn id %d opcode %d abs_vol %d", __FUNCTION__, conn_id, opcode, abs_vol);
    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);

    struct { ga_lib_vcs_volume_control_opcodes_t opcode; uint8_t abs_vol; } ctx = { opcode, abs_vol };
    for_each_device(p_set, set_volume_cb, &ctx);
}

void lepl_cap_vcp_set_mute_state(uint16_t conn_id, ga_lib_mute_val_t mute_state)
{
    WICED_BT_TRACE("[%s] conn id %d mute %d", __FUNCTION__, conn_id, mute_state);
    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    for_each_device(p_set, set_mute_cb, &mute_state);
}

static void sync_volume_state(lepl_clcb_t *p_clcb, ga_lib_vcs_volume_state_t *p_vol)
{
    if (p_clcb->waiting_for_vcs_notif)
    {
        return;
    }

    if (p_clcb->vcs.volume_state.mute_state != p_vol->mute_state)
    {
        lepl_vcs_set_mute_state(p_clcb->conn_id, p_vol->mute_state);
    }
    else if (p_clcb->vcs.volume_state.volume_setting != p_vol->volume_setting)
    {
        lepl_vcs_set_volume(p_clcb->conn_id, VOLUME_CONTROL_OPCODE_SET_ABSOLUTE_VOLUME, p_vol->volume_setting);
    }
}

void lepl_cap_handle_vcs_notification(lepl_clcb_t *p_clcb, ga_lib_vcs_volume_state_t *p_vol)
{
    p_clcb->waiting_for_vcs_notif = 0;
    lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(p_clcb->conn_id);
    if (p_set == NULL)
    {
        return;
    }

    WICED_MEMCPY(&p_set->set_volume_state, p_vol, sizeof(ga_lib_vcs_volume_state_t));
    for (int i = 0; i < p_set->set_size; i++)
    {
        uint16_t member_conn_id = p_set->member_list[i].conn_id;
        if (p_set->member_list[i].is_used && (member_conn_id != p_clcb->conn_id))
        {
            lepl_clcb_t *p_member_clcb = lepl_gatt_get_clcb_by_conn_id(member_conn_id);
            sync_volume_state(p_member_clcb, &p_set->set_volume_state);
        }
    }
}

//MICS
void lepl_cap_micp_mute(uint16_t conn_id, uint8_t mute)
{
    WICED_BT_TRACE("[%s] conn id %d mute %d", __FUNCTION__, conn_id, mute);
    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    for_each_device(p_set, set_micp_mute_cb, &mute);
}

static void sync_mics_mute_state(lepl_clcb_t *p_clcb, lepl_mics_data_t *p_mics)
{
    if (p_clcb->waiting_for_mics_notif)
    {
        return;
    }

    if (p_clcb->mics.mute_state != p_mics->mute_state)
    {
        lepl_mics_set_mute(p_clcb->conn_id, p_mics->mute_state);
    }
}

void lepl_cap_handle_mics_notification(lepl_clcb_t *p_clcb, lepl_mics_data_t *p_mics)
{
    p_clcb->waiting_for_mics_notif = 0;
    lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(p_clcb->conn_id);
    if (p_set == NULL)
    {
        return;
    }

    WICED_MEMCPY(&p_set->set_mics_state, p_mics, sizeof(lepl_mics_data_t));
    for (int i = 0; i < p_set->set_size; i++)
    {
        uint16_t member_conn_id = p_set->member_list[i].conn_id;
        if (p_set->member_list[i].is_used && (member_conn_id != p_clcb->conn_id))
        {
            lepl_clcb_t *p_member_clcb = lepl_gatt_get_clcb_by_conn_id(member_conn_id);
            sync_mics_mute_state(p_member_clcb, &p_set->set_mics_state);
        }
    }
}

void lepl_cap_micp_aics_mute(uint16_t conn_id, uint32_t instance, uint8_t mute)
{
    WICED_BT_TRACE("[%s] conn id %d instance %d mute %d", __FUNCTION__, conn_id, instance, mute);

    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    struct
    {
        uint32_t instance;
        uint8_t mute;
    } ctx = {instance, mute};
    for_each_device(p_set, set_aics_mute_cb, &ctx);
}

void lepl_cap_micp_aics_set_gain(uint16_t conn_id, uint32_t instance, uint8_t opcode, int8_t input_gain)
{
    WICED_BT_TRACE("[%s] conn id %d instance %d opcode %d input_gain %d",
                   __FUNCTION__,
                   conn_id,
                   instance,
                   opcode,
                   input_gain);

    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    struct
    {
        uint32_t instance;
        uint8_t opcode;
        uint8_t input_gain;
    } ctx = {instance, opcode, input_gain};
    for_each_device(p_set, set_aics_gain_cb, &ctx);
}

static void sync_aics_input_state(lepl_clcb_t *p_clcb, ga_lib_aics_input_state_t *p_input, uint8_t instance)
{
    if (p_clcb->waiting_for_aics_notif[instance])
    {
        return;
    }

    if (p_clcb->mics_aics[instance].input_state.mute_mode != p_input->mute_mode)
    {
        lepl_mics_aics_set_mute(p_clcb->conn_id, instance, p_input->mute_mode);
    }
    else if (p_clcb->mics_aics[instance].input_state.gain_setting != p_input->gain_setting)
    {
        lepl_mics_aics_set_gain(p_clcb->conn_id,
                                instance,
                                HCI_CONTROL_LEA_MICS_AICS_GAIN_INCREMENT,
                                p_input->gain_setting);
    }
}

void lepl_cap_handle_mics_aics_notification(lepl_clcb_t *p_clcb, uint8_t instance, ga_lib_aics_input_state_t *p_input)
{
    p_clcb->waiting_for_aics_notif[instance] = 0;
    lepl_cordinated_set_t *p_set = lepl_csis_get_cordinated_set(p_clcb->conn_id);
    if (p_set == NULL)
    {
        return;
    }

    WICED_MEMCPY(&p_set->set_aics_input_state[instance], p_input, sizeof(ga_lib_aics_input_state_t));
    for (int i = 0; i < p_set->set_size; i++)
    {
        uint16_t member_conn_id = p_set->member_list[i].conn_id;
        if (p_set->member_list[i].is_used && (member_conn_id != p_clcb->conn_id))
        {
            lepl_clcb_t *p_member_clcb = lepl_gatt_get_clcb_by_conn_id(member_conn_id);
            sync_aics_input_state(p_member_clcb, &p_set->set_aics_input_state[instance], instance);
        }
    }
}

//ASCS
void lepl_get_codec_config_param(uint8_t ase_type,
                                 lepl_unicast_stream_config_t *p_stream_config,
                                 ga_lib_ascs_config_codec_args_t *p_param)
{
    lepl_audio_config_t *p_audio_config =
        (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? &p_stream_config->in_cfg : &p_stream_config->out_cfg;

    if (p_stream_config->context_type == BAP_CONTEXT_TYPE_GAME)
    {
        p_audio_config->target_latency = 1;
        p_audio_config->rtn = 1;
        p_audio_config->mtl = (p_audio_config->sdu_interval == 7500) ? 15 : 20;
        p_audio_config->presentation_delay = (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? 10000 : 60000;
    }
    else
    {
        p_audio_config->target_latency = 2;
        p_audio_config->mtl = (p_audio_config->sdu_interval == 7500) ? 75 : 100;
        p_audio_config->rtn = 5;
        p_audio_config->presentation_delay = 40000;
    }

    //Codec param
    p_param->codec_id.coding_format = 0x06;
    p_param->codec_id.company_id = 0;
    p_param->codec_id.vendor_specific_codec_id = 0;
    p_param->csc.sampling_frequency_hz = p_audio_config->sampling_frequency;
    p_param->csc.frame_duration_us = p_audio_config->frame_duration;
    p_param->csc.audio_channel_allocation = BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT;
    p_param->csc.octets_per_codec_frame = p_audio_config->octets_per_codec_frame;
    p_param->csc.lc3_blocks_per_sdu = 1;
    p_param->target_phy = WICED_BLE_ISOC_LE_2M_PHY;
    p_param->target_latency = p_audio_config->target_latency;
    WICED_BT_TRACE("[%s] sf %d fd %d opcf %d tl %d",
                   __FUNCTION__,
                   p_param->csc.sampling_frequency_hz,
                   p_param->csc.frame_duration_us,
                   p_param->csc.octets_per_codec_frame,
                   p_param->target_latency);
}

void lepl_get_qos_config_param(uint8_t ase_type,
                               lepl_unicast_stream_config_t *p_stream_config,
                               ga_lib_ascs_config_qos_args_t *p_param)
{
    lepl_audio_config_t *p_audio_config =
        (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE) ? &p_stream_config->in_cfg : &p_stream_config->out_cfg;

    //QoS param
    p_param->framing = p_stream_config->framing;
    p_param->max_transport_latency = p_audio_config->mtl;
    p_param->phy = p_audio_config->phy;
    p_param->retransmission_number = p_audio_config->rtn;
    p_param->sdu_interval = p_audio_config->sdu_interval;
    p_param->cig_id = 1;
    p_param->cis_id = 1;
    p_param->presentation_delay = p_audio_config->presentation_delay;
    #ifdef MULTIPLEX_AUDIO_MODE
    p_param->max_sdu = (p_audio_config->channel_count & 2) ? p_audio_config->octets_per_codec_frame * 2
                                                           : p_audio_config->octets_per_codec_frame;
    #else
    p_param->max_sdu = ((p_stream_config->in_cfg.channel_count & 3) == 2) ? p_audio_config->octets_per_codec_frame * 2
                                                                          : p_audio_config->octets_per_codec_frame;
    #endif

    WICED_BT_TRACE("[%s] mtl %d framing %d phy %d rtn %d sdu %d pd %d max_sdu %d",
                   __FUNCTION__,
                   p_param->max_transport_latency,
                   p_param->framing,
                   p_param->phy,
                   p_param->retransmission_number,
                   p_param->sdu_interval,
                   p_param->presentation_delay,
                   p_param->max_sdu);
}

static void lepl_cap_select_qos_config_param(lepl_ase_data_t *p_lepl_ase)
{
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
    lepl_audio_config_t *p_audio_cfg = (p_ase->ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
                                           ? &g_lepl_gatt_cb.audio_state.current_strm_config.in_cfg
                                           : &g_lepl_gatt_cb.audio_state.current_strm_config.out_cfg;

    ga_lib_ascs_ase_codec_config_data_t *p_pref = &p_lepl_ase->ase.ase_cfg;
    p_audio_cfg->presentation_delay = MIN(p_pref->presentation_delay_in_us_max,
                                          MAX(p_pref->presentation_delay_in_us_min, p_audio_cfg->presentation_delay));
    if (p_pref->preferred_presentation_delay_in_us_max != 0)
    {
        p_audio_cfg->presentation_delay =
            MIN(p_pref->preferred_presentation_delay_in_us_max, p_audio_cfg->presentation_delay);
    }
    if (p_pref->preferred_presentation_delay_in_us_min != 0)
    {
        p_audio_cfg->presentation_delay =
            MAX(p_pref->preferred_presentation_delay_in_us_min, p_audio_cfg->presentation_delay);
    }

    p_audio_cfg->mtl = MAX(5, MIN(p_pref->max_transport_latency, p_audio_cfg->mtl));
    p_audio_cfg->rtn = MIN(10, MAX(p_pref->preferred_retransmission_number, p_audio_cfg->rtn));

    /* No preferred value set, choosing 2M Phy */
    p_audio_cfg->phy = (p_pref->preferred_phy) ? p_pref->preferred_phy : WICED_BLE_ISOC_LE_2M_PHY;
}

static wiced_result_t lepl_cap_utils_create_cig(lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] cis %d", __FUNCTION__, p_stream_config->num_cis);
    int index;
    wiced_ble_isoc_cis_config_t cis_config_list[2];
    wiced_ble_isoc_cig_param_t cig_param = {0};

    cig_param.cig_id = 1;
    cig_param.cis_count = p_stream_config->num_cis;
    cig_param.sdu_interval_c_to_p = (p_stream_config->in_cfg.sdu_interval) ? p_stream_config->in_cfg.sdu_interval
                                                                           : p_stream_config->out_cfg.sdu_interval;
    cig_param.sdu_interval_p_to_c = (p_stream_config->out_cfg.sdu_interval) ? p_stream_config->out_cfg.sdu_interval
                                                                            : p_stream_config->in_cfg.sdu_interval;
    cig_param.max_trans_latency_c_to_p =
        (p_stream_config->in_cfg.mtl) ? p_stream_config->in_cfg.mtl : p_stream_config->out_cfg.mtl;
    cig_param.max_trans_latency_p_to_c =
        (p_stream_config->out_cfg.mtl) ? p_stream_config->out_cfg.mtl : p_stream_config->in_cfg.mtl;
    cig_param.packing = WICED_BLE_ISOC_SEQUENTIAL_PACKING;
    cig_param.framing = p_stream_config->framing;

    WICED_BT_TRACE("CIG ID %d SDU Interval (M->S/S->M) (0x%x/0x%x) SCA %d Packing %d Framing %d ",
                   cig_param.cig_id,
                   cig_param.sdu_interval_c_to_p,
                   cig_param.sdu_interval_p_to_c,
                   cig_param.worst_case_sca,
                   cig_param.packing,
                   cig_param.framing);
    WICED_BT_TRACE("Trans Latency (M->S/S->M) (%d/%d) CIS count %d",
                   cig_param.max_trans_latency_c_to_p,
                   cig_param.max_trans_latency_p_to_c,
                   cig_param.cis_count);

    cig_param.p_cis_config_list = cis_config_list;
    for (index = 0; index < cig_param.cis_count; index++)
    {
        cis_config_list[index].cis_id = index + 1;
        WICED_BT_TRACE("[%s] cis_id %d", __FUNCTION__, cis_config_list[index].cis_id);
        #ifdef MULTIPLEX_AUDIO_MODE
        cis_config_list[index].max_sdu_c_to_p = (p_stream_config->in_cfg.channel_count & 2)
                                                    ? p_stream_config->in_cfg.octets_per_codec_frame * 2
                                                    : p_stream_config->in_cfg.octets_per_codec_frame;
        cis_config_list[index].max_sdu_p_to_c = (p_stream_config->out_cfg.channel_count & 2)
                                                    ? p_stream_config->out_cfg.octets_per_codec_frame * 2
                                                    : p_stream_config->out_cfg.octets_per_codec_frame;
        #else
        cis_config_list[index].max_sdu_c_to_p = ((p_stream_config->in_cfg.channel_count & 3) == 2)
                                                    ? p_stream_config->in_cfg.octets_per_codec_frame * 2
                                                    : p_stream_config->in_cfg.octets_per_codec_frame;
        cis_config_list[index].max_sdu_p_to_c = ((p_stream_config->out_cfg.channel_count & 3) == 2)
                                                    ? p_stream_config->out_cfg.octets_per_codec_frame * 2
                                                    : p_stream_config->out_cfg.octets_per_codec_frame;
        #endif

        cis_config_list[index].phy_c_to_p =
            (p_stream_config->in_cfg.phy) ? p_stream_config->in_cfg.phy : p_stream_config->out_cfg.phy;
        cis_config_list[index].phy_p_to_c =
            (p_stream_config->out_cfg.phy) ? p_stream_config->out_cfg.phy : p_stream_config->in_cfg.phy;
        cis_config_list[index].rtn_c_to_p =
            (p_stream_config->in_cfg.rtn) ? p_stream_config->in_cfg.rtn : p_stream_config->out_cfg.rtn;
        cis_config_list[index].rtn_p_to_c =
            (p_stream_config->out_cfg.rtn) ? p_stream_config->out_cfg.rtn : p_stream_config->in_cfg.rtn;
        WICED_BT_TRACE("CIS ID %d SDU (M->S/S->M) (%d/%d) PHY (M->S/S->M) (%d/%d) RTN (M->S/S->M) (%d/%d)",
                       cis_config_list[index].cis_id,
                       cis_config_list[index].max_sdu_c_to_p,
                       cis_config_list[index].max_sdu_p_to_c,
                       cis_config_list[index].phy_c_to_p,
                       cis_config_list[index].phy_p_to_c,
                       cis_config_list[index].rtn_c_to_p,
                       cis_config_list[index].rtn_p_to_c);
    }
    return wiced_ble_isoc_central_set_cig_param(&cig_param);
}

static void lepl_cap_create_cis(lepl_ase_data_t *p_lepl_ase)
{
    wiced_result_t res = WICED_SUCCESS;
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
    ga_lib_ascs_config_qos_args_t *p_qos = &p_ase->qos_configured;
    wiced_ble_isoc_cis_acl_t conn_hdl;

    conn_hdl.cis_conn_handle = wiced_ble_isoc_central_get_cis_conn_handle(p_qos->cig_id, p_qos->cis_id);
    conn_hdl.acl_conn_handle = p_lepl_ase->acl_conn_handle;

    //Assign cis handle to p_ase if it's already connected
    if (wiced_ble_isoc_is_cis_connected_with_conn_hdl(conn_hdl.cis_conn_handle))
    {
        p_lepl_ase->cis_conn_handle = conn_hdl.cis_conn_handle;
    }
    else
    {
        res = wiced_ble_isoc_central_create_cis(1, &conn_hdl);
    }

    WICED_BT_TRACE("[%s] create cis acl 0x%d cis 0x%x ase_cis 0x%x res %d",
                   __FUNCTION__,
                   conn_hdl.acl_conn_handle,
                   conn_hdl.cis_conn_handle,
                   p_lepl_ase->cis_conn_handle,
                   res);
}

// Cleanup ASE entries for a given ACL connection handle
void lepl_cap_cleanup_ases_for_acl_handle(uint16_t acl_handle)
{
    lepl_ase_data_t **p_ase_list = g_lepl_gatt_cb.audio_state.p_lepl_ase_list;
    for (int i = 0; i < MAX_CIS_CONN * 2; i++)
    {
        if (p_ase_list[i] && (p_ase_list[i]->acl_conn_handle == acl_handle))
        {
            if (p_ase_list[i]->data_path_established)
            {
                lepl_isoc_dhm_remove_cis_datapath(p_ase_list[i]);
            }
            p_ase_list[i] = NULL;
        }
    }
}

static void assign_ase_to_be_configured(lepl_ase_data_t *p_lepl_ase)
{
    lepl_ase_data_t **p_ase_list = g_lepl_gatt_cb.audio_state.p_lepl_ase_list;
    for (int i = 0; i < MAX_CIS_CONN * 2; i++)
    {
        if (p_ase_list[i] == NULL)
        {
            p_ase_list[i] = p_lepl_ase;
            break;
        }
    }
}

// Configure and enable ASE
// Returns WICED_SUCCESS if ASE configured and enabled successfully
// note that p_ase_count is incremented only on success and p_offset points to next ASE to be configured
static wiced_result_t configure_and_enable_ase(lepl_clcb_t* p_clcb,
    uint8_t ase_type,
    lepl_unicast_stream_config_t* p_stream_config,
    uint8_t* p_offset,
    uint8_t* p_ase_count)
{
    lepl_ase_data_t *p_lepl_ase = lepl_get_remote_ase(p_clcb, ase_type, p_offset);
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
    if (!p_lepl_ase)
    {
        WICED_BT_TRACE_CRIT("[%s] conn id %x p_ase is NULL", __FUNCTION__, p_clcb->conn_id);
        return WICED_ERROR;
    }

    ga_lib_ascs_config_codec_args_t codec_param = {0};
    lepl_get_codec_config_param(ase_type, p_stream_config, &codec_param);
    p_ase->qos_configured.cis_id = (lepl_csis_check_is_csis_supported(p_clcb)) ? p_clcb->csis.rank:1;

    codec_param.csc.audio_channel_allocation = (p_clcb->pacs.snk_audio_location & BAP_AUDIO_LOCATION_FRONT_LEFT)
                                                   ? BAP_AUDIO_LOCATION_FRONT_LEFT
                                                   : BAP_AUDIO_LOCATION_FRONT_RIGHT;

    // Set channel allocation
    if (ase_type == GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE)
    {
        ga_lib_bap_context_type_t ctx = p_stream_config->context_type;
#ifdef MULTIPLEX_AUDIO_MODE

        if (p_stream_config->in_cfg.channel_count & 2)
        {
            codec_param.csc.audio_channel_allocation = p_clcb->pacs.snk_audio_location &
                                                       (BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT);
        }
        else if ((!p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle || (p_clcb->csis.size == 1)))
        {
            if (ctx == BAP_CONTEXT_TYPE_MEDIA)
            {
                codec_param.csc.audio_channel_allocation =
                    (p_clcb->pacs.snk_audio_location & (BAP_AUDIO_LOCATION_FRONT_LEFT << (*p_ase_count)));
                p_ase->qos_configured.cis_id = (*p_ase_count) + 1;
            }
            else if (ctx == BAP_CONTEXT_TYPE_RINGTONE)
            {
                // Source ASE already has beeen configured ase count incremented to 1
                codec_param.csc.audio_channel_allocation =
                    (p_clcb->pacs.snk_audio_location & (BAP_AUDIO_LOCATION_FRONT_LEFT << ((*p_ase_count) - 1)));
                p_ase->qos_configured.cis_id = (*p_ase_count);
            }
        }
        #else
        if (((p_stream_config->in_cfg.channel_count & 3) != 2) &&
            ((!p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle) || (p_clcb->csis.size == 1)))
        {
            // For media and ringtone context, player will send multiple stream to different channels to play stereo audio
            // For other context, we will follow sink audio location of peer device to assign channel for mono stream.

            if (ctx == BAP_CONTEXT_TYPE_MEDIA)
            {
                codec_param.csc.audio_channel_allocation =
                    (p_clcb->pacs.snk_audio_location & (BAP_AUDIO_LOCATION_FRONT_LEFT << (*p_ase_count)));
                p_ase->qos_configured.cis_id = (*p_ase_count) + 1;
            }
            else if (ctx == BAP_CONTEXT_TYPE_RINGTONE)
            {
                // Source ASE already has beeen configured ase count incremented to 1
                codec_param.csc.audio_channel_allocation =
                    (p_clcb->pacs.snk_audio_location & (BAP_AUDIO_LOCATION_FRONT_LEFT << ((*p_ase_count) - 1)));
                p_ase->qos_configured.cis_id = (*p_ase_count);
            }
        }
        else
        {
            codec_param.csc.audio_channel_allocation = p_clcb->pacs.snk_audio_location &
                                                       (BAP_AUDIO_LOCATION_FRONT_LEFT | BAP_AUDIO_LOCATION_FRONT_RIGHT);
        }
        #endif
    }

    (*p_ase_count)++;
    lepl_ascs_write_control_point_config_codec(p_clcb, &p_lepl_ase->ase, &codec_param);
    assign_ase_to_be_configured(p_lepl_ase);
    return WICED_SUCCESS;
}

wiced_result_t lepl_cap_start_media_streaming(lepl_cordinated_set_t *p_set,
                                              lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s] set 0x%x strm_cfg 0x%x num dev %d", __FUNCTION__, p_set, p_stream_config, p_set->num_devices);
    uint8_t ase_count = 0;
    wiced_result_t result = WICED_SUCCESS;

    for (int i = 0; i < p_set->set_size; i++)
    {
        uint8_t offset = 0;
        uint16_t conn_id = p_set->member_list[i].conn_id;
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
        if (!p_clcb)
        {
            WICED_BT_TRACE_CRIT("[%s] conn id %x NO p_clcb!", __FUNCTION__, conn_id);
            continue;
        }

        uint8_t set_size = (p_clcb->peer_profiles.service_handles[LEPL_CSIS].start_handle) ? p_clcb->csis.size : 1;
        if (set_size < 2)
        {
            #ifdef MULTIPLEX_AUDIO_MODE
            uint8_t max_ase = (p_stream_config->in_cfg.channel_count & 2) ? 1 : 2;
            #else
            uint8_t max_ase = ((p_stream_config->in_cfg.channel_count & 3) == 2) ? 1 : 2;
            #endif
            // Stereo or multiple ASEs per device
            for (int index = 0; index < max_ase; index++)
            {
                if (configure_and_enable_ase(p_clcb,
                                             GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE,
                                             p_stream_config,
                                             &offset,
                                             &ase_count) != WICED_SUCCESS)
                {
                    result = WICED_ERROR;
                    break;
                }
            }
            p_stream_config->num_cis = ase_count;
        }
        else
        {
            // Mono or single ASE per device
            if (configure_and_enable_ase(p_clcb,
                                         GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE,
                                         p_stream_config, &offset,
                                         &ase_count) != WICED_SUCCESS)
            {
                result = WICED_ERROR;
                break;
            }
            p_stream_config->num_cis = set_size;
        }
    }
#ifdef HS_SPK_ENABLED
    lepl_cap_update_cig_sync(LE_CIG_SYNC_DELAY);
#endif

    return result;
}

wiced_result_t lepl_cap_start_conv_streaming(lepl_cordinated_set_t *p_set,
                                             lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s]", __FUNCTION__);
    uint8_t ase_count = 0;
    wiced_result_t result = WICED_SUCCESS;

    for (int i = 0; i < p_set->set_size; i++)
    {
        uint8_t offset = 0;
        uint16_t conn_id = p_set->member_list[i].conn_id;
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
        if (!p_clcb)
        {
            WICED_BT_TRACE_CRIT("[%s] conn id %x No p_clcb", __FUNCTION__, conn_id);
            continue;
        }

        // Source ASE (if left channel supported)
        if (p_clcb->pacs.snk_audio_location & BAP_AUDIO_LOCATION_FRONT_LEFT)
        {
            if (configure_and_enable_ase(p_clcb,
                                         GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE,
                                         p_stream_config,
                                         &offset,
                                         &ase_count) != WICED_SUCCESS)
            {
                result = WICED_ERROR;
                break;
            }
        }

        // Sink ASE
        offset = 0;
        if (configure_and_enable_ase(p_clcb, GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE, p_stream_config, &offset, &ase_count) !=
            WICED_SUCCESS)
        {
            result = WICED_ERROR;
            break;
        }
    }
    p_stream_config->num_cis = p_set->num_devices;
    return result;
}

wiced_result_t lepl_cap_start_mic_streaming(lepl_cordinated_set_t *p_set, lepl_unicast_stream_config_t *p_stream_config)
{
    WICED_BT_TRACE("[%s]", __FUNCTION__);

    uint8_t ase_count = 0;
    wiced_result_t result = WICED_SUCCESS;
    for (int i = 0; i < p_set->set_size; i++)
    {
        uint8_t offset = 0;
        uint16_t conn_id = p_set->member_list[i].conn_id;
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);

        if (!p_clcb)
        {
            WICED_BT_TRACE_CRIT("[%s] conn id %x No p_clcb", __FUNCTION__, conn_id);
            continue;
        }

        if (configure_and_enable_ase(p_clcb, GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE, p_stream_config, &offset, &ase_count) !=
            WICED_SUCCESS)
        {
            result = WICED_ERROR;
            break;
        }
    }
    p_stream_config->num_cis = p_set->num_devices;
    return result;
}

wiced_result_t lepl_cap_start_streaming(uint16_t conn_id, lepl_unicast_stream_config_t *p_stream_config)
{
    wiced_result_t res = WICED_SUCCESS;
    if (!p_stream_config)
    {
        WICED_BT_TRACE_CRIT("[%s] No audio config ", __FUNCTION__);
        return WICED_BADARG;
    }

    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    WICED_BT_TRACE("lepl_start_streaming %x ctx 0x%x\n", conn_id, p_stream_config->context_type);
    switch (p_stream_config->context_type)
    {
    case BAP_CONTEXT_TYPE_MEDIA:
        lepl_cap_start_media_streaming(p_set, p_stream_config);
        break;
    case BAP_CONTEXT_TYPE_RINGTONE:
    case BAP_CONTEXT_TYPE_CONVERSATIONAL:
    case BAP_CONTEXT_TYPE_GAME:
        lepl_cap_start_conv_streaming(p_set, p_stream_config);
        break;
    case BAP_CONTEXT_TYPE_VOICE_ASSISTANTS:
        lepl_cap_start_mic_streaming(p_set, p_stream_config);
        break;
    default:
        break;
    }
    return res;
}

static void release_configured_ases(lepl_clcb_t *p_clcb)
{
    if (p_clcb == NULL)
    {
        WICED_BT_TRACE_CRIT("[%s] p_clcb is NULL", __FUNCTION__);
        return;
    }

    for (int i = 0; i < MAX_CIS_CONN * 2; i++)
    {
        lepl_ase_data_t *p_lepl_ase = g_lepl_gatt_cb.audio_state.p_lepl_ase_list[i];
        if(p_lepl_ase && (p_lepl_ase->acl_conn_handle == p_clcb->acl_handle))
        {
            lepl_ascs_write_control_point_release(p_clcb, p_lepl_ase);
            g_lepl_gatt_cb.audio_state.is_releasing = 1;
        }
    }
}

void lepl_cap_stop_media_streaming(lepl_cordinated_set_t *p_set)
{
    for (int i = 0; i < p_set->set_size; i++)
    {
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(p_set->member_list[i].conn_id);
        release_configured_ases(p_clcb);
    }
}

void lepl_cap_stop_conv_streaming(lepl_cordinated_set_t *p_set)
{
    for (int i = 0; i < p_set->set_size; i++)
    {
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(p_set->member_list[i].conn_id);
        release_configured_ases(p_clcb);
    }
}

void lepl_cap_stop_mic_streaming(lepl_cordinated_set_t *p_set)
{
    for (int i = 0; i < p_set->set_size; i++)
    {
        lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(p_set->member_list[i].conn_id);
         release_configured_ases(p_clcb);
    }
    lepl_rpc_send_mic_state(0);
}

wiced_result_t lepl_cap_stop_streaming(uint16_t conn_id)
{
    wiced_result_t res = WICED_SUCCESS;
    WICED_BT_TRACE("[%s]", __FUNCTION__);
    lepl_cordinated_set_t temp_set = {0};
    lepl_cordinated_set_t *p_set = get_cordinated_set(conn_id, &temp_set);
    switch (g_lepl_gatt_cb.audio_state.current_strm_config.context_type)
    {
    case BAP_CONTEXT_TYPE_MEDIA:
        lepl_cap_stop_media_streaming(p_set);
        break;
    case BAP_CONTEXT_TYPE_RINGTONE:
    case BAP_CONTEXT_TYPE_CONVERSATIONAL:
    case BAP_CONTEXT_TYPE_GAME:
        lepl_cap_stop_conv_streaming(p_set);
        break;
    case BAP_CONTEXT_TYPE_VOICE_ASSISTANTS:
        lepl_cap_stop_mic_streaming(p_set);
        break;
    default:
        break;
    }
    return res;
}

wiced_bool_t cap_if_ase_state_changed(ga_lib_ascs_state_t state)
{
    WICED_BT_TRACE("[%s] state %d", __FUNCTION__, state);
    for (int i = 0; i < MAX_CIS_CONN * 2; i++)
    {
        lepl_ase_data_t *p_lepl_ase = g_lepl_gatt_cb.audio_state.p_lepl_ase_list[i];
        if (p_lepl_ase == NULL)
        {
            continue;
        }

        ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
        if (p_ase->ase_state != state)
        {
            return WICED_FALSE;
        }
    }
    return WICED_TRUE;
}

void lepl_cap_ascs_state_update(uint16_t conn_id, lepl_clcb_t *p_clcb, lepl_ase_data_t *p_lepl_ase)
{
    ga_lib_ascs_ase_t *p_ase = &p_lepl_ase->ase;
    ga_lib_ascs_state_t state = p_ase->ase_state;
    lepl_audio_state_info_t *p_audio_state = &g_lepl_gatt_cb.audio_state;
    WICED_BT_TRACE("[%s] conn_id %d ase_id %d state %d", __FUNCTION__, conn_id, p_ase->ase_id, p_ase->ase_state);

    if (!p_audio_state->is_releasing)
    {
        switch (state)
        {
        case GA_LIB_ASCS_STATE_CODEC_CONFIGURED:
        {
            ga_lib_ascs_config_qos_args_t qos_param = {0};
            lepl_cap_select_qos_config_param(p_lepl_ase);
            lepl_get_qos_config_param(p_ase->ase_type, &p_audio_state->current_strm_config, &qos_param);
            qos_param.cis_id = p_ase->qos_configured.cis_id;
            lepl_ascs_write_control_point_config_qos(p_clcb, p_lepl_ase, &qos_param);
        }
        break;
        case GA_LIB_ASCS_STATE_QOS_CONFIGURED:
        {

            p_lepl_ase->ase.metadata.streaming_audio_ctx = p_audio_state->current_strm_config.context_type;
            lepl_ascs_write_control_point_enable(p_clcb, p_lepl_ase);
        }
        break;
        case GA_LIB_ASCS_STATE_ENABLING:
            if (p_audio_state->current_mode != LEPL_AUDIO_MODE_IN_TRANSIT)
            {
                lepl_cap_create_cis(p_lepl_ase);
            }
            break;
        default:
            break;
        }
    }

    if (cap_if_ase_state_changed(state))
    {
        lepl_cap_update_audio_mode(conn_id, state);
    }
}

void lepl_cap_handle_set_member_reconnection(uint16_t conn_id)
{
    lepl_audio_mode_t current_mode = lepl_cap_get_audio_mode();

    switch (current_mode)
    {
    case LEPL_AUDIO_MODE_MEDIA:
        le_audio_rpc_send_mcs_state_update(conn_id, g_lepl_gatt_cb.local_service_data.gmcs.media_state);
        break;
    case LEPL_AUDIO_MODE_MIC:
        le_audio_rpc_send_mic_state_update(conn_id, 1);
        break;
    case LEPL_AUDIO_MODE_CALL:
        break;
    case LEPL_AUDIO_MODE_BIDIRMIC:
        le_audio_rpc_send_convo_stream_state_update(conn_id, 1);
        break;
    default:
        return;
    }

    lepl_clcb_t *p_clcb = lepl_gatt_get_clcb_by_conn_id(conn_id);
    lepl_unicast_stream_config_t *p_stream_config = &g_lepl_gatt_cb.audio_state.current_strm_config;
    uint8_t offset = 0;
    uint8_t ase_count = 0;
    if (current_mode == LEPL_AUDIO_MODE_MEDIA || current_mode == LEPL_AUDIO_MODE_BIDIRMIC ||
        current_mode == LEPL_AUDIO_MODE_CALL)
    {
        configure_and_enable_ase(p_clcb, GA_LIB_ASCS_CHARACTERISTIC_SNK_ASE, p_stream_config, &offset, &ase_count);
    }
    else if (current_mode == LEPL_AUDIO_MODE_MIC || current_mode == LEPL_AUDIO_MODE_BIDIRMIC ||
             current_mode == LEPL_AUDIO_MODE_CALL)
    {
        if (current_mode == LEPL_AUDIO_MODE_CALL &&
            !(p_clcb->pacs.src_audio_location & BAP_AUDIO_LOCATION_FRONT_LEFT))
        {
            return;
        }
        offset = 0;
        configure_and_enable_ase(p_clcb, GA_LIB_ASCS_CHARACTERISTIC_SRC_ASE, p_stream_config, &offset, &ase_count);
    }
}

void lepl_cap_handle_ascs_error_response(uint16_t conn_id)
{
    lepl_audio_mode_t mode = lepl_cap_get_final_audio_mode();
    switch (mode)
    {
    case LEPL_AUDIO_MODE_MEDIA:
        lepl_mcs_pause(conn_id);
        break;
    case LEPL_AUDIO_MODE_CALL:
        lepl_cap_stop_streaming(conn_id);
        break;
    case LEPL_AUDIO_MODE_BIDIRMIC:
        lepl_start_stop_bidir_mic(conn_id, FALSE, NULL);
        break;
    case LEPL_AUDIO_MODE_MIC:
        lepl_cap_stop_streaming(conn_id);
        break;
    default:
        break;
    }
    lepl_cap_reset_audio_state();
}

//Application State
void lepl_cap_set_next_audio_mode(lepl_audio_mode_t mode, lepl_unicast_stream_config_t *p_stream_config)
{
    lepl_audio_state_info_t *p_audio_state = &g_lepl_gatt_cb.audio_state;
    lepl_audio_mode_t current_mode = p_audio_state->current_mode;

    if (mode != LEPL_AUDIO_MODE_NONE)
    {
        if (current_mode != LEPL_AUDIO_MODE_NONE && current_mode != LEPL_AUDIO_MODE_IN_TRANSIT)
        {
            if (p_audio_state->paused_mode == LEPL_AUDIO_MODE_MEDIA && mode != LEPL_AUDIO_MODE_MEDIA)
            {
                lepl_media_control_service_update_state(GA_LIB_MCS_MEDIA_INACTIVE);
            }
            p_audio_state->paused_mode = current_mode;
            p_audio_state->paused_strm_config = p_audio_state->current_strm_config;
        }
    }
    else if (current_mode == LEPL_AUDIO_MODE_MEDIA)
    {
        p_audio_state->paused_strm_config = p_audio_state->current_strm_config;
    }

    if (p_stream_config)
        WICED_MEMCPY(&p_audio_state->current_strm_config, p_stream_config, sizeof(lepl_unicast_stream_config_t));
    else
        WICED_MEMSET(&p_audio_state->current_strm_config, 0, sizeof(lepl_unicast_stream_config_t));
    p_audio_state->transit_info.initial_mode = current_mode;
    p_audio_state->transit_info.final_mode = mode;
    p_audio_state->current_mode = LEPL_AUDIO_MODE_IN_TRANSIT;
    WICED_BT_TRACE("[%s] paused %d current %d initial %d final %d",
                   __FUNCTION__,
                   p_audio_state->paused_mode,
                   p_audio_state->current_mode,
                   p_audio_state->transit_info.initial_mode,
                   p_audio_state->transit_info.final_mode);
}

lepl_audio_mode_t lepl_cap_get_audio_mode(void)
{
    WICED_BT_TRACE("[%s] current mode %d", __FUNCTION__, g_lepl_gatt_cb.audio_state.current_mode);
    return g_lepl_gatt_cb.audio_state.current_mode;
}

lepl_audio_mode_t lepl_cap_get_final_audio_mode(void)
{
    WICED_BT_TRACE("[%s] final mode %d", __FUNCTION__, g_lepl_gatt_cb.audio_state.transit_info.final_mode);
    return g_lepl_gatt_cb.audio_state.transit_info.final_mode;
}

void lepl_cap_reset_audio_state(void)
{
    WICED_BT_TRACE("[%s]", __FUNCTION__);
    WICED_MEMSET(&g_lepl_gatt_cb.audio_state, 0, sizeof(lepl_audio_state_info_t));
}

void lepl_cap_handle_set_disconnection(void)
{
    WICED_BT_TRACE("[%s]", __FUNCTION__);
    g_lepl_gatt_cb.local_service_data.gmcs.media_state = GA_LIB_MCS_MEDIA_PAUSED;
    lepl_cap_reset_audio_state();
}

void lepl_cap_resume_streaming(uint16_t conn_id, lepl_audio_mode_t mode, lepl_unicast_stream_config_t *p_stream_config)
{
    switch (mode)
    {
    case LEPL_AUDIO_MODE_MEDIA:
        lepl_mcs_play(conn_id, p_stream_config);
        break;
    case LEPL_AUDIO_MODE_CALL:
        lepl_rpc_ccs_set_incoming_remote_call(conn_id, p_stream_config);
        break;
    case LEPL_AUDIO_MODE_MIC:
        lepl_start_voice_capture(conn_id, p_stream_config);
        break;
    case LEPL_AUDIO_MODE_BIDIRMIC:
        lepl_start_stop_bidir_mic(conn_id, TRUE, p_stream_config);
        break;
    default:
        WICED_BT_TRACE_CRIT("[%s] Not a valid mode %d", __FUNCTION__, mode);
        break;
    }
}


void lepl_cap_update_audio_mode(uint16_t conn_id, ga_lib_ascs_state_t state)
{
    WICED_BT_TRACE("[%s] conn_id %d state %d", __FUNCTION__, conn_id, state);
    lepl_audio_state_info_t *p_audio_state = &g_lepl_gatt_cb.audio_state;
    if (p_audio_state->current_mode != LEPL_AUDIO_MODE_IN_TRANSIT)
        return;

    switch (state)
    {
    case GA_LIB_ASCS_STATE_IDLE:
    case GA_LIB_ASCS_STATE_CODEC_CONFIGURED:
    {
        if (!p_audio_state->is_releasing)
        {
            WICED_BT_TRACE("[%s] is releasing %d", __FUNCTION__, p_audio_state->is_releasing);
            return;
        }
        WICED_MEMSET(p_audio_state->p_lepl_ase_list, 0, sizeof(p_audio_state->p_lepl_ase_list));
        p_audio_state->is_releasing = 0;
        if (!p_audio_state->paused_mode)
            break;

        if (p_audio_state->transit_info.final_mode)
        {
            WICED_BT_TRACE("[%s] final mode %d", __FUNCTION__, p_audio_state->transit_info.final_mode);
            lepl_cap_resume_streaming(conn_id, p_audio_state->transit_info.final_mode, &p_audio_state->current_strm_config);
        }
        else
        {
            WICED_BT_TRACE("[%s] paused mode %d", __FUNCTION__, p_audio_state->paused_mode);
            lepl_cap_resume_streaming(conn_id, p_audio_state->paused_mode, &p_audio_state->paused_strm_config);
            lepl_cap_set_next_audio_mode(p_audio_state->paused_mode, &p_audio_state->paused_strm_config);
            p_audio_state->paused_mode = LEPL_AUDIO_MODE_NONE;
        }
        return;
    }
    break;
    case GA_LIB_ASCS_STATE_ENABLING:
        lepl_cap_utils_create_cig(&p_audio_state->current_strm_config);
        break;
    case GA_LIB_ASCS_STATE_STREAMING:
        WICED_MEMSET(p_audio_state->p_lepl_ase_list, 0, sizeof(p_audio_state->p_lepl_ase_list));
        if (lepl_get_call_control_server_state() == CALL_CONTROL_SERVER_STATE_INBAND_RINGTONE)
        {
            WICED_BT_TRACE("[%s] Ringtone streaming %d", __FUNCTION__);
            return;
        }
        break;
    default:
        return;
    }
    p_audio_state->current_mode = p_audio_state->transit_info.final_mode;
}


#ifdef HS_SPK_ENABLED
void lepl_cap_update_cig_sync(uint16_t delay_limit)
{
    uint8_t param_buf[3] ={0x09,0,0};
    param_buf[1] = (delay_limit)&0xFF;
    param_buf[2] = (delay_limit>>8)&0xFF;
    // Send VSC to update CIG_Delay_Sync
    wiced_bt_dev_vendor_specific_command(0xFDDF,sizeof(param_buf),param_buf,NULL);
}
#endif

#if defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
le_pl_transcoding_data_t le_pl_transcoding_data = {0};
extern void lepl_rpc_broadcast_src_handle_start_streaming(uint8_t *p_data, uint32_t data_len);
extern void lepl_rpc_handle_pause(uint8_t *p_data, uint32_t data_len);
extern void lepl_rpc_handle_play(uint8_t *p_data, uint32_t data_len);
void lepl_transcoding_save_broadcast_config(uint8_t *p_data, uint32_t data_len)
{
    // Store for start only
    if (!p_data)
        return;
    if (*p_data)
    {
        if(data_len > LE_PL_BR_CONFIG_DATA_SIZE)
            data_len = LE_PL_BR_CONFIG_DATA_SIZE;
        le_pl_transcoding_data.broadcast_config_len = data_len;
        WICED_MEMCPY(le_pl_transcoding_data.broadcast_config, p_data, le_pl_transcoding_data.broadcast_config_len);
        le_pl_transcoding_data.is_unicast = 0;
        le_pl_transcoding_data.in_use = 1;
    }
}
void lepl_transcoding_start_streaming(wiced_bool_t start)
{
    if (!le_pl_transcoding_data.in_use)
        return;
    if (start)
    {
        if (le_pl_transcoding_data.is_unicast)
        {
            lepl_audio_config_t *audio_config = &g_lepl_gatt_cb.app_state.current_strm_config;
            if ((g_lepl_gatt_cb.app_state.current_state == LEPL_APP_STATE_IDLE) && lepl_mcs_play(le_pl_transcoding_data.conn_id, audio_config) == WICED_SUCCESS)
            {
                lepl_cap_set_next_application_state(LEPL_APP_STATE_MEDIA, audio_config);
            }
        }
        else
        {
            lepl_rpc_broadcast_src_handle_start_streaming(le_pl_transcoding_data.broadcast_config, le_pl_transcoding_data.broadcast_config_len);
        }
    }
    else
    {
        if (le_pl_transcoding_data.is_unicast && (g_lepl_gatt_cb.app_state.current_state != LEPL_APP_STATE_IDLE))
        {
            lepl_rpc_handle_pause((uint8_t *)&le_pl_transcoding_data.conn_id, sizeof(le_pl_transcoding_data.conn_id));
        }
        else
        {
            uint8_t start=0;
            lepl_rpc_broadcast_src_handle_start_streaming(&start, sizeof(uint8_t));
        }
    }
}
#endif // defined(AUDIO_TRANSCODING) && defined(AUDIO_TRANSCODING_AUTO_PLAY_PAUSE)
