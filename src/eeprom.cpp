#include "eeprom.h"

#ifdef DESKTOPSIM
#include <cstdio>
#include <cassert>
#define pcassert assert
#else
#define pcassert(...)
#include "gpio.h"
#endif
#include <cstring>

// Bank layout (16 KB each, corresponding to sectors 2 and 3 on STM32F4xx):
// Word 0: version number (incremented each time a new bank is started)
// Words 1-64: initial data
// Word 65: written as 0 to indicate that the bank is fully initialized
// Words 66...: sequences of this variable tuple structure:
//              1 word represents up to 4 word indices (until the index of 0xFF = EOF, indices might be non-unique if the same word has been written multiple times in a row)
//              the subsequent up to 4 words represent values at those indices
// This is designed to allow flash-friendly writing using whole word writes and to permit appending to the last tuple if it has less than 4 values in it to minimize flash wear

EEPROM::EEPROM()
: words{}
, dirty{}
{
#ifdef DESKTOPSIM
    memset(eeprom_data, 0xFF, sizeof(eeprom_data));
    FILE *f = fopen("eeprom.bin", "rb");
    if (f) {
        fread(&eeprom_data, 1, 2 * BANK_SIZE, f);
        fclose(f);
    }
#endif
}

void EEPROM::init()
{
#ifdef DESKTOPSIM
    uint32_t *banks[] = {&eeprom_data[0][0], &eeprom_data[1][0]};
#else
    uint32_t *banks[] = {(uint32_t *)0x08008000, (uint32_t *)0x0800C000};
#endif
    memset(words, 0, sizeof(words));
    dirty = 0;
    
    int ver1 = banks[0][READY_ADDR] == 0 ? banks[0][0] : -1;
    int ver2 = banks[1][READY_ADDR] == 0 ? banks[1][0] : -1;
    if (ver1 == -1 && ver2 == -1) {
        // Fresh flash with no entries
        cur_bank = -1;
        cur_version = -1;
    } else if (ver2 == -1) {
        // Bank 1 is empty, bank 0 is not
        cur_bank = 0;
        cur_version = ver1;
    } else if (ver1 == -1) {
        // The other way around (
        cur_bank = 1;
        cur_version = ver2;
    } else if (ver1 > ver2) {
        cur_bank = 0;
        cur_version = ver1;
    } else if (ver2 > ver1) {
        cur_bank = 1;
        cur_version = ver2;
    } else if (ver1 == ver2) {
        // Identical version? Must be a bug. Use bank 0 anyway.
        // XXXKF might put in some heurestic here
        cur_bank = 0;
        cur_version = ver1;
    }
    if (cur_bank != -1 && cur_version != -1) {
        fetchBank(banks[cur_bank]);
    } else {
        eraseBank(0);
        initBank(0);
        flush();
    }
}

void EEPROM::write32(uint32_t addr, uint32_t data) {
    pcassert(!(addr & 3));
    pcassert(addr >= 0 && addr < NUM_WORDS * WORD_SIZE);
    if (words[addr >> 2] == data)
        return;
    words[addr >> 2] = data;
    dirty |= uint64_t{1} << (addr >> 2);
}

void EEPROM::write8(uint32_t addr, uint8_t data) {
    uint32_t shift = (addr & 3) << 3;
    uint32_t oldv = words[addr >> 2];
    write32(addr &~ 3, (oldv &~ (0xFF << shift)) | (uint32_t{data} << shift));
}


bool EEPROM::fetchBank(uint32_t *bank_data)
{
    memcpy(words, &bank_data[1], NUM_WORDS * WORD_SIZE);
    dirty = 0;
    const uint32_t *bank_end = bank_data + BANK_SIZE / WORD_SIZE;
    const uint32_t *p = bank_data + (READY_ADDR + 1);
    while (p < bank_end) {
        if (*p == 0xFFFFFFFF) {
            empty_ptr = (p - bank_data) * WORD_SIZE;
            return true;
        }
        const uint8_t *indices = reinterpret_cast<const uint8_t *>(p++);
        int i;
        for (i = 0; i < 4; ++i) {
            if (indices[i] == 0xFF) {
                empty_ptr = (p - bank_data) * WORD_SIZE + i;
                return true;
            }
            if (indices[i] >= NUM_WORDS) {
                empty_ptr = BANK_SIZE;
                return false;
            }
            words[indices[i]] = *p++;
        }
    }
    empty_ptr = BANK_SIZE;
    return true;
}

void EEPROM::eraseBank(int bank)
{
#ifdef DESKTOPSIM
    memset(&eeprom_data[bank][0], 0xFF, BANK_SIZE);
#else
    FLASH_EraseInitTypeDef erase_data;
    erase_data.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_data.Banks = FLASH_BANK_1;
    erase_data.Sector = FLASH_SECTOR_2 + bank; // bootloader covers sectors 0-1, sectors 2-3 are for EEPROM sim, sectors 4-end are for the application
    erase_data.NbSectors = 1;
    erase_data.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t sector_error = 0;
    HAL_FLASHEx_Erase(&erase_data, &sector_error);
#endif
}

void EEPROM::initBank(int bank)
{
#ifndef DESKTOPSIM
    HAL_FLASH_Unlock();
#endif
    cur_version++;
    cur_bank = bank;
    eraseBank(cur_bank);
    programWord(0, cur_version);
    for (uint32_t i = 0; i < NUM_WORDS; ++i) {
        programWord(i + 1, words[i]);
    }
    programWord(READY_ADDR, 0);
    dirty = 0;
    empty_ptr = (READY_ADDR + 1) * WORD_SIZE;
#ifndef DESKTOPSIM
    HAL_FLASH_Lock();
#endif
}

void EEPROM::programWord(int addr, uint32_t value)
{
#ifdef DESKTOPSIM
    uint32_t mem = eeprom_data[cur_bank][addr];
    // Verify that we're not trying to change any bits from '0' to '1' - that would require an erase operation!
    pcassert((mem | value) == mem);
    eeprom_data[cur_bank][addr] = value;
#else
    uint32_t base = 0x08008000 + 0x4000 * cur_bank;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base + addr * 4, value);
#endif
}

// Possible tuple formats:
// FFFFFFFF
// FFFFFFi0 w0
// FFFFi1i0 w0 w1
// FFi2i1i0 w0 w1 w2
// i3i2i1i0 w0 w1 w2 w3

void EEPROM::flush()
{
#ifdef DESKTOPSIM
    uint32_t *banks[] = {&eeprom_data[0][0], &eeprom_data[1][0]};
#else
    uint32_t *banks[] = {(uint32_t *)0x08008000, (uint32_t *)0x0800C000};
#endif
    if(dirty) {
        uint32_t words_needed = 0;
        for (uint32_t i = 0; i < NUM_WORDS; ++i) {
            words_needed += ((dirty >> i) & 1);
        }
        // How many words can we stuff into the most recent indices entry
        uint32_t extra = (empty_ptr & 3) ? (4 - (empty_ptr & 3)) : 0;
        // Count how many extra indices entries we need - 1 for any
        // started group of 4 except for ones that we can stuff into the
        // previous entry.
        words_needed += (words_needed + 3 - extra) >> 2;
        if ((empty_ptr & ~3) + words_needed * WORD_SIZE >= BANK_SIZE) {
            initBank((cur_bank == 0) ? 1 : 0);            
        } else {
#ifndef DESKTOPSIM
            HAL_FLASH_Unlock();
#endif
            for (uint32_t i = 0; i < NUM_WORDS; ++i) {
                if ((dirty >> i) & 1) {
                    uint32_t which = empty_ptr & 3;
                    if (which) {
                        pcassert(empty_ptr < BANK_SIZE);
                        // Appending to an existing entry
                        uint32_t shift = which << 3;
                        uint32_t addr = empty_ptr >> 2;
                        uint32_t mask = 0xFF << shift;
                        // Write i to indices at which'th byte
                        uint32_t indices = banks[cur_bank][addr - which - 1];
                        pcassert((indices & mask) == mask);
                        programWord(addr - which - 1, (indices &~ mask) | (i << shift));
                        programWord(addr, words[i]);
                        if (which == 3)
                            empty_ptr += WORD_SIZE - 3;
                        else
                            empty_ptr += WORD_SIZE + 1;
                    } else {
                        // First word out of four
                        programWord(empty_ptr >> 2, i | 0xFFFFFF00);
                        programWord((empty_ptr >> 2) + 1, words[i]);
                        empty_ptr += 2 * WORD_SIZE + 1;
                    }
                }
            }
#ifndef DESKTOPSIM
            HAL_FLASH_Lock();
#endif
            dirty = 0;
        }
    }
#ifdef DESKTOPSIM
    FILE *f = fopen("eeprom.bin", "wb");
    if (f) {
        fwrite(eeprom_data, 1, 2 * BANK_SIZE, f);
        fclose(f);
    }
#endif
}