#include <stdio.h>

static const char* CPU_INST[] = {

//       0       |      1       |      2       |      3
//       4       |      5       |      6       |      7
//       8       |      9       |      A       |      B
//       C       |      D       |      E       |      F

   "NOP"         , "LD BC, d16" , "LD (BC), A" , "INC BC"    ,
   "INC B"       , "DEC B"      , "LD B, d8"   , "RLCA"      ,
   "LD (a16), SP", "ADD HL, BC" , "LD A, (BC)" , "DEC BC"    ,
   "INC C"       , "DEC C"      , "LD C, d8"   , "RRCA"      , // 0
	
   "STOP 0"      , "LD DE, d16" , "LD (DE), A" , "INC DE"    ,
   "INC D"       , "DEC D"      , "LD D, d8"   , "RLA"       ,
   "JR r8"       , "ADD HL, DE" , "LD A, (DE)" , "DEC DE"    ,
   "INC E"       , "DEC E"      , "LD E, d8"   , "RRA"       , // 1

   "JR NZ, r8"   , "LD HL, d16" , "LD (HL+), A", "INC HL"    ,
   "INC H"       , "DEC H"      , "LD H, d8"   , "DAA"       ,
   "JR Z, r8"    , "ADD HL, HL" , "LD A, (HL+)", "DEC HL"    ,
   "INC L"       , "DEC L"      , "LD L, d8"   , "CPL"       , // 2

   "JR NC r8"    , "LD SP, d16" , "LD (HL-), A", "INC SP"    ,
   "INC (HL)"    , "DEC (HL)"   , "LD (HL), d8", "SCF"       ,
   "JR C, r8"    , "ADD HL, SP" , "LD A, (HL-)", "DEC SP"    ,
   "INC A"       , "DEC A"      , "LD A, d8"   , "CCF"       , // 3

   "LD B, B"     , "LD B, C"    , "LD B, D"    , "LD B, E"   ,
   "LD B, H"     , "LD B, L"    , "LD B, (HL)" , "LD B, A"   ,
   "LD C, B"     , "LD C, C"    , "LD C, D"    , "LD C, E"   ,
   "LD C, H"     , "LD C, L"    , "LD C, (HL)" , "LD C, A"   , // 4

   "LD D, B"     , "LD D, C"    , "LD D, D"    , "LD D, E"   ,
   "LD D, H"     , "LD D, L"    , "LD D, (HL)" , "LD D, A"   ,
   "LD E, B"     , "LD E, C"    , "LD E, D"    , "LD E, E"   ,
   "LD E, H"     , "LD E, L"    , "LD E, (HL)" , "LD E, A"   , // 5

   "LD H, B"     , "LD H, C"    , "LD H, D"    , "LD H, E"   ,
   "LD H, H"     , "LD H, L"    , "LD H, (HL)" , "LD H, A"   ,
   "LD L, B"     , "LD L, C"    , "LD L, D"    , "LD L, E"   ,
   "LD L, H"     , "LD L, L"    , "LD L, (HL)" , "LD L, A"   , // 6

   "LD (HL), B"  , "LD (HL), C" , "LD (HL), D" , "LD (HL), E",
   "LD (HL), H"  , "LD (HL), L" , "HALT"       , "LD (HL), A",
   "LD A, B4"    , "LD A, C"    , "LD A, D"    , "LD A, E"   ,
   "LD A, H"     , "LD A, L"    , "LD A, (HL)" , "LD A, A"   , // 7

   "ADD B"       , "ADD C"      , "ADD D"      , "ADD E"     ,
   "ADD H"       , "ADD L"      , "ADD (HL)"   , "ADD A"     ,
   "ADC B"       , "ADC C"      , "ADC D"      , "ADC E"     ,
   "ADC H"       , "ADC L"      , "ADC (HL)"   , "ADC A"     , // 8
   
   "SUB B"       , "SUB C"      , "SUB D"      , "SUB E"     ,
   "SUB H"       , "SUB L"      , "SUB (HL)"   , "SUB A"     ,
   "SBC B"       , "SBC C"      , "SBC D"      , "SBC E"     ,
   "SBC H"       , "SBC L"      , "SBC (HL)"   , "SBC A"     , // 9

   "AND B"       , "AND C"      , "AND D"      , "AND E"     ,
   "AND H"       , "AND L"      , "AND (HL)"   , "AND A"     ,
   "XOR B"       , "XOR C"      , "XOR D"      , "XOR E"     ,
   "XOR H"       , "XOR L"      , "XOR (HL)"   , "XOR A"     , // A

   "OR B"        , "OR C"       , "OR D"       , "OR E"      ,
   "OR H"        , "OR L"       , "OR (HL)"    , "OR A"      ,
   "CP B"        , "CP C"       , "CP D"       , "CP E"      ,
   "CP H"        , "CP L"       , "CP (HL)"    , "CP A"      , // B

   "RET NZ"      , "POP BC"     , "JP NZ, a16" , "JP a16"    ,
   "CALL NZ, a16", "PUSH BC"    , "ADD d8"     , "RST 00H"   ,
   "RET Z"       , "RET"        , "JP Z, a16"  , "PREFIX CB" ,
   "CALL Z, a16" , "CALL a16"   , "ADC d8"     , "RST 08H"   , // C

   "RET NC"      , "POP DE"     , "JP NC, a16" , "-"         ,
   "CALL NC, a16", "PUSH DE"    , "SUB d8"     , "RST 10H"   ,
   "RET C"       , "RETI"       , "JP C, a16"  , "-"         ,
   "CALL C, a16" , "-"          , "SBC d8"     , "RST 18H"   , // D

   "LDH (a8), A" , "POP HL"     , "LD (C), A"  , "-"         ,
   "-"           , "PUSH HL"    , "AND d8"     , "RST 20H"   ,
   "ADD SP, r8"  , "JP (HL)"    , "LD (a16), A", "-"         ,
   "-"           , "-"          , "XOR d8"     , "RST 28H"   , // E

   "LDH (a8)"    , "POP AF"     , "LD (C)"     , "DI"        ,
   "-"           , "PUSH AF"    , "OR d8"      , "RST 30H"   ,
   "LD HL, SP+r8", "LD SP, HL"  , "LD (a16)"   , "EI"        ,
   "-"           , "-"          , "CP d8"      , "RST 38H"     // F
};
