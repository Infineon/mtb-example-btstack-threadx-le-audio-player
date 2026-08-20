/*
 * $ Copyright Cypress Semiconductor $
 */

#include "lepl.h"

void lepl_gmap_handle_received_characteristics_value(uint16_t conn_id,
                                                     lepl_clcb_t *p_clcb,
                                                     wiced_bt_gatt_optype_t op,
                                                     wiced_bt_gatt_status_t status,
                                                     wiced_bt_gatt_data_t *p_gatt_data)
{
    int gmap_index = gatt_intf_find_characteristic_type_by_value_handle(p_clcb->peer_profiles.gmap,
                                                                        GA_LIB_GMAP_CHARACTERISTIC_MAX,
                                                                        p_gatt_data->handle);
    lepl_gmap_t *p_gmap = &p_clcb->gmap;
    uint8_t *p_data = p_gatt_data->p_data;
    uint8_t u8_value;

    STREAM_TO_UINT8(u8_value, p_data);

    //check for GMAP handles
    switch (gmap_index)
    {
    case GA_LIB_GMAP_CHARACTERISTIC_ROLE: // 0
        p_gmap->gmap_role = u8_value;
        break;
    case GA_LIB_GMAP_CHARACTERISTIC_UGG_FEATURES: //1
        p_gmap->ugg_features = u8_value;
        break;
    case GA_LIB_GMAP_CHARACTERISTIC_UGT_FEATURES: //2
        p_gmap->ugt_features = u8_value;
        break;
    case GA_LIB_GMAP_CHARACTERISTIC_BGS_FEATURES: //3
        p_gmap->bgs_feature = u8_value;
        break;
    case GA_LIB_GMAP_CHARACTERISTIC_BGR_FEATURES: //4
        p_gmap->bgr_feature = u8_value;
        break;
    default:
        break;
    }

    return;
}
