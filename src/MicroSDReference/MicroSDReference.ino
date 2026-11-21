// SPDX-License-Identifier: Apache-2.0
// Slot-0/LDO configuration follows Espressif SD_MMC.cpp in M5Stack core 3.3.10.
// Auxiliary Arduino/ESP-IDF reference, not a native MaiaRTOS driver.
#include <Arduino.h>
#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>
#include <sd_pwr_ctrl_by_on_chip_ldo.h>

static sdmmc_card_t card;
static sd_pwr_ctrl_handle_t power;
static uint8_t sector[512] __attribute__((aligned(64)));
static uint8_t fat_sectors[2][512] __attribute__((aligned(64)));
static const uint32_t lbas[] = {0, 1, 32768, 32774};
static const uint32_t fat_scan_limit = 4096U;

static uint16_t read_le16(const uint8_t *data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8U);
}

static uint32_t read_le32(const uint8_t *data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8U) |
           (static_cast<uint32_t>(data[2]) << 16U) |
           (static_cast<uint32_t>(data[3]) << 24U);
}

static bool read_sector(uint64_t lba) {
    if (lba >= card.csd.capacity) return false;
    return sdmmc_read_sectors(&card, sector, static_cast<size_t>(lba), 1) == ESP_OK;
}

static bool find_free_fat32_cluster(uint32_t *free_cluster, uint32_t *free_lba) {
    if (!read_sector(0U)) {
        Serial.println("REFERENCE_FAT_FAIL reason=mbr-read");
        return false;
    }
    if (sector[510] != 0x55U || sector[511] != 0xAAU) {
        Serial.println("REFERENCE_FAT_FAIL reason=mbr-signature");
        return false;
    }

    uint32_t volume_start = 0U;
    uint32_t partition_sectors = 0U;
    for (unsigned partition = 0U; partition < 4U; ++partition) {
        const uint8_t *entry = sector + 446U + partition * 16U;
        if (entry[4] == 0x0BU || entry[4] == 0x0CU) {
            volume_start = read_le32(entry + 8U);
            partition_sectors = read_le32(entry + 12U);
            break;
        }
    }
    if (volume_start == 0U || partition_sectors == 0U ||
        static_cast<uint64_t>(volume_start) + partition_sectors > card.csd.capacity ||
        !read_sector(volume_start)) {
        Serial.println("REFERENCE_FAT_FAIL reason=fat32-partition");
        return false;
    }
    if (sector[510] != 0x55U || sector[511] != 0xAAU) {
        Serial.println("REFERENCE_FAT_FAIL reason=vbr-signature");
        return false;
    }

    const uint16_t bytes_per_sector = read_le16(sector + 11U);
    const uint8_t sectors_per_cluster = sector[13];
    const uint16_t reserved_sectors = read_le16(sector + 14U);
    const uint8_t fat_count = sector[16];
    const uint16_t root_entries = read_le16(sector + 17U);
    const uint16_t total_sectors_16 = read_le16(sector + 19U);
    const uint16_t fat_size_16 = read_le16(sector + 22U);
    const uint32_t total_sectors = total_sectors_16 != 0U
        ? total_sectors_16 : read_le32(sector + 32U);
    const uint32_t fat_size = read_le32(sector + 36U);
    const uint16_t extended_flags = read_le16(sector + 40U);
    const uint16_t filesystem_version = read_le16(sector + 42U);
    const uint32_t root_cluster = read_le32(sector + 44U) & UINT32_C(0x0FFFFFFF);
    const bool mirrored = (extended_flags & 0x80U) == 0U;
    const uint8_t active_fat = static_cast<uint8_t>(extended_flags & 0x0FU);

    if (bytes_per_sector != sizeof(sector) ||
        sectors_per_cluster == 0U ||
    sectors_per_cluster > 128U ||
    (sectors_per_cluster & (sectors_per_cluster - 1U)) != 0U ||
        reserved_sectors == 0U || fat_count == 0U || fat_count > 2U ||
        root_entries != 0U || fat_size_16 != 0U || fat_size == 0U ||
        filesystem_version != 0U || root_cluster < 2U ||
        total_sectors == 0U || total_sectors > partition_sectors ||
        (!mirrored && active_fat >= fat_count)) {
        Serial.println("REFERENCE_FAT_FAIL reason=bpb-fields");
        return false;
    }

    const uint64_t fat_area = static_cast<uint64_t>(fat_count) * fat_size;
    const uint64_t first_data_sector = reserved_sectors + fat_area;
    if (first_data_sector >= total_sectors) {
        Serial.println("REFERENCE_FAT_FAIL reason=fat-layout");
        return false;
    }
    const uint64_t cluster_count_wide =
        (total_sectors - first_data_sector) / sectors_per_cluster;
    const uint64_t fat_entry_capacity = static_cast<uint64_t>(fat_size) * 128U;
    if (cluster_count_wide < 65525U ||
        cluster_count_wide > UINT32_MAX - 2U ||
        fat_entry_capacity < cluster_count_wide + 2U) {
        Serial.println("REFERENCE_FAT_FAIL reason=fat32-geometry");
        return false;
    }
    const uint32_t cluster_count = static_cast<uint32_t>(cluster_count_wide);
    if (root_cluster >= cluster_count + 2U) {
        Serial.println("REFERENCE_FAT_FAIL reason=root-cluster");
        return false;
    }

    const uint32_t sectors_to_scan =
        fat_size < fat_scan_limit ? fat_size : fat_scan_limit;
    for (uint32_t fat_sector_index = 0U;
         fat_sector_index < sectors_to_scan; ++fat_sector_index) {
        for (uint8_t fat_copy = 0U; fat_copy < fat_count; ++fat_copy) {
            if (!mirrored && fat_copy != active_fat) continue;
            const uint64_t fat_lba = static_cast<uint64_t>(volume_start) +
                reserved_sectors + static_cast<uint64_t>(fat_copy) * fat_size +
                fat_sector_index;
            if (!read_sector(fat_lba)) {
                Serial.println("REFERENCE_FAT_FAIL reason=fat-read");
                return false;
            }
            for (unsigned byte = 0U; byte < sizeof(sector); ++byte)
                fat_sectors[fat_copy][byte] = sector[byte];
        }

        for (uint32_t entry_index = 0U; entry_index < 128U; ++entry_index) {
            const uint32_t cluster = fat_sector_index * 128U + entry_index;
            if (cluster < 2U) continue;
            if (cluster >= cluster_count + 2U) break;

            bool free_in_authoritative_fats = true;
            for (uint8_t fat_copy = 0U; fat_copy < fat_count; ++fat_copy) {
                if (!mirrored && fat_copy != active_fat) continue;
                const uint32_t value = read_le32(
                    fat_sectors[fat_copy] + entry_index * 4U) & UINT32_C(0x0FFFFFFF);
                if (value != 0U) free_in_authoritative_fats = false;
            }
            if (!free_in_authoritative_fats) continue;

            const uint64_t data_lba = static_cast<uint64_t>(volume_start) +
                first_data_sector +
                static_cast<uint64_t>(cluster - 2U) * sectors_per_cluster;
            if (data_lba > UINT32_MAX ||
                data_lba + sectors_per_cluster >
                    static_cast<uint64_t>(volume_start) + total_sectors ||
                !read_sector(data_lba)) {
                Serial.println("REFERENCE_FAT_FAIL reason=free-cluster-read");
                return false;
            }

            *free_cluster = cluster;
            *free_lba = static_cast<uint32_t>(data_lba);
            Serial.printf(
                "REFERENCE_FAT32 start=%lu sectors=%lu spc=%u reserved=%u fats=%u "
                "fat_size=%lu mirrored=%u active_fat=%u clusters=%lu scan_limit=%lu\n",
                static_cast<unsigned long>(volume_start),
                static_cast<unsigned long>(total_sectors),
                static_cast<unsigned>(sectors_per_cluster),
                static_cast<unsigned>(reserved_sectors),
                static_cast<unsigned>(fat_count),
                static_cast<unsigned long>(fat_size),
                mirrored ? 1U : 0U,
                static_cast<unsigned>(active_fat),
                static_cast<unsigned long>(cluster_count),
                static_cast<unsigned long>(sectors_to_scan));
            Serial.printf(
                "REFERENCE_FREE_CLUSTER cluster=%lu lba=%lu sectors=%u\n",
                static_cast<unsigned long>(cluster),
                static_cast<unsigned long>(*free_lba),
                static_cast<unsigned>(sectors_per_cluster));
            for (unsigned offset = 0U; offset < sizeof(sector); offset += 16U) {
                Serial.printf("REFERENCE_FREE_DATA lba=%08lx offset=%08x data=",
                              static_cast<unsigned long>(*free_lba), offset);
                for (unsigned i = 0U; i < 16U; ++i)
                    Serial.printf("%02x", static_cast<unsigned>(sector[offset + i]));
                Serial.println();
            }
            return true;
        }
    }

    Serial.println("REFERENCE_FAT_FAIL reason=no-free-cluster-in-scan-window");
    return false;
}

static void failure(const char *stage, esp_err_t error) {
    Serial.printf("MAIA_SDMMC_REFERENCE_FAIL stage=%s error=%s\n",
                  stage, esp_err_to_name(error));
    Serial.flush();
}

void setup() {
    Serial.begin(115200);
    const uint32_t started = millis();
    while (!Serial && millis() - started < 10000) {
        delay(10);
    }
    Serial.println("MAIA_SDMMC_REFERENCE_READY schema=1");
    const uint32_t ready = millis();
    bool requested = false;
    while (millis() - ready < 30000) {
        if (Serial.available() && Serial.read() == 'R') {
            requested = true;
            break;
        }
        delay(10);
    }
    if (!requested) {
        failure("host-timeout", ESP_ERR_TIMEOUT);
        return;
    }

    sd_pwr_ctrl_ldo_config_t ldo = {};
    ldo.ldo_chan_id = 4;
    esp_err_t error = sd_pwr_ctrl_new_on_chip_ldo(&ldo, &power);
    if (error != ESP_OK) {
        failure("power", error);
        return;
    }
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.flags = SDMMC_HOST_FLAG_4BIT;
    host.max_freq_khz = 20000;
    host.pwr_ctrl_handle = power;

    // Slot 0 uses IO_MUX GPIO39..44; zero pin fields select the fixed routing.
    sdmmc_slot_config_t slot = {};
    slot.cd = SDMMC_SLOT_NO_CD;
    slot.wp = SDMMC_SLOT_NO_WP;
    slot.width = 4;
    error = sdmmc_host_init();
    if (error != ESP_OK) {
        failure("host", error);
        return;
    }
    error = sdmmc_host_init_slot(host.slot, &slot);
    if (error != ESP_OK) {
        failure("slot", error);
        return;
    }
    error = sdmmc_card_init(&host, &card);
    if (error != ESP_OK) {
        failure("card", error);
        return;
    }
    if (card.csd.sector_size != sizeof(sector)) {
        failure("sector-size", ESP_ERR_INVALID_SIZE);
        return;
    }
    Serial.println("MAIA_SDMMC_REFERENCE schema=1 slot=0 width=4 read-only raw");
    Serial.printf("REFERENCE_CARD blocks=%llu sector_size=%u ocr=%08lx\n",
                  static_cast<unsigned long long>(card.csd.capacity),
                  static_cast<unsigned>(card.csd.sector_size),
                  static_cast<unsigned long>(card.ocr));
    for (uint32_t lba : lbas) {
        if (lba >= card.csd.capacity) {
            failure("lba-range", ESP_ERR_INVALID_ARG);
            return;
        }
        memset(sector, 0xA5, sizeof(sector));
        error = sdmmc_read_sectors(&card, sector, lba, 1);
        if (error != ESP_OK) {
            failure("read", error);
            return;
        }
        for (unsigned offset = 0; offset < sizeof(sector); offset += 16) {
            Serial.printf("REFERENCE_DATA lba=%08lx offset=%08x data=",
                          static_cast<unsigned long>(lba), offset);
            for (unsigned i = 0; i < 16; ++i) {
                Serial.printf("%02x", static_cast<unsigned>(sector[offset + i]));
            }
            Serial.println();
        }
    }
    uint32_t free_cluster = 0U;
    uint32_t free_lba = 0U;
    if (!find_free_fat32_cluster(&free_cluster, &free_lba)) return;
    Serial.println("MAIA_SDMMC_REFERENCE_PASS");
    Serial.flush();
}

void loop() {
    delay(100);
}
