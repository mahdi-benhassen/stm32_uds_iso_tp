#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "canopen_reference_od.h"
#include "inventus_battery_data.h"

static const uint16_t requested_indices[] = {
    0x4800, 0x4801, 0x4802, 0x4803, 0x4804, 0x4805, 0x4806, 0x4807,
    0x4808, 0x4809, 0x480A, 0x480B, 0x480C, 0x480D, 0x480E, 0x480F,
    0x4810, 0x4811, 0x4812, 0x4813, 0x4819, 0x4850, 0x4851, 0x4852,
    0x4853, 0x4854, 0x4855, 0x4856, 0x4857, 0x4858, 0x4859, 0x485A,
    0x485B, 0x485C, 0x485D, 0x485E, 0x485F, 0x4860, 0x4861, 0x4862,
    0x4863, 0x4864, 0x4865, 0x4866, 0x4867, 0x4868, 0x4869, 0x486A,
    0x486B, 0x486C, 0x4880, 0x4881, 0x4882, 0x4883, 0x4900, 0x4901,
    0x4903, 0x4904, 0x4920, 0x4921
};

static OD_IO_t get_io(uint16_t index, uint8_t sub_index) {
    OD_IO_t io;
    OD_entry_t *entry = OD_find(OD, index);
    assert(entry != NULL);
    assert(OD_getSub(entry, sub_index, &io, false) == ODR_OK);
    return io;
}

int main(void) {
    assert(sizeof(requested_indices) / sizeof(requested_indices[0]) == 60U);
    const uint16_t issue12_indices[] = {
        0x6000U, 0x6001U, 0x6010U, 0x6020U, 0x6030U,
        0x6050U, 0x6051U, 0x6052U, 0x6060U, 0x6070U, 0x6081U
    };
    assert(sizeof(issue12_indices) / sizeof(issue12_indices[0]) == 11U);
    assert(OD != NULL);
    OD_size_t count_read = 0U;

    for (size_t position = 0U;
         position < sizeof(requested_indices) / sizeof(requested_indices[0]);
         ++position) {
        OD_entry_t *entry = OD_find(OD, requested_indices[position]);
        assert(entry != NULL);
    }

    for (size_t position = 0U;
         position < sizeof(issue12_indices) / sizeof(issue12_indices[0]);
         ++position) {
        assert(OD_find(OD, issue12_indices[position]) != NULL);
    }

    InventusBatteryDataState data_state = {0};
    InventusBatteryData_Init(&data_state);
    assert(data_state.initialized);
    assert(InventusBatteryData_UpdateMeasurements(&data_state, 512U, -16, 75U, 320U));
    assert(!InventusBatteryData_UpdateMeasurements(&data_state, 512U, -16, 101U, 320U));
    uint32_t battery_voltage = 0U;
    assert(InventusBatteryData_Read(&data_state, 0x6060U, 0U, &battery_voltage));
    assert(battery_voltage == 512U);
    uint32_t battery_soc = 0U;
    assert(InventusBatteryData_Read(&data_state, 0x6081U, 0U, &battery_soc));
    assert(battery_soc == 75U);

    OD_IO_t battery_status = get_io(0x6000U, 0U);
    assert(battery_status.stream.dataLength == 1U);
    assert((battery_status.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t temperature = get_io(0x6010U, 0U);
    assert(temperature.stream.dataLength == 2U);
    assert((temperature.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t charge_current = get_io(0x6070U, 0U);
    assert(charge_current.stream.dataLength == 2U);
    assert((charge_current.stream.attribute & ODA_SDO_W) == 0U);
    uint16_t charge_current_default = 0U;
    count_read = 0U;
    assert(charge_current.read != NULL);
    assert(charge_current.read(&charge_current.stream, &charge_current_default,
                               sizeof(charge_current_default), &count_read) == ODR_OK);
    assert(charge_current_default == 320U);

    OD_IO_t battery_parameters_count = get_io(0x6020U, 0U);
    assert(battery_parameters_count.stream.dataLength == 1U);
    assert((battery_parameters_count.stream.attribute & ODA_SDO_W) == 0U);
    uint8_t battery_parameters_highest = 0U;
    count_read = 0U;
    assert(battery_parameters_count.read(&battery_parameters_count.stream,
                                         &battery_parameters_highest,
                                         sizeof(battery_parameters_highest), &count_read) == ODR_OK);
    assert(battery_parameters_highest == 4U);
    OD_IO_t battery_type = get_io(0x6020U, 1U);
    assert(battery_type.stream.dataLength == 1U);
    OD_IO_t battery_capacity = get_io(0x6020U, 2U);
    assert(battery_capacity.stream.dataLength == 2U);
    OD_IO_t battery_cells = get_io(0x6020U, 4U);
    assert(battery_cells.stream.dataLength == 2U);
    OD_IO_t serial_count = get_io(0x6030U, 0U);
    assert(serial_count.stream.dataLength == 1U);
    OD_IO_t serial_first_half = get_io(0x6030U, 1U);
    assert(serial_first_half.stream.dataLength == 4U);
    OD_IO_t serial_second_half = get_io(0x6030U, 2U);
    assert(serial_second_half.stream.dataLength == 4U);

    const uint16_t issue12_gaps[] = {0x6002U, 0x6021U, 0x6031U, 0x6053U, 0x6071U, 0x6080U};
    for (size_t position = 0U;
         position < sizeof(issue12_gaps) / sizeof(issue12_gaps[0]);
         ++position) {
        assert(OD_find(OD, issue12_gaps[position]) == NULL);
    }

    const uint16_t identity_indices[] = {0x1008U, 0x1009U, 0x100AU};
    const uint8_t identity_lengths[] = {32U, 16U, 16U};
    for (size_t position = 0U; position < sizeof(identity_indices) / sizeof(identity_indices[0]); ++position) {
        OD_IO_t identity = get_io(identity_indices[position], 0U);
        assert(identity.stream.dataLength == identity_lengths[position]);
        assert((identity.stream.attribute & ODA_SDO_W) == 0U);
    }

    OD_IO_t tpdo5_cob_id = get_io(0x1804U, 1U);
    assert(tpdo5_cob_id.stream.dataLength == 4U);
    assert((tpdo5_cob_id.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t tpdo6_event_timer = get_io(0x1805U, 5U);
    assert(tpdo6_event_timer.stream.dataLength == 2U);
    assert((tpdo6_event_timer.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t tpdo5_map_count = get_io(0x1A04U, 0U);
    assert(tpdo5_map_count.stream.dataLength == 1U);
    assert((tpdo5_map_count.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t tpdo6_map_entry = get_io(0x1A05U, 1U);
    assert(tpdo6_map_entry.stream.dataLength == 4U);
    assert((tpdo6_map_entry.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t reserved_tpdo_subindex = {0};
    assert(OD_getSub(OD_find(OD, 0x1804U), 4U, &reserved_tpdo_subindex, false) == ODR_SUB_NOT_EXIST);

    OD_IO_t d000_count = get_io(0xD000U, 0U);
    assert(d000_count.stream.dataLength == 1U);
    assert((d000_count.stream.attribute & ODA_SDO_W) == 0U);
    uint8_t d000_highest = 0U;
    count_read = 0U;
    assert(d000_count.read != NULL);
    assert(d000_count.read(&d000_count.stream, &d000_highest, sizeof(d000_highest), &count_read) == ODR_OK);
    assert(count_read == sizeof(d000_highest));
    assert(d000_highest == 0x70U);

    OD_IO_t d000_ntc1 = get_io(0xD000U, 1U);
    assert(d000_ntc1.stream.dataLength == 2U);
    assert((d000_ntc1.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t d000_current = get_io(0xD000U, 0x64U);
    assert(d000_current.stream.dataLength == 2U);
    assert((d000_current.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t d000_rtc_time = get_io(0xD000U, 0x6CU);
    assert(d000_rtc_time.stream.dataLength == 4U);
    assert((d000_rtc_time.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t d000_eeprom = get_io(0xD000U, 0x6FU);
    assert(d000_eeprom.stream.dataLength == 2U);
    assert((d000_eeprom.stream.attribute & ODA_SDO_W) != 0U);
    OD_IO_t d000_balance = get_io(0xD000U, 0x70U);
    assert(d000_balance.stream.dataLength == 2U);
    assert((d000_balance.stream.attribute & ODA_SDO_W) != 0U);
    uint16_t d000_command = 0x3456U;
    OD_size_t count_written = 0U;
    assert(d000_balance.write != NULL);
    assert(d000_balance.write(&d000_balance.stream, &d000_command, sizeof(d000_command), &count_written) == ODR_OK);
    assert(count_written == sizeof(d000_command));

    for (uint8_t gap = 0x19U; gap <= 0x1AU; ++gap) {
        OD_IO_t unsupported_d000_subindex = {0};
        assert(OD_getSub(OD_find(OD, 0xD000U), gap, &unsupported_d000_subindex, false) == ODR_SUB_NOT_EXIST);
    }
    for (uint8_t gap = 0x25U; gap <= 0x27U; ++gap) {
        OD_IO_t unsupported_d000_subindex = {0};
        assert(OD_getSub(OD_find(OD, 0xD000U), gap, &unsupported_d000_subindex, false) == ODR_SUB_NOT_EXIST);
    }
    OD_IO_t unsupported_d000_29 = {0};
    assert(OD_getSub(OD_find(OD, 0xD000U), 0x29U, &unsupported_d000_29, false) == ODR_SUB_NOT_EXIST);
    OD_IO_t unsupported_d000_subindex = {0};
    assert(OD_getSub(OD_find(OD, 0xD000U), 0xFFU, &unsupported_d000_subindex, false) == ODR_SUB_NOT_EXIST);

    OD_IO_t d001_count = get_io(0xD001U, 0U);
    assert(d001_count.stream.dataLength == 1U);
    assert((d001_count.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t d001_byte = get_io(0xD001U, 1U);
    assert(d001_byte.stream.dataLength == 1U);
    assert((d001_byte.stream.attribute & ODA_SDO_W) != 0U);
    uint8_t diagnostic_value = 0x5AU;
    assert(d001_byte.write != NULL);
    count_written = 0U;
    assert(d001_byte.write(&d001_byte.stream, &diagnostic_value, sizeof(diagnostic_value), &count_written) == ODR_OK);
    assert(count_written == sizeof(diagnostic_value));
    OD_IO_t unsupported_d001_subindex = {0};
    assert(OD_getSub(OD_find(OD, 0xD001U), 0xFFU, &unsupported_d001_subindex, false) == ODR_SUB_NOT_EXIST);

    OD_IO_t bq_count = get_io(0x4900U, 0U);
    assert(bq_count.stream.dataLength == 1U);
    assert((bq_count.stream.attribute & ODA_SDO_W) == 0U);
    OD_IO_t bq = get_io(0x4900U, 1U);
    assert(bq.stream.dataLength == 2U);
    assert((bq.stream.attribute & ODA_SDO_W) == 0U);

    OD_IO_t soh = get_io(0x4800U, 0U);
    assert(soh.stream.dataLength == 1U);
    assert((soh.stream.attribute & ODA_SDO_W) == 0U);

    OD_IO_t sleep = get_io(0x4819U, 0U);
    assert(sleep.stream.dataLength == 2U);
    assert((sleep.stream.attribute & ODA_SDO_W) != 0U);
    uint16_t sleep_value = 1U;
    count_written = 0U;
    assert(sleep.write != NULL);
    assert(sleep.write(&sleep.stream, &sleep_value, sizeof(sleep_value), &count_written) == ODR_OK);
    assert(count_written == sizeof(sleep_value));

    printf("inventus_battery_od: PASS (%u core + %u Issue #12 application objects, structured D000, bounded D001)\\n",
           (unsigned)(sizeof(requested_indices) / sizeof(requested_indices[0])),
           (unsigned)(sizeof(issue12_indices) / sizeof(issue12_indices[0])));
    return 0;
}
