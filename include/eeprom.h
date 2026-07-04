#pragma once

#include "hwdefs.h"
#include <stdint.h>

#ifndef EEPROM_IS_EMULATED

#include <bitset>

class EEPROM
{
    static const uint32_t NUM_WORDS = 64;

    uint32_t words[NUM_WORDS];
    std::bitset<NUM_WORDS> populated;

    I2C_HandleTypeDef i2c{};
    static const uint32_t i2c_addr = 0x50 << 1;
public:
    void init();
    inline uint8_t read8(uint32_t addr) { return uint8_t(read32(addr &~ 3) >> ((addr & 3) << 3)); }
    uint32_t read32(uint32_t addr);
    void write8(uint32_t addr, uint8_t data);
    void write32(uint32_t addr, uint32_t data);
    void flush();
};

#else

class EEPROM
{
    static const uint32_t WORD_SIZE = sizeof(uint32_t);
    static const uint32_t NUM_WORDS = 64;
    static const uint32_t BANK_SIZE = 16384;
    static const uint32_t READY_ADDR = 1 + NUM_WORDS;

#ifdef DESKTOPSIM
    uint32_t eeprom_data[2][BANK_SIZE / WORD_SIZE];
#endif

    uint32_t words[NUM_WORDS];
    uint64_t dirty;
    uint16_t empty_ptr; // dodgy encoding: 2 low bits represent the number of words in the partial tuple
    int8_t cur_bank;
    int32_t cur_version;
public:
    EEPROM();
    void init();
    inline uint8_t read8(uint32_t addr) { return (words[addr >> 2] >> ((addr & 3) << 3)) & 0xFF; }
    inline uint32_t read32(uint32_t addr) { return words[addr >> 2]; }
    void write32(uint32_t addr, uint32_t data);
    void write8(uint32_t addr, uint8_t data);
    bool fetchBank(uint32_t *bank_data);
    void eraseBank(int bank);
    void initBank(int bank);
    void programWord(int num_word, uint32_t value);
    
    void flush();
};

#endif
