#include <stdbool.h>
#include <stdio.h>
#include "cpu.h"
#include "main.h"

#define REGION_ROM_HEADER 0x0100

bool mode_bios = true;

// MMU read functions
uint8_t mmu_rb(const uint16_t addr, const uint8_t memcyc)
{
  cpu->clock.cur_mem += memcyc; // memory byte cycle

  switch(addr & 0xf000)
  {
    // BIOS / ROM
    case 0x0000:
      if (mode_bios)
      {
        if (cpu->registers.pc != REGION_ROM_HEADER)
          return m_bios[addr];
        else
          mode_bios = false;
      }

      return m_rom[addr];

    // ROM 0
    case 0x1000:
    case 0x2000:
    case 0x3000:
      return m_rom[addr];

    // ROM bank (extension)
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
      return m_rom[addr];

    // Graphics
    case 0x8000:
    case 0x9000:
      return m_vram[addr & SIZE_VRAM];

    // External RAM
    case 0xA000:
    case 0xB000:
      return e_ram[addr & SIZE_ERAM];

    // Working RAM
    case 0xC000:
    case 0xD000:
      return w_ram[addr & SIZE_WRAM];

    // WRAM Mirror
    case 0xE000:
      return w_ram[addr & SIZE_WRAM];

    // WRAM Mirror / OAM / IO / fast ram
    case 0xF000:
      switch(addr & 0xF00)
      {
        // WRAM Mirror
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00:
          return w_ram[addr & SIZE_WRAM];

        // Graphics OAM
        case 0xE00:
          if (addr < 0xFEA0) // 0xFE00 = loc., 0xA0 = 160 bytes
            return g_oam[addr & SIZE_OAM];
          else
            return 0;

        // Zero Page
        case 0xF00:
          if (addr >= 0xFF80)
            return z_ram[addr & 0x7F];
          else
            return 0; // unhandled
      }

    default:
      printf("MMU: Address %04x unhandled.\n", addr);
      return 0;
  }
}

// DEPRECATED
/*
static inline uint16_t mmu_rw(const uint16_t addr)
{
  return mmu_rb(addr) | mmu_rb(addr + 1) << 8;
}
*/

// MMU write to memory functions
void mmu_wb(const uint16_t addr, const uint8_t data)
{
  switch(addr & 0xf000)
  {
    // BIOS / ROM
    case 0x0000:
      if (mode_bios)
      {
          m_bios[addr] = data;
          break;
      }

      m_rom[addr] = data;
      break;

    // ROM 0
    case 0x1000:
    case 0x2000:
    case 0x3000:
      m_rom[addr] = data;
      break;

    // ROM bank (extension)
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
      m_rom[addr] = data;
      break;

    // Graphics
    case 0x8000:
    case 0x9000:
      m_vram[addr & SIZE_VRAM] = data;
      break;

    // External RAM
    case 0xA000:
    case 0xB000:
      e_ram[addr & SIZE_ERAM] = data;
      break;

    // Working RAM
    case 0xC000:
    case 0xD000:
      w_ram[addr & SIZE_WRAM] = data;
      break;

    // WRAM Mirror
    case 0xE000:
      w_ram[addr & SIZE_WRAM] = data;
      break;

    // WRAM Mirror / OAM / IO / fast ram
    case 0xF000:
      switch(addr & 0xF00)
      {
        // WRAM Mirror
        case 0x000: case 0x100: case 0x200: case 0x300:
        case 0x400: case 0x500: case 0x600: case 0x700:
        case 0x800: case 0x900: case 0xA00: case 0xB00:
        case 0xC00: case 0xD00:
          w_ram[addr & SIZE_WRAM] = data;
          break;

        // Graphics OAM
        case 0xE00:
          if (addr < 0xFEA0) // 0xFE00 = loc., 0xA0 = 160 bytes
          {
            g_oam[addr & SIZE_OAM] = data;
          }
          break;

        // Zero page
        case 0xF00:
          if (addr >= 0xFF80)
            z_ram[addr & 0x7F] = data;
            break;
      }
    }
}
