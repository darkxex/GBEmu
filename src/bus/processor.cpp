/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "processor.h"
#include <string.h>

static struct {
    bool halt_bug;
    bool halted;
    bool stopped;
    uint8_t delay;
    gb_error_e status;
    gb_register_t af;
    gb_register_t bc;
    gb_register_t de;
    gb_register_t hl;
    gb_register_t pc;
    gb_register_t sp;
    struct {
        uint16_t address;
        gb_instruction_e opcode;
    } instruction;
    struct {
        bool enabled;
        uint8_t delay;
        gb_interrupt_t enable;
        gb_interrupt_t flag;
    } interrupt;
} processor = {};

static void gb_processor_instruction_adc(void) {
    uint16_t carry = 0, sum = 0;
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_ADC_A:
        operand.low = processor.af.high;
        break;
    case GB_INSTRUCTION_ADC_B:
        operand.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_ADC_C:
        operand.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_ADC_D:
        operand.low = processor.de.high;
        break;
    case GB_INSTRUCTION_ADC_E:
        operand.low = processor.de.low;
        break;
    case GB_INSTRUCTION_ADC_H:
        operand.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_ADC_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_ADC_L:
        operand.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_ADC_N:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    sum = processor.af.high + operand.low + processor.af.carry;
    carry = processor.af.high ^ operand.low ^ processor.af.carry ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = false;
    processor.af.zero = !(sum & 0xFF);
    processor.af.high = sum;
}

static void gb_processor_instruction_add(void) {
    uint16_t carry = 0, sum = 0;
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_ADD_A:
        operand.low = processor.af.high;
        break;
    case GB_INSTRUCTION_ADD_B:
        operand.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_ADD_C:
        operand.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_ADD_D:
        operand.low = processor.de.high;
        break;
    case GB_INSTRUCTION_ADD_E:
        operand.low = processor.de.low;
        break;
    case GB_INSTRUCTION_ADD_H:
        operand.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_ADD_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_ADD_L:
        operand.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_ADD_N:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    sum = processor.af.high + operand.low;
    carry = processor.af.high ^ operand.low ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = false;
    processor.af.zero = !(sum & 0xFF);
    processor.af.high = sum;
}

static void gb_processor_instruction_add_hl(void) {
    uint32_t carry = 0, sum = 0;
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_ADD_HL_BC:
        operand.word = processor.bc.word;
        break;
    case GB_INSTRUCTION_ADD_HL_DE:
        operand.word = processor.de.word;
        break;
    case GB_INSTRUCTION_ADD_HL_HL:
        operand.word = processor.hl.word;
        break;
    case GB_INSTRUCTION_ADD_HL_SP:
        operand.word = processor.sp.word;
        break;
    default:
        break;
    }
    sum = processor.hl.word + operand.word;
    carry = processor.hl.word ^ operand.word ^ sum;
    processor.af.carry = ((carry & 0x10000) == 0x10000);
    processor.af.half_carry = ((carry & 0x1000) == 0x1000);
    processor.af.negative = false;
    processor.hl.word = sum;
}

static void gb_processor_instruction_add_sp(void) {
    uint32_t carry = 0, sum = 0;
    gb_register_t operand = { .low = gb_bus_read(processor.pc.word++) };
    processor.delay = 16;
    sum = processor.sp.word + (int8_t)operand.low;
    carry = processor.sp.word ^ (int8_t)operand.low ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = false;
    processor.af.zero = false;
    processor.sp.word = sum;
}

static void gb_processor_instruction_and(void) {
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_AND_A:
        break;
    case GB_INSTRUCTION_AND_B:
        processor.af.high &= processor.bc.high;
        break;
    case GB_INSTRUCTION_AND_C:
        processor.af.high &= processor.bc.low;
        break;
    case GB_INSTRUCTION_AND_D:
        processor.af.high &= processor.de.high;
        break;
    case GB_INSTRUCTION_AND_E:
        processor.af.high &= processor.de.low;
        break;
    case GB_INSTRUCTION_AND_H:
        processor.af.high &= processor.hl.high;
        break;
    case GB_INSTRUCTION_AND_HLI:
        processor.delay += 4;
        processor.af.high &= gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_AND_L:
        processor.af.high &= processor.hl.low;
        break;
    case GB_INSTRUCTION_AND_N:
        processor.delay += 4;
        processor.af.high &= gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    processor.af.carry = false;
    processor.af.half_carry = true;
    processor.af.negative = false;
    processor.af.zero = !processor.af.high;
}

static void gb_processor_instruction_bit(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_BIT_0_A:
    case GB_INSTRUCTION_BIT_1_A:
    case GB_INSTRUCTION_BIT_2_A:
    case GB_INSTRUCTION_BIT_3_A:
    case GB_INSTRUCTION_BIT_4_A:
    case GB_INSTRUCTION_BIT_5_A:
    case GB_INSTRUCTION_BIT_6_A:
    case GB_INSTRUCTION_BIT_7_A:
        processor.af.zero = !(processor.af.high & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_A) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_B:
    case GB_INSTRUCTION_BIT_1_B:
    case GB_INSTRUCTION_BIT_2_B:
    case GB_INSTRUCTION_BIT_3_B:
    case GB_INSTRUCTION_BIT_4_B:
    case GB_INSTRUCTION_BIT_5_B:
    case GB_INSTRUCTION_BIT_6_B:
    case GB_INSTRUCTION_BIT_7_B:
        processor.af.zero = !(processor.bc.high & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_B) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_C:
    case GB_INSTRUCTION_BIT_1_C:
    case GB_INSTRUCTION_BIT_2_C:
    case GB_INSTRUCTION_BIT_3_C:
    case GB_INSTRUCTION_BIT_4_C:
    case GB_INSTRUCTION_BIT_5_C:
    case GB_INSTRUCTION_BIT_6_C:
    case GB_INSTRUCTION_BIT_7_C:
        processor.af.zero = !(processor.bc.low & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_C) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_D:
    case GB_INSTRUCTION_BIT_1_D:
    case GB_INSTRUCTION_BIT_2_D:
    case GB_INSTRUCTION_BIT_3_D:
    case GB_INSTRUCTION_BIT_4_D:
    case GB_INSTRUCTION_BIT_5_D:
    case GB_INSTRUCTION_BIT_6_D:
    case GB_INSTRUCTION_BIT_7_D:
        processor.af.zero = !(processor.de.high & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_D) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_E:
    case GB_INSTRUCTION_BIT_1_E:
    case GB_INSTRUCTION_BIT_2_E:
    case GB_INSTRUCTION_BIT_3_E:
    case GB_INSTRUCTION_BIT_4_E:
    case GB_INSTRUCTION_BIT_5_E:
    case GB_INSTRUCTION_BIT_6_E:
    case GB_INSTRUCTION_BIT_7_E:
        processor.af.zero = !(processor.de.low & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_E) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_H:
    case GB_INSTRUCTION_BIT_1_H:
    case GB_INSTRUCTION_BIT_2_H:
    case GB_INSTRUCTION_BIT_3_H:
    case GB_INSTRUCTION_BIT_4_H:
    case GB_INSTRUCTION_BIT_5_H:
    case GB_INSTRUCTION_BIT_6_H:
    case GB_INSTRUCTION_BIT_7_H:
        processor.af.zero = !(processor.hl.high & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_H) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_HLI:
    case GB_INSTRUCTION_BIT_1_HLI:
    case GB_INSTRUCTION_BIT_2_HLI:
    case GB_INSTRUCTION_BIT_3_HLI:
    case GB_INSTRUCTION_BIT_4_HLI:
    case GB_INSTRUCTION_BIT_5_HLI:
    case GB_INSTRUCTION_BIT_6_HLI:
    case GB_INSTRUCTION_BIT_7_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.zero = !(operand.low & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_HLI) >> 3)));
        break;
    case GB_INSTRUCTION_BIT_0_L:
    case GB_INSTRUCTION_BIT_1_L:
    case GB_INSTRUCTION_BIT_2_L:
    case GB_INSTRUCTION_BIT_3_L:
    case GB_INSTRUCTION_BIT_4_L:
    case GB_INSTRUCTION_BIT_5_L:
    case GB_INSTRUCTION_BIT_6_L:
    case GB_INSTRUCTION_BIT_7_L:
        processor.af.zero = !(processor.hl.low & (1 << ((processor.instruction.opcode - GB_INSTRUCTION_BIT_0_L) >> 3)));
        break;
    default:
        break;
    }
    processor.af.half_carry = true;
    processor.af.negative = false;
}

static void gb_processor_instruction_call(void) {
    bool taken = false;
    gb_register_t operand = { .low = gb_bus_read(processor.pc.word++), .high = gb_bus_read(processor.pc.word++) };
    processor.delay = 12;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_CALL:
        taken = true;
        break;
    case GB_INSTRUCTION_CALL_C:
        taken = processor.af.carry;
        break;
    case GB_INSTRUCTION_CALL_NC:
        taken = !processor.af.carry;
        break;
    case GB_INSTRUCTION_CALL_NZ:
        taken = !processor.af.zero;
        break;
    case GB_INSTRUCTION_CALL_Z:
        taken = processor.af.zero;
        break;
    default:
        break;
    }
    if (taken) {
        processor.delay += 12;
        gb_bus_write(--processor.sp.word, processor.pc.high);
        gb_bus_write(--processor.sp.word, processor.pc.low);
        processor.pc.word = operand.word;
    }
}

static void gb_processor_instruction_ccf(void) {
    processor.delay = 4;
    processor.af.carry = !processor.af.carry;
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_cp(void) {
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_CP_A:
        operand.low = processor.af.high;
        break;
    case GB_INSTRUCTION_CP_B:
        operand.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_CP_C:
        operand.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_CP_D:
        operand.low = processor.de.high;
        break;
    case GB_INSTRUCTION_CP_E:
        operand.low = processor.de.low;
        break;
    case GB_INSTRUCTION_CP_H:
        operand.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_CP_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_CP_L:
        operand.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_CP_N:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    processor.af.carry = (processor.af.high < operand.low);
    processor.af.half_carry = ((processor.af.high & 0x0F) < ((processor.af.high - operand.low) & 0x0F));
    processor.af.negative = true;
    processor.af.zero = (processor.af.high == operand.low);
}

static void gb_processor_instruction_cpl(void) {
    processor.delay = 4;
    processor.af.high = ~processor.af.high;
    processor.af.half_carry = true;
    processor.af.negative = true;
}

static void gb_processor_instruction_daa(void) {
    processor.delay = 4;
    if (!processor.af.negative) {
        if (processor.af.carry || (processor.af.high > 0x99)) {
            processor.af.high += 0x60;
            processor.af.carry = true;
        }
        if (processor.af.half_carry || ((processor.af.high & 0x0F) > 0x09)) {
            processor.af.high += 0x06;
        }
    } else {
        if (processor.af.carry) {
            processor.af.high -= 0x60;
        }
        if (processor.af.half_carry) {
            processor.af.high -= 0x06;
        }
    }
    processor.af.half_carry = false;
    processor.af.zero = !processor.af.high;
}

static void gb_processor_instruction_dec(void) {
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_DEC_A:
        operand.low = --processor.af.high;
        break;
    case GB_INSTRUCTION_DEC_B:
        operand.low = --processor.bc.high;
        break;
    case GB_INSTRUCTION_DEC_C:
        operand.low = --processor.bc.low;
        break;
    case GB_INSTRUCTION_DEC_D:
        operand.low = --processor.de.high;
        break;
    case GB_INSTRUCTION_DEC_E:
        operand.low = --processor.de.low;
        break;
    case GB_INSTRUCTION_DEC_H:
        operand.low = --processor.hl.high;
        break;
    case GB_INSTRUCTION_DEC_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word) - 1;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_DEC_L:
        operand.low = --processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = ((operand.low & 0x0F) == 0x0F);
    processor.af.negative = true;
    processor.af.zero = !operand.low;
}

static void gb_processor_instruction_dec_word(void) {
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_DEC_BC:
        --processor.bc.word;
        break;
    case GB_INSTRUCTION_DEC_DE:
        --processor.de.word;
        break;
    case GB_INSTRUCTION_DEC_HL:
        --processor.hl.word;
        break;
    case GB_INSTRUCTION_DEC_SP:
        --processor.sp.word;
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_di(void) {
    processor.delay = 4;
    processor.interrupt.delay = 0;
    processor.interrupt.enabled = false;
}

static void gb_processor_instruction_ei(void) {
    processor.delay = 4;
    if (!processor.interrupt.delay) {
        processor.interrupt.delay = 2;
    }
}

static void gb_processor_instruction_halt(void) {
    processor.delay = 4;
    processor.halt_bug = (!processor.interrupt.enabled && (processor.interrupt.enable.raw & processor.interrupt.flag.raw & 0x1F));
    processor.halted = true;
}

static void gb_processor_instruction_inc(void) {
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_INC_A:
        operand.low = ++processor.af.high;
        break;
    case GB_INSTRUCTION_INC_B:
        operand.low = ++processor.bc.high;
        break;
    case GB_INSTRUCTION_INC_C:
        operand.low = ++processor.bc.low;
        break;
    case GB_INSTRUCTION_INC_D:
        operand.low = ++processor.de.high;
        break;
    case GB_INSTRUCTION_INC_E:
        operand.low = ++processor.de.low;
        break;
    case GB_INSTRUCTION_INC_H:
        operand.low = ++processor.hl.high;
        break;
    case GB_INSTRUCTION_INC_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word) + 1;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_INC_L:
        operand.low = ++processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = !(operand.low & 0x0F);
    processor.af.negative = false;
    processor.af.zero = !operand.low;
}

static void gb_processor_instruction_inc_word(void) {
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_INC_BC:
        ++processor.bc.word;
        break;
    case GB_INSTRUCTION_INC_DE:
        ++processor.de.word;
        break;
    case GB_INSTRUCTION_INC_HL:
        ++processor.hl.word;
        break;
    case GB_INSTRUCTION_INC_SP:
        ++processor.sp.word;
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_jp(void) {
    bool taken = false;
    gb_register_t operand = { .low = gb_bus_read(processor.pc.word++), .high = gb_bus_read(processor.pc.word++) };
    processor.delay = 12;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_JP:
        taken = true;
        break;
    case GB_INSTRUCTION_JP_C:
        taken = processor.af.carry;
        break;
    case GB_INSTRUCTION_JP_NC:
        taken = !processor.af.carry;
        break;
    case GB_INSTRUCTION_JP_NZ:
        taken = !processor.af.zero;
        break;
    case GB_INSTRUCTION_JP_Z:
        taken = processor.af.zero;
        break;
    default:
        break;
    }
    if (taken) {
        processor.delay += 4;
        processor.pc.word = operand.word;
    }
}

static void gb_processor_instruction_jp_hl(void) {
    processor.delay = 4;
    processor.pc.word = processor.hl.word;
}

static void gb_processor_instruction_jr(void) {
    bool taken = false;
    gb_register_t operand = { .low = gb_bus_read(processor.pc.word++) };
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_JR:
        taken = true;
        break;
    case GB_INSTRUCTION_JR_C:
        taken = processor.af.carry;
        break;
    case GB_INSTRUCTION_JR_NC:
        taken = !processor.af.carry;
        break;
    case GB_INSTRUCTION_JR_NZ:
        taken = !processor.af.zero;
        break;
    case GB_INSTRUCTION_JR_Z:
        taken = processor.af.zero;
        break;
    default:
        break;
    }
    if (taken) {
        processor.delay += 4;
        processor.pc.word += (int8_t)operand.low;
    }
}

static void gb_processor_instruction_ld(void) {
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_LD_A_A:
        break;
    case GB_INSTRUCTION_LD_A_B:
        processor.af.high = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_A_BCI:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.bc.word);
        break;
    case GB_INSTRUCTION_LD_A_C:
        processor.af.high = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_A_D:
        processor.af.high = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_A_DEI:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.de.word);
        break;
    case GB_INSTRUCTION_LD_A_E:
        processor.af.high = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_A_FF00_CI:
        processor.delay += 4;
        processor.af.high = gb_bus_read(0xFF00 + processor.bc.low);
        break;
    case GB_INSTRUCTION_LD_A_FF00_NI:
        processor.delay += 8;
        processor.af.high = gb_bus_read(0xFF00 + gb_bus_read(processor.pc.word++));
        break;
    case GB_INSTRUCTION_LD_A_H:
        processor.af.high = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_A_HLI:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_A_HLID:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.hl.word--);
        break;
    case GB_INSTRUCTION_LD_A_HLII:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.hl.word++);
        break;
    case GB_INSTRUCTION_LD_A_L:
        processor.af.high = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_A_N:
        processor.delay += 4;
        processor.af.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_A_NNI:
        processor.delay += 12;
        operand.low = gb_bus_read(processor.pc.word++);
        operand.high = gb_bus_read(processor.pc.word++);
        processor.af.high = gb_bus_read(operand.word);
        break;
    case GB_INSTRUCTION_LD_B_A:
        processor.bc.high = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_B_B:
        break;
    case GB_INSTRUCTION_LD_B_C:
        processor.bc.high = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_B_D:
        processor.bc.high = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_B_E:
        processor.bc.high = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_B_H:
        processor.bc.high = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_B_HLI:
        processor.delay += 4;
        processor.bc.high = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_B_L:
        processor.bc.high = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_B_N:
        processor.delay += 4;
        processor.bc.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_BC_NN:
        processor.delay += 8;
        processor.bc.low = gb_bus_read(processor.pc.word++);
        processor.bc.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_BCI_A:
        processor.delay += 4;
        gb_bus_write(processor.bc.word, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_C_A:
        processor.bc.low = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_C_B:
        processor.bc.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_C_C:
        break;
    case GB_INSTRUCTION_LD_C_D:
        processor.bc.low = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_C_E:
        processor.bc.low = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_C_H:
        processor.bc.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_C_HLI:
        processor.delay += 4;
        processor.bc.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_C_L:
        processor.bc.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_C_N:
        processor.delay += 4;
        processor.bc.low = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_DE_NN:
        processor.delay += 8;
        processor.de.low = gb_bus_read(processor.pc.word++);
        processor.de.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_DEI_A:
        processor.delay += 4;
        gb_bus_write(processor.de.word, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_D_A:
        processor.de.high = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_D_B:
        processor.de.high = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_D_C:
        processor.de.high = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_D_D:
        break;
    case GB_INSTRUCTION_LD_D_E:
        processor.de.high = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_D_H:
        processor.de.high = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_D_HLI:
        processor.delay += 4;
        processor.de.high = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_D_L:
        processor.de.high = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_D_N:
        processor.delay += 4;
        processor.de.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_E_A:
        processor.de.low = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_E_B:
        processor.de.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_E_C:
        processor.de.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_E_D:
        processor.de.low = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_E_E:
        break;
    case GB_INSTRUCTION_LD_E_H:
        processor.de.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_E_HLI:
        processor.delay += 4;
        processor.de.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_E_L:
        processor.de.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_E_N:
        processor.delay += 4;
        processor.de.low = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_FF00_CI_A:
        processor.delay += 4;
        gb_bus_write(0xFF00 + processor.bc.low, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_FF00_NI_A:
        processor.delay += 8;
        gb_bus_write(0xFF00 + gb_bus_read(processor.pc.word++), processor.af.high);
        break;
    case GB_INSTRUCTION_LD_H_A:
        processor.hl.high = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_H_B:
        processor.hl.high = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_H_C:
        processor.hl.high = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_H_D:
        processor.hl.high = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_H_E:
        processor.hl.high = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_H_H:
        break;
    case GB_INSTRUCTION_LD_H_HLI:
        processor.delay += 4;
        processor.hl.high = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_H_L:
        processor.hl.high = processor.hl.low;
        break;
    case GB_INSTRUCTION_LD_H_N:
        processor.delay += 4;
        processor.hl.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_HL_NN:
        processor.delay += 8;
        processor.hl.low = gb_bus_read(processor.pc.word++);
        processor.hl.high = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_HLI_A:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_HLI_B:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.bc.high);
        break;
    case GB_INSTRUCTION_LD_HLI_C:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.bc.low);
        break;
    case GB_INSTRUCTION_LD_HLI_D:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.de.high);
        break;
    case GB_INSTRUCTION_LD_HLI_E:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.de.low);
        break;
    case GB_INSTRUCTION_LD_HLI_H:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.hl.high);
        break;
    case GB_INSTRUCTION_LD_HLI_L:
        processor.delay += 4;
        gb_bus_write(processor.hl.word, processor.hl.low);
        break;
    case GB_INSTRUCTION_LD_HLI_N:
        processor.delay += 8;
        gb_bus_write(processor.hl.word, gb_bus_read(processor.pc.word++));
        break;
    case GB_INSTRUCTION_LD_HLID_A:
        processor.delay += 4;
        gb_bus_write(processor.hl.word--, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_HLII_A:
        processor.delay += 4;
        gb_bus_write(processor.hl.word++, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_L_A:
        processor.hl.low = processor.af.high;
        break;
    case GB_INSTRUCTION_LD_L_B:
        processor.hl.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_LD_L_C:
        processor.hl.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_LD_L_D:
        processor.hl.low = processor.de.high;
        break;
    case GB_INSTRUCTION_LD_L_E:
        processor.hl.low = processor.de.low;
        break;
    case GB_INSTRUCTION_LD_L_H:
        processor.hl.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_LD_L_HLI:
        processor.delay += 4;
        processor.hl.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_LD_L_L:
        break;
    case GB_INSTRUCTION_LD_L_N:
        processor.delay += 4;
        processor.hl.low = gb_bus_read(processor.pc.word++);
        break;
    case GB_INSTRUCTION_LD_NNI_A:
        processor.delay += 12;
        operand.low = gb_bus_read(processor.pc.word++);
        operand.high = gb_bus_read(processor.pc.word++);
        gb_bus_write(operand.word, processor.af.high);
        break;
    case GB_INSTRUCTION_LD_NNI_SP:
        processor.delay += 16;
        operand.low = gb_bus_read(processor.pc.word++);
        operand.high = gb_bus_read(processor.pc.word++);
        gb_bus_write(operand.word, processor.sp.low);
        gb_bus_write(operand.word + 1, processor.sp.high);
        break;
    case GB_INSTRUCTION_LD_SP_HL:
        processor.delay += 4;
        processor.sp.word = processor.hl.word;
        break;
    case GB_INSTRUCTION_LD_SP_NN:
        processor.delay += 8;
        processor.sp.low = gb_bus_read(processor.pc.word++);
        processor.sp.high = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_ld_hl(void) {
    uint32_t carry = 0, sum = 0;
    gb_register_t operand = { .low = gb_bus_read(processor.pc.word++) };
    processor.delay = 12;
    sum = processor.sp.word + (int8_t)operand.low;
    carry = processor.sp.word ^ (int8_t)operand.low ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = false;
    processor.af.zero = false;
    processor.hl.word = sum;
}

static void gb_processor_instruction_nop(void) {
    processor.delay = 4;
}

static void gb_processor_instruction_or(void) {
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_OR_A:
        break;
    case GB_INSTRUCTION_OR_B:
        processor.af.high |= processor.bc.high;
        break;
    case GB_INSTRUCTION_OR_C:
        processor.af.high |= processor.bc.low;
        break;
    case GB_INSTRUCTION_OR_D:
        processor.af.high |= processor.de.high;
        break;
    case GB_INSTRUCTION_OR_E:
        processor.af.high |= processor.de.low;
        break;
    case GB_INSTRUCTION_OR_H:
        processor.af.high |= processor.hl.high;
        break;
    case GB_INSTRUCTION_OR_HLI:
        processor.delay += 4;
        processor.af.high |= gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_OR_L:
        processor.af.high |= processor.hl.low;
        break;
    case GB_INSTRUCTION_OR_N:
        processor.delay += 4;
        processor.af.high |= gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    processor.af.carry = false;
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = !processor.af.high;
}

static void gb_processor_instruction_pop(void) {
    processor.delay = 12;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_POP_AF:
        processor.af.low = gb_bus_read(processor.sp.word++) & 0xF0;
        processor.af.high = gb_bus_read(processor.sp.word++);
        break;
    case GB_INSTRUCTION_POP_BC:
        processor.bc.low = gb_bus_read(processor.sp.word++);
        processor.bc.high = gb_bus_read(processor.sp.word++);
        break;
    case GB_INSTRUCTION_POP_DE:
        processor.de.low = gb_bus_read(processor.sp.word++);
        processor.de.high = gb_bus_read(processor.sp.word++);
        break;
    case GB_INSTRUCTION_POP_HL:
        processor.hl.low = gb_bus_read(processor.sp.word++);
        processor.hl.high = gb_bus_read(processor.sp.word++);
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_push(void) {
    processor.delay = 16;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_PUSH_AF:
        gb_bus_write(--processor.sp.word, processor.af.high);
        gb_bus_write(--processor.sp.word, processor.af.low);
        break;
    case GB_INSTRUCTION_PUSH_BC:
        gb_bus_write(--processor.sp.word, processor.bc.high);
        gb_bus_write(--processor.sp.word, processor.bc.low);
        break;
    case GB_INSTRUCTION_PUSH_DE:
        gb_bus_write(--processor.sp.word, processor.de.high);
        gb_bus_write(--processor.sp.word, processor.de.low);
        break;
    case GB_INSTRUCTION_PUSH_HL:
        gb_bus_write(--processor.sp.word, processor.hl.high);
        gb_bus_write(--processor.sp.word, processor.hl.low);
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_res(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RES_0_A:
    case GB_INSTRUCTION_RES_1_A:
    case GB_INSTRUCTION_RES_2_A:
    case GB_INSTRUCTION_RES_3_A:
    case GB_INSTRUCTION_RES_4_A:
    case GB_INSTRUCTION_RES_5_A:
    case GB_INSTRUCTION_RES_6_A:
    case GB_INSTRUCTION_RES_7_A:
        processor.af.high &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_A) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_B:
    case GB_INSTRUCTION_RES_1_B:
    case GB_INSTRUCTION_RES_2_B:
    case GB_INSTRUCTION_RES_3_B:
    case GB_INSTRUCTION_RES_4_B:
    case GB_INSTRUCTION_RES_5_B:
    case GB_INSTRUCTION_RES_6_B:
    case GB_INSTRUCTION_RES_7_B:
        processor.bc.high &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_B) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_C:
    case GB_INSTRUCTION_RES_1_C:
    case GB_INSTRUCTION_RES_2_C:
    case GB_INSTRUCTION_RES_3_C:
    case GB_INSTRUCTION_RES_4_C:
    case GB_INSTRUCTION_RES_5_C:
    case GB_INSTRUCTION_RES_6_C:
    case GB_INSTRUCTION_RES_7_C:
        processor.bc.low &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_C) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_D:
    case GB_INSTRUCTION_RES_1_D:
    case GB_INSTRUCTION_RES_2_D:
    case GB_INSTRUCTION_RES_3_D:
    case GB_INSTRUCTION_RES_4_D:
    case GB_INSTRUCTION_RES_5_D:
    case GB_INSTRUCTION_RES_6_D:
    case GB_INSTRUCTION_RES_7_D:
        processor.de.high &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_D) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_E:
    case GB_INSTRUCTION_RES_1_E:
    case GB_INSTRUCTION_RES_2_E:
    case GB_INSTRUCTION_RES_3_E:
    case GB_INSTRUCTION_RES_4_E:
    case GB_INSTRUCTION_RES_5_E:
    case GB_INSTRUCTION_RES_6_E:
    case GB_INSTRUCTION_RES_7_E:
        processor.de.low &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_E) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_H:
    case GB_INSTRUCTION_RES_1_H:
    case GB_INSTRUCTION_RES_2_H:
    case GB_INSTRUCTION_RES_3_H:
    case GB_INSTRUCTION_RES_4_H:
    case GB_INSTRUCTION_RES_5_H:
    case GB_INSTRUCTION_RES_6_H:
    case GB_INSTRUCTION_RES_7_H:
        processor.hl.high &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_H) >> 3));
        break;
    case GB_INSTRUCTION_RES_0_HLI:
    case GB_INSTRUCTION_RES_1_HLI:
    case GB_INSTRUCTION_RES_2_HLI:
    case GB_INSTRUCTION_RES_3_HLI:
    case GB_INSTRUCTION_RES_4_HLI:
    case GB_INSTRUCTION_RES_5_HLI:
    case GB_INSTRUCTION_RES_6_HLI:
    case GB_INSTRUCTION_RES_7_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        operand.low &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_HLI) >> 3));
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_RES_0_L:
    case GB_INSTRUCTION_RES_1_L:
    case GB_INSTRUCTION_RES_2_L:
    case GB_INSTRUCTION_RES_3_L:
    case GB_INSTRUCTION_RES_4_L:
    case GB_INSTRUCTION_RES_5_L:
    case GB_INSTRUCTION_RES_6_L:
    case GB_INSTRUCTION_RES_7_L:
        processor.hl.low &= ~(1 << ((processor.instruction.opcode - GB_INSTRUCTION_RES_0_L) >> 3));
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_ret(void) {
    bool taken = false;
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RET:
        taken = true;
        break;
    case GB_INSTRUCTION_RET_C:
        taken = processor.af.carry;
        break;
    case GB_INSTRUCTION_RET_NC:
        taken = !processor.af.carry;
        break;
    case GB_INSTRUCTION_RET_NZ:
        taken = !processor.af.zero;
        break;
    case GB_INSTRUCTION_RET_Z:
        taken = processor.af.zero;
        break;
    default:
        break;
    }
    if (taken) {
        processor.delay += (processor.instruction.opcode == GB_INSTRUCTION_RET) ? 8 : 12;
        processor.pc.low = gb_bus_read(processor.sp.word++);
        processor.pc.high = gb_bus_read(processor.sp.word++);
    }
}

static void gb_processor_instruction_reti(void) {
    processor.delay = 16;
    processor.pc.low = gb_bus_read(processor.sp.word++);
    processor.pc.high = gb_bus_read(processor.sp.word++);
    processor.interrupt.delay = 0;
    processor.interrupt.enabled = true;
}

static void gb_processor_instruction_rl(void) {
    uint8_t carry = processor.af.carry;
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RL_A:
        processor.af.carry = ((processor.af.high & 0x80) == 0x80);
        processor.af.high = (processor.af.high << 1) | carry;
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_RL_B:
        processor.af.carry = ((processor.bc.high & 0x80) == 0x80);
        processor.bc.high = (processor.bc.high << 1) | carry;
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_RL_C:
        processor.af.carry = ((processor.bc.low & 0x80) == 0x80);
        processor.bc.low = (processor.bc.low << 1) | carry;
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_RL_D:
        processor.af.carry = ((processor.de.high & 0x80) == 0x80);
        processor.de.high = (processor.de.high << 1) | carry;
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_RL_E:
        processor.af.carry = ((processor.de.low & 0x80) == 0x80);
        processor.de.low = (processor.de.low << 1) | carry;
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_RL_H:
        processor.af.carry = ((processor.hl.high & 0x80) == 0x80);
        processor.hl.high = (processor.hl.high << 1) | carry;
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_RL_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 0x80) == 0x80);
        operand.low = (operand.low << 1) | carry;
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_RL_L:
        processor.af.carry = ((processor.hl.low & 0x80) == 0x80);
        processor.hl.low = (processor.hl.low << 1) | carry;
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_rla(void) {
    uint8_t carry = processor.af.carry;
    processor.delay = 4;
    processor.af.carry = ((processor.af.high & 0x80) == 0x80);
    processor.af.high = (processor.af.high << 1) | carry;
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = false;
}

static void gb_processor_instruction_rlc(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RLC_A:
        processor.af.carry = ((processor.af.high & 0x80) == 0x80);
        processor.af.high = (processor.af.high << 1) | processor.af.carry;
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_RLC_B:
        processor.af.carry = ((processor.bc.high & 0x80) == 0x80);
        processor.bc.high = (processor.bc.high << 1) | processor.af.carry;
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_RLC_C:
        processor.af.carry = ((processor.bc.low & 0x80) == 0x80);
        processor.bc.low = (processor.bc.low << 1) | processor.af.carry;
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_RLC_D:
        processor.af.carry = ((processor.de.high & 0x80) == 0x80);
        processor.de.high = (processor.de.high << 1) | processor.af.carry;
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_RLC_E:
        processor.af.carry = ((processor.de.low & 0x80) == 0x80);
        processor.de.low = (processor.de.low << 1) | processor.af.carry;
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_RLC_H:
        processor.af.carry = ((processor.hl.high & 0x80) == 0x80);
        processor.hl.high = (processor.hl.high << 1) | processor.af.carry;
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_RLC_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 0x80) == 0x80);
        operand.low = (operand.low << 1) | processor.af.carry;
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_RLC_L:
        processor.af.carry = ((processor.hl.low & 0x80) == 0x80);
        processor.hl.low = (processor.hl.low << 1) | processor.af.carry;
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_rlca(void) {
    processor.delay = 4;
    processor.af.carry = ((processor.af.high & 0x80) == 0x80);
    processor.af.high = (processor.af.high << 1) | processor.af.carry;
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = false;
}

static void gb_processor_instruction_rr(void) {
    uint8_t carry = processor.af.carry;
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RR_A:
        processor.af.carry = ((processor.af.high & 1) == 1);
        processor.af.high = (processor.af.high >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_RR_B:
        processor.af.carry = ((processor.bc.high & 1) == 1);
        processor.bc.high = (processor.bc.high >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_RR_C:
        processor.af.carry = ((processor.bc.low & 1) == 1);
        processor.bc.low = (processor.bc.low >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_RR_D:
        processor.af.carry = ((processor.de.high & 1) == 1);
        processor.de.high = (processor.de.high >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_RR_E:
        processor.af.carry = ((processor.de.low & 1) == 1);
        processor.de.low = (processor.de.low >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_RR_H:
        processor.af.carry = ((processor.hl.high & 1) == 1);
        processor.hl.high = (processor.hl.high >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_RR_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 1) == 1);
        operand.low = (operand.low >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_RR_L:
        processor.af.carry = ((processor.hl.low & 1) == 1);
        processor.hl.low = (processor.hl.low >> 1) | (carry ? 0x80 : 0);
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_rra(void) {
    uint8_t carry = processor.af.carry;
    processor.delay = 4;
    processor.af.carry = ((processor.af.high & 1) == 1);
    processor.af.high = (processor.af.high >> 1) | (carry ? 0x80 : 0);
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = false;
}

static void gb_processor_instruction_rrc(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_RRC_A:
        processor.af.carry = ((processor.af.high & 1) == 1);
        processor.af.high = (processor.af.high >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_RRC_B:
        processor.af.carry = ((processor.bc.high & 1) == 1);
        processor.bc.high = (processor.bc.high >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_RRC_C:
        processor.af.carry = ((processor.bc.low & 1) == 1);
        processor.bc.low = (processor.bc.low >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_RRC_D:
        processor.af.carry = ((processor.de.high & 1) == 1);
        processor.de.high = (processor.de.high >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_RRC_E:
        processor.af.carry = ((processor.de.low & 1) == 1);
        processor.de.low = (processor.de.low >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_RRC_H:
        processor.af.carry = ((processor.hl.high & 1) == 1);
        processor.hl.high = (processor.hl.high >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_RRC_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 1) == 1);
        operand.low = (operand.low >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_RRC_L:
        processor.af.carry = ((processor.hl.low & 1) == 1);
        processor.hl.low = (processor.hl.low >> 1) | (processor.af.carry ? 0x80 : 0);
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_rrca(void) {
    processor.delay = 4;
    processor.af.carry = ((processor.af.high & 1) == 1);
    processor.af.high = (processor.af.high >> 1) | (processor.af.carry ? 0x80 : 0);
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = false;
}

static void gb_processor_instruction_rst(void) {
    processor.delay = 16;
    gb_bus_write(--processor.sp.word, processor.pc.high);
    gb_bus_write(--processor.sp.word, processor.pc.low);
    processor.pc.word = processor.instruction.opcode - GB_INSTRUCTION_RST_00;
}

static void gb_processor_instruction_sbc(void) {
    uint16_t carry = 0, sum = 0;
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SBC_A:
        operand.low = processor.af.high;
        break;
    case GB_INSTRUCTION_SBC_B:
        operand.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_SBC_C:
        operand.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_SBC_D:
        operand.low = processor.de.high;
        break;
    case GB_INSTRUCTION_SBC_E:
        operand.low = processor.de.low;
        break;
    case GB_INSTRUCTION_SBC_H:
        operand.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_SBC_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_SBC_L:
        operand.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_SBC_N:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    sum = processor.af.high - operand.low - processor.af.carry;
    carry = processor.af.high ^ operand.low ^ processor.af.carry ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = true;
    processor.af.zero = !(sum & 0xFF);
    processor.af.high = sum;
}

static void gb_processor_instruction_scf(void) {
    processor.delay = 4;
    processor.af.carry = true;
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_set(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SET_0_A:
    case GB_INSTRUCTION_SET_1_A:
    case GB_INSTRUCTION_SET_2_A:
    case GB_INSTRUCTION_SET_3_A:
    case GB_INSTRUCTION_SET_4_A:
    case GB_INSTRUCTION_SET_5_A:
    case GB_INSTRUCTION_SET_6_A:
    case GB_INSTRUCTION_SET_7_A:
        processor.af.high |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_A) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_B:
    case GB_INSTRUCTION_SET_1_B:
    case GB_INSTRUCTION_SET_2_B:
    case GB_INSTRUCTION_SET_3_B:
    case GB_INSTRUCTION_SET_4_B:
    case GB_INSTRUCTION_SET_5_B:
    case GB_INSTRUCTION_SET_6_B:
    case GB_INSTRUCTION_SET_7_B:
        processor.bc.high |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_B) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_C:
    case GB_INSTRUCTION_SET_1_C:
    case GB_INSTRUCTION_SET_2_C:
    case GB_INSTRUCTION_SET_3_C:
    case GB_INSTRUCTION_SET_4_C:
    case GB_INSTRUCTION_SET_5_C:
    case GB_INSTRUCTION_SET_6_C:
    case GB_INSTRUCTION_SET_7_C:
        processor.bc.low |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_C) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_D:
    case GB_INSTRUCTION_SET_1_D:
    case GB_INSTRUCTION_SET_2_D:
    case GB_INSTRUCTION_SET_3_D:
    case GB_INSTRUCTION_SET_4_D:
    case GB_INSTRUCTION_SET_5_D:
    case GB_INSTRUCTION_SET_6_D:
    case GB_INSTRUCTION_SET_7_D:
        processor.de.high |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_D) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_E:
    case GB_INSTRUCTION_SET_1_E:
    case GB_INSTRUCTION_SET_2_E:
    case GB_INSTRUCTION_SET_3_E:
    case GB_INSTRUCTION_SET_4_E:
    case GB_INSTRUCTION_SET_5_E:
    case GB_INSTRUCTION_SET_6_E:
    case GB_INSTRUCTION_SET_7_E:
        processor.de.low |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_E) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_H:
    case GB_INSTRUCTION_SET_1_H:
    case GB_INSTRUCTION_SET_2_H:
    case GB_INSTRUCTION_SET_3_H:
    case GB_INSTRUCTION_SET_4_H:
    case GB_INSTRUCTION_SET_5_H:
    case GB_INSTRUCTION_SET_6_H:
    case GB_INSTRUCTION_SET_7_H:
        processor.hl.high |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_H) >> 3);
        break;
    case GB_INSTRUCTION_SET_0_HLI:
    case GB_INSTRUCTION_SET_1_HLI:
    case GB_INSTRUCTION_SET_2_HLI:
    case GB_INSTRUCTION_SET_3_HLI:
    case GB_INSTRUCTION_SET_4_HLI:
    case GB_INSTRUCTION_SET_5_HLI:
    case GB_INSTRUCTION_SET_6_HLI:
    case GB_INSTRUCTION_SET_7_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        operand.low |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_HLI) >> 3);
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_SET_0_L:
    case GB_INSTRUCTION_SET_1_L:
    case GB_INSTRUCTION_SET_2_L:
    case GB_INSTRUCTION_SET_3_L:
    case GB_INSTRUCTION_SET_4_L:
    case GB_INSTRUCTION_SET_5_L:
    case GB_INSTRUCTION_SET_6_L:
    case GB_INSTRUCTION_SET_7_L:
        processor.hl.low |= 1 << ((processor.instruction.opcode - GB_INSTRUCTION_SET_0_L) >> 3);
        break;
    default:
        break;
    }
}

static void gb_processor_instruction_sla(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SLA_A:
        processor.af.carry = ((processor.af.high & 0x80) == 0x80);
        processor.af.high <<= 1;
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_SLA_B:
        processor.af.carry = ((processor.bc.high & 0x80) == 0x80);
        processor.bc.high <<= 1;
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_SLA_C:
        processor.af.carry = ((processor.bc.low & 0x80) == 0x80);
        processor.bc.low <<= 1;
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_SLA_D:
        processor.af.carry = ((processor.de.high & 0x80) == 0x80);
        processor.de.high <<= 1;
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_SLA_E:
        processor.af.carry = ((processor.de.low & 0x80) == 0x80);
        processor.de.low <<= 1;
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_SLA_H:
        processor.af.carry = ((processor.hl.high & 0x80) == 0x80);
        processor.hl.high <<= 1;
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_SLA_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 0x80) == 0x80);
        operand.low <<= 1;
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_SLA_L:
        processor.af.carry = ((processor.hl.low & 0x80) == 0x80);
        processor.hl.low <<= 1;
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_sra(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SRA_A:
        processor.af.carry = ((processor.af.high & 1) == 1);
        processor.af.high = (processor.af.high >> 1) | (processor.af.high & 0x80);
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_SRA_B:
        processor.af.carry = ((processor.bc.high & 1) == 1);
        processor.bc.high = (processor.bc.high >> 1) | (processor.bc.high & 0x80);
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_SRA_C:
        processor.af.carry = ((processor.bc.low & 1) == 1);
        processor.bc.low = (processor.bc.low >> 1) | (processor.bc.low & 0x80);
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_SRA_D:
        processor.af.carry = ((processor.de.high & 1) == 1);
        processor.de.high = (processor.de.high >> 1) | (processor.de.high & 0x80);
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_SRA_E:
        processor.af.carry = ((processor.de.low & 1) == 1);
        processor.de.low = (processor.de.low >> 1) | (processor.de.low & 0x80);
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_SRA_H:
        processor.af.carry = ((processor.hl.high & 1) == 1);
        processor.hl.high = (processor.hl.high >> 1) | (processor.hl.high & 0x80);
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_SRA_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 1) == 1);
        operand.low = (operand.low >> 1) | (operand.low & 0x80);
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_SRA_L:
        processor.af.carry = ((processor.hl.low & 1) == 1);
        processor.hl.low = (processor.hl.low >> 1) | (processor.hl.low & 0x80);
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_srl(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SRL_A:
        processor.af.carry = ((processor.af.high & 1) == 1);
        processor.af.high >>= 1;
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_SRL_B:
        processor.af.carry = ((processor.bc.high & 1) == 1);
        processor.bc.high >>= 1;
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_SRL_C:
        processor.af.carry = ((processor.bc.low & 1) == 1);
        processor.bc.low >>= 1;
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_SRL_D:
        processor.af.carry = ((processor.de.high & 1) == 1);
        processor.de.high >>= 1;
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_SRL_E:
        processor.af.carry = ((processor.de.low & 1) == 1);
        processor.de.low >>= 1;
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_SRL_H:
        processor.af.carry = ((processor.hl.high & 1) == 1);
        processor.hl.high >>= 1;
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_SRL_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        processor.af.carry = ((operand.low & 1) == 1);
        operand.low >>= 1;
        processor.af.zero = !operand.low;
        gb_bus_write(processor.hl.word, operand.low);
        break;
    case GB_INSTRUCTION_SRL_L:
        processor.af.carry = ((processor.hl.low & 1) == 1);
        processor.hl.low >>= 1;
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_stop(void) {
    processor.delay = 4;
    gb_bus_read(processor.pc.word++);
    processor.stopped = true;
}

static void gb_processor_instruction_sub(void) {
    uint16_t carry = 0, sum = 0;
    gb_register_t operand = {};
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SUB_A:
        operand.low = processor.af.high;
        break;
    case GB_INSTRUCTION_SUB_B:
        operand.low = processor.bc.high;
        break;
    case GB_INSTRUCTION_SUB_C:
        operand.low = processor.bc.low;
        break;
    case GB_INSTRUCTION_SUB_D:
        operand.low = processor.de.high;
        break;
    case GB_INSTRUCTION_SUB_E:
        operand.low = processor.de.low;
        break;
    case GB_INSTRUCTION_SUB_H:
        operand.low = processor.hl.high;
        break;
    case GB_INSTRUCTION_SUB_HLI:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_SUB_L:
        operand.low = processor.hl.low;
        break;
    case GB_INSTRUCTION_SUB_N:
        processor.delay += 4;
        operand.low = gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    sum = processor.af.high - operand.low;
    carry = processor.af.high ^ operand.low ^ sum;
    processor.af.carry = ((carry & 0x100) == 0x100);
    processor.af.half_carry = ((carry & 0x10) == 0x10);
    processor.af.negative = true;
    processor.af.zero = !(sum & 0xFF);
    processor.af.high = sum;
}

static void gb_processor_instruction_swap(void) {
    gb_register_t operand = {};
    processor.delay = 8;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_SWAP_A:
        processor.af.high = (processor.af.high << 4) | (processor.af.high >> 4);
        processor.af.zero = !processor.af.high;
        break;
    case GB_INSTRUCTION_SWAP_B:
        processor.bc.high = (processor.bc.high << 4) | (processor.bc.high >> 4);
        processor.af.zero = !processor.bc.high;
        break;
    case GB_INSTRUCTION_SWAP_C:
        processor.bc.low = (processor.bc.low << 4) | (processor.bc.low >> 4);
        processor.af.zero = !processor.bc.low;
        break;
    case GB_INSTRUCTION_SWAP_D:
        processor.de.high = (processor.de.high << 4) | (processor.de.high >> 4);
        processor.af.zero = !processor.de.high;
        break;
    case GB_INSTRUCTION_SWAP_E:
        processor.de.low = (processor.de.low << 4) | (processor.de.low >> 4);
        processor.af.zero = !processor.de.low;
        break;
    case GB_INSTRUCTION_SWAP_H:
        processor.hl.high = (processor.hl.high << 4) | (processor.hl.high >> 4);
        processor.af.zero = !processor.hl.high;
        break;
    case GB_INSTRUCTION_SWAP_HLI:
        processor.delay += 8;
        operand.low = gb_bus_read(processor.hl.word);
        operand.low = (operand.low << 4) | (operand.low >> 4);
        gb_bus_write(processor.hl.word, operand.low);
        processor.af.zero = !operand.low;
        break;
    case GB_INSTRUCTION_SWAP_L:
        processor.hl.low = (processor.hl.low << 4) | (processor.hl.low >> 4);
        processor.af.zero = !processor.hl.low;
        break;
    default:
        break;
    }
    processor.af.carry = false;
    processor.af.half_carry = false;
    processor.af.negative = false;
}

static void gb_processor_instruction_xor(void) {
    processor.delay = 4;
    switch (processor.instruction.opcode) {
    case GB_INSTRUCTION_XOR_A:
        processor.af.high = 0;
        break;
    case GB_INSTRUCTION_XOR_B:
        processor.af.high ^= processor.bc.high;
        break;
    case GB_INSTRUCTION_XOR_C:
        processor.af.high ^= processor.bc.low;
        break;
    case GB_INSTRUCTION_XOR_D:
        processor.af.high ^= processor.de.high;
        break;
    case GB_INSTRUCTION_XOR_E:
        processor.af.high ^= processor.de.low;
        break;
    case GB_INSTRUCTION_XOR_H:
        processor.af.high ^= processor.hl.high;
        break;
    case GB_INSTRUCTION_XOR_HLI:
        processor.delay += 4;
        processor.af.high ^= gb_bus_read(processor.hl.word);
        break;
    case GB_INSTRUCTION_XOR_L:
        processor.af.high ^= processor.hl.low;
        break;
    case GB_INSTRUCTION_XOR_N:
        processor.delay += 4;
        processor.af.high ^= gb_bus_read(processor.pc.word++);
        break;
    default:
        break;
    }
    processor.af.carry = false;
    processor.af.half_carry = false;
    processor.af.negative = false;
    processor.af.zero = !processor.af.high;
}

static void gb_processor_instruction_xxx(void) {
    processor.delay = 4;
    processor.status = GB_ERROR("Invalid instruction: [%04X] %02X", processor.instruction.address, processor.instruction.opcode);
}

static void gb_processor_execute(void) {
    processor.instruction.address = processor.pc.word;
    processor.instruction.opcode = static_cast<gb_instruction_e>(gb_bus_read(processor.pc.word++));
    if (processor.halt_bug) {
        processor.halt_bug = false;
        --processor.pc.word;
    }
    if (processor.instruction.opcode != GB_INSTRUCTION_PREFIX) {
        switch (processor.instruction.opcode) {
        case GB_INSTRUCTION_ADC_A:
        case GB_INSTRUCTION_ADC_B:
        case GB_INSTRUCTION_ADC_C:
        case GB_INSTRUCTION_ADC_D:
        case GB_INSTRUCTION_ADC_E:
        case GB_INSTRUCTION_ADC_H:
        case GB_INSTRUCTION_ADC_HLI:
        case GB_INSTRUCTION_ADC_L:
        case GB_INSTRUCTION_ADC_N:
            gb_processor_instruction_adc();
            break;
        case GB_INSTRUCTION_ADD_A:
        case GB_INSTRUCTION_ADD_B:
        case GB_INSTRUCTION_ADD_C:
        case GB_INSTRUCTION_ADD_D:
        case GB_INSTRUCTION_ADD_E:
        case GB_INSTRUCTION_ADD_H:
        case GB_INSTRUCTION_ADD_HLI:
        case GB_INSTRUCTION_ADD_L:
        case GB_INSTRUCTION_ADD_N:
            gb_processor_instruction_add();
            break;
        case GB_INSTRUCTION_ADD_HL_BC:
        case GB_INSTRUCTION_ADD_HL_DE:
        case GB_INSTRUCTION_ADD_HL_HL:
        case GB_INSTRUCTION_ADD_HL_SP:
            gb_processor_instruction_add_hl();
            break;
        case GB_INSTRUCTION_ADD_SP:
            gb_processor_instruction_add_sp();
            break;
        case GB_INSTRUCTION_AND_A:
        case GB_INSTRUCTION_AND_B:
        case GB_INSTRUCTION_AND_C:
        case GB_INSTRUCTION_AND_D:
        case GB_INSTRUCTION_AND_E:
        case GB_INSTRUCTION_AND_H:
        case GB_INSTRUCTION_AND_HLI:
        case GB_INSTRUCTION_AND_L:
        case GB_INSTRUCTION_AND_N:
            gb_processor_instruction_and();
            break;
        case GB_INSTRUCTION_CALL:
        case GB_INSTRUCTION_CALL_C:
        case GB_INSTRUCTION_CALL_NC:
        case GB_INSTRUCTION_CALL_NZ:
        case GB_INSTRUCTION_CALL_Z:
            gb_processor_instruction_call();
            break;
        case GB_INSTRUCTION_CCF:
            gb_processor_instruction_ccf();
            break;
        case GB_INSTRUCTION_CPL:
            gb_processor_instruction_cpl();
            break;
        case GB_INSTRUCTION_CP_A:
        case GB_INSTRUCTION_CP_B:
        case GB_INSTRUCTION_CP_C:
        case GB_INSTRUCTION_CP_D:
        case GB_INSTRUCTION_CP_E:
        case GB_INSTRUCTION_CP_H:
        case GB_INSTRUCTION_CP_HLI:
        case GB_INSTRUCTION_CP_L:
        case GB_INSTRUCTION_CP_N:
            gb_processor_instruction_cp();
            break;
        case GB_INSTRUCTION_DAA:
            gb_processor_instruction_daa();
            break;
        case GB_INSTRUCTION_DEC_A:
        case GB_INSTRUCTION_DEC_B:
        case GB_INSTRUCTION_DEC_C:
        case GB_INSTRUCTION_DEC_D:
        case GB_INSTRUCTION_DEC_E:
        case GB_INSTRUCTION_DEC_H:
        case GB_INSTRUCTION_DEC_HLI:
        case GB_INSTRUCTION_DEC_L:
            gb_processor_instruction_dec();
            break;
        case GB_INSTRUCTION_DEC_BC:
        case GB_INSTRUCTION_DEC_DE:
        case GB_INSTRUCTION_DEC_HL:
        case GB_INSTRUCTION_DEC_SP:
            gb_processor_instruction_dec_word();
            break;
        case GB_INSTRUCTION_DI:
            gb_processor_instruction_di();
            break;
        case GB_INSTRUCTION_EI:
            gb_processor_instruction_ei();
            break;
        case GB_INSTRUCTION_HALT:
            gb_processor_instruction_halt();
            break;
        case GB_INSTRUCTION_INC_A:
        case GB_INSTRUCTION_INC_B:
        case GB_INSTRUCTION_INC_C:
        case GB_INSTRUCTION_INC_D:
        case GB_INSTRUCTION_INC_E:
        case GB_INSTRUCTION_INC_H:
        case GB_INSTRUCTION_INC_HLI:
        case GB_INSTRUCTION_INC_L:
            gb_processor_instruction_inc();
            break;
        case GB_INSTRUCTION_INC_BC:
        case GB_INSTRUCTION_INC_DE:
        case GB_INSTRUCTION_INC_HL:
        case GB_INSTRUCTION_INC_SP:
            gb_processor_instruction_inc_word();
            break;
        case GB_INSTRUCTION_JP:
        case GB_INSTRUCTION_JP_C:
        case GB_INSTRUCTION_JP_NC:
        case GB_INSTRUCTION_JP_NZ:
        case GB_INSTRUCTION_JP_Z:
            gb_processor_instruction_jp();
            break;
        case GB_INSTRUCTION_JP_HL:
            gb_processor_instruction_jp_hl();
            break;
        case GB_INSTRUCTION_JR:
        case GB_INSTRUCTION_JR_C:
        case GB_INSTRUCTION_JR_NC:
        case GB_INSTRUCTION_JR_NZ:
        case GB_INSTRUCTION_JR_Z:
            gb_processor_instruction_jr();
            break;
        case GB_INSTRUCTION_LD_A_A:
        case GB_INSTRUCTION_LD_A_B:
        case GB_INSTRUCTION_LD_A_BCI:
        case GB_INSTRUCTION_LD_A_C:
        case GB_INSTRUCTION_LD_A_D:
        case GB_INSTRUCTION_LD_A_DEI:
        case GB_INSTRUCTION_LD_A_E:
        case GB_INSTRUCTION_LD_A_FF00_CI:
        case GB_INSTRUCTION_LD_A_FF00_NI:
        case GB_INSTRUCTION_LD_A_H:
        case GB_INSTRUCTION_LD_A_HLI:
        case GB_INSTRUCTION_LD_A_HLID:
        case GB_INSTRUCTION_LD_A_HLII:
        case GB_INSTRUCTION_LD_A_L:
        case GB_INSTRUCTION_LD_A_N:
        case GB_INSTRUCTION_LD_A_NNI:
        case GB_INSTRUCTION_LD_B_A:
        case GB_INSTRUCTION_LD_B_B:
        case GB_INSTRUCTION_LD_B_C:
        case GB_INSTRUCTION_LD_B_D:
        case GB_INSTRUCTION_LD_B_E:
        case GB_INSTRUCTION_LD_B_H:
        case GB_INSTRUCTION_LD_B_HLI:
        case GB_INSTRUCTION_LD_B_L:
        case GB_INSTRUCTION_LD_B_N:
        case GB_INSTRUCTION_LD_BC_NN:
        case GB_INSTRUCTION_LD_BCI_A:
        case GB_INSTRUCTION_LD_C_A:
        case GB_INSTRUCTION_LD_C_B:
        case GB_INSTRUCTION_LD_C_C:
        case GB_INSTRUCTION_LD_C_D:
        case GB_INSTRUCTION_LD_C_E:
        case GB_INSTRUCTION_LD_C_H:
        case GB_INSTRUCTION_LD_C_HLI:
        case GB_INSTRUCTION_LD_C_L:
        case GB_INSTRUCTION_LD_C_N:
        case GB_INSTRUCTION_LD_D_A:
        case GB_INSTRUCTION_LD_D_B:
        case GB_INSTRUCTION_LD_D_C:
        case GB_INSTRUCTION_LD_D_D:
        case GB_INSTRUCTION_LD_D_E:
        case GB_INSTRUCTION_LD_D_H:
        case GB_INSTRUCTION_LD_D_HLI:
        case GB_INSTRUCTION_LD_D_L:
        case GB_INSTRUCTION_LD_D_N:
        case GB_INSTRUCTION_LD_DE_NN:
        case GB_INSTRUCTION_LD_DEI_A:
        case GB_INSTRUCTION_LD_E_A:
        case GB_INSTRUCTION_LD_E_B:
        case GB_INSTRUCTION_LD_E_C:
        case GB_INSTRUCTION_LD_E_D:
        case GB_INSTRUCTION_LD_E_E:
        case GB_INSTRUCTION_LD_E_H:
        case GB_INSTRUCTION_LD_E_HLI:
        case GB_INSTRUCTION_LD_E_L:
        case GB_INSTRUCTION_LD_E_N:
        case GB_INSTRUCTION_LD_FF00_CI_A:
        case GB_INSTRUCTION_LD_FF00_NI_A:
        case GB_INSTRUCTION_LD_H_A:
        case GB_INSTRUCTION_LD_H_B:
        case GB_INSTRUCTION_LD_H_C:
        case GB_INSTRUCTION_LD_H_D:
        case GB_INSTRUCTION_LD_H_E:
        case GB_INSTRUCTION_LD_H_H:
        case GB_INSTRUCTION_LD_H_HLI:
        case GB_INSTRUCTION_LD_H_L:
        case GB_INSTRUCTION_LD_H_N:
        case GB_INSTRUCTION_LD_HL_NN:
        case GB_INSTRUCTION_LD_HLI_A:
        case GB_INSTRUCTION_LD_HLI_B:
        case GB_INSTRUCTION_LD_HLI_C:
        case GB_INSTRUCTION_LD_HLI_D:
        case GB_INSTRUCTION_LD_HLI_E:
        case GB_INSTRUCTION_LD_HLI_H:
        case GB_INSTRUCTION_LD_HLI_L:
        case GB_INSTRUCTION_LD_HLI_N:
        case GB_INSTRUCTION_LD_HLID_A:
        case GB_INSTRUCTION_LD_HLII_A:
        case GB_INSTRUCTION_LD_L_A:
        case GB_INSTRUCTION_LD_L_B:
        case GB_INSTRUCTION_LD_L_C:
        case GB_INSTRUCTION_LD_L_D:
        case GB_INSTRUCTION_LD_L_E:
        case GB_INSTRUCTION_LD_L_H:
        case GB_INSTRUCTION_LD_L_HLI:
        case GB_INSTRUCTION_LD_L_L:
        case GB_INSTRUCTION_LD_L_N:
        case GB_INSTRUCTION_LD_NNI_A:
        case GB_INSTRUCTION_LD_NNI_SP:
        case GB_INSTRUCTION_LD_SP_HL:
        case GB_INSTRUCTION_LD_SP_NN:
            gb_processor_instruction_ld();
            break;
        case GB_INSTRUCTION_LD_HL_SP:
            gb_processor_instruction_ld_hl();
            break;
        case GB_INSTRUCTION_NOP:
            gb_processor_instruction_nop();
            break;
        case GB_INSTRUCTION_OR_A:
        case GB_INSTRUCTION_OR_B:
        case GB_INSTRUCTION_OR_C:
        case GB_INSTRUCTION_OR_D:
        case GB_INSTRUCTION_OR_E:
        case GB_INSTRUCTION_OR_H:
        case GB_INSTRUCTION_OR_HLI:
        case GB_INSTRUCTION_OR_L:
        case GB_INSTRUCTION_OR_N:
            gb_processor_instruction_or();
            break;
        case GB_INSTRUCTION_POP_AF:
        case GB_INSTRUCTION_POP_BC:
        case GB_INSTRUCTION_POP_DE:
        case GB_INSTRUCTION_POP_HL:
            gb_processor_instruction_pop();
            break;
        case GB_INSTRUCTION_PUSH_AF:
        case GB_INSTRUCTION_PUSH_BC:
        case GB_INSTRUCTION_PUSH_DE:
        case GB_INSTRUCTION_PUSH_HL:
            gb_processor_instruction_push();
            break;
        case GB_INSTRUCTION_RET:
            gb_processor_instruction_ret();
            break;
        case GB_INSTRUCTION_RETI:
            gb_processor_instruction_reti();
            break;
        case GB_INSTRUCTION_RET_C:
        case GB_INSTRUCTION_RET_NC:
        case GB_INSTRUCTION_RET_NZ:
        case GB_INSTRUCTION_RET_Z:
            gb_processor_instruction_ret();
            break;
        case GB_INSTRUCTION_RLA:
            gb_processor_instruction_rla();
            break;
        case GB_INSTRUCTION_RLCA:
            gb_processor_instruction_rlca();
            break;
        case GB_INSTRUCTION_RRA:
            gb_processor_instruction_rra();
            break;
        case GB_INSTRUCTION_RRCA:
            gb_processor_instruction_rrca();
            break;
        case GB_INSTRUCTION_RST_00:
        case GB_INSTRUCTION_RST_08:
        case GB_INSTRUCTION_RST_10:
        case GB_INSTRUCTION_RST_18:
        case GB_INSTRUCTION_RST_20:
        case GB_INSTRUCTION_RST_28:
        case GB_INSTRUCTION_RST_30:
        case GB_INSTRUCTION_RST_38:
            gb_processor_instruction_rst();
            break;
        case GB_INSTRUCTION_SBC_A:
        case GB_INSTRUCTION_SBC_B:
        case GB_INSTRUCTION_SBC_C:
        case GB_INSTRUCTION_SBC_D:
        case GB_INSTRUCTION_SBC_E:
        case GB_INSTRUCTION_SBC_H:
        case GB_INSTRUCTION_SBC_HLI:
        case GB_INSTRUCTION_SBC_L:
        case GB_INSTRUCTION_SBC_N:
            gb_processor_instruction_sbc();
            break;
        case GB_INSTRUCTION_SCF:
            gb_processor_instruction_scf();
            break;
        case GB_INSTRUCTION_STOP:
            gb_processor_instruction_stop();
            break;
        case GB_INSTRUCTION_SUB_A:
        case GB_INSTRUCTION_SUB_B:
        case GB_INSTRUCTION_SUB_C:
        case GB_INSTRUCTION_SUB_D:
        case GB_INSTRUCTION_SUB_E:
        case GB_INSTRUCTION_SUB_H:
        case GB_INSTRUCTION_SUB_HLI:
        case GB_INSTRUCTION_SUB_L:
        case GB_INSTRUCTION_SUB_N:
            gb_processor_instruction_sub();
            break;
        case GB_INSTRUCTION_XOR_A:
        case GB_INSTRUCTION_XOR_B:
        case GB_INSTRUCTION_XOR_C:
        case GB_INSTRUCTION_XOR_D:
        case GB_INSTRUCTION_XOR_E:
        case GB_INSTRUCTION_XOR_H:
        case GB_INSTRUCTION_XOR_HLI:
        case GB_INSTRUCTION_XOR_L:
        case GB_INSTRUCTION_XOR_N:
            gb_processor_instruction_xor();
            break;
        default:
            gb_processor_instruction_xxx();
            break;
        }
    } else {
        processor.instruction.opcode = static_cast<gb_instruction_e>(gb_bus_read(processor.pc.word++));
        switch (processor.instruction.opcode) {
        case GB_INSTRUCTION_BIT_0_B ... GB_INSTRUCTION_BIT_7_A:
            gb_processor_instruction_bit();
            break;
        case GB_INSTRUCTION_RES_0_B ... GB_INSTRUCTION_RES_7_A:
            gb_processor_instruction_res();
            break;
        case GB_INSTRUCTION_RLC_A:
        case GB_INSTRUCTION_RLC_B:
        case GB_INSTRUCTION_RLC_C:
        case GB_INSTRUCTION_RLC_D:
        case GB_INSTRUCTION_RLC_E:
        case GB_INSTRUCTION_RLC_H:
        case GB_INSTRUCTION_RLC_HLI:
        case GB_INSTRUCTION_RLC_L:
            gb_processor_instruction_rlc();
            break;
        case GB_INSTRUCTION_RL_A:
        case GB_INSTRUCTION_RL_B:
        case GB_INSTRUCTION_RL_C:
        case GB_INSTRUCTION_RL_D:
        case GB_INSTRUCTION_RL_E:
        case GB_INSTRUCTION_RL_H:
        case GB_INSTRUCTION_RL_HLI:
        case GB_INSTRUCTION_RL_L:
            gb_processor_instruction_rl();
            break;
        case GB_INSTRUCTION_RRC_A:
        case GB_INSTRUCTION_RRC_B:
        case GB_INSTRUCTION_RRC_C:
        case GB_INSTRUCTION_RRC_D:
        case GB_INSTRUCTION_RRC_E:
        case GB_INSTRUCTION_RRC_H:
        case GB_INSTRUCTION_RRC_HLI:
        case GB_INSTRUCTION_RRC_L:
            gb_processor_instruction_rrc();
            break;
        case GB_INSTRUCTION_RR_A:
        case GB_INSTRUCTION_RR_B:
        case GB_INSTRUCTION_RR_C:
        case GB_INSTRUCTION_RR_D:
        case GB_INSTRUCTION_RR_E:
        case GB_INSTRUCTION_RR_H:
        case GB_INSTRUCTION_RR_HLI:
        case GB_INSTRUCTION_RR_L:
            gb_processor_instruction_rr();
            break;
        case GB_INSTRUCTION_SET_0_B ... GB_INSTRUCTION_SET_7_A:
            gb_processor_instruction_set();
            break;
        case GB_INSTRUCTION_SLA_A:
        case GB_INSTRUCTION_SLA_B:
        case GB_INSTRUCTION_SLA_C:
        case GB_INSTRUCTION_SLA_D:
        case GB_INSTRUCTION_SLA_E:
        case GB_INSTRUCTION_SLA_H:
        case GB_INSTRUCTION_SLA_HLI:
        case GB_INSTRUCTION_SLA_L:
            gb_processor_instruction_sla();
            break;
        case GB_INSTRUCTION_SRA_A:
        case GB_INSTRUCTION_SRA_B:
        case GB_INSTRUCTION_SRA_C:
        case GB_INSTRUCTION_SRA_D:
        case GB_INSTRUCTION_SRA_E:
        case GB_INSTRUCTION_SRA_H:
        case GB_INSTRUCTION_SRA_HLI:
        case GB_INSTRUCTION_SRA_L:
            gb_processor_instruction_sra();
            break;
        case GB_INSTRUCTION_SRL_A:
        case GB_INSTRUCTION_SRL_B:
        case GB_INSTRUCTION_SRL_C:
        case GB_INSTRUCTION_SRL_D:
        case GB_INSTRUCTION_SRL_E:
        case GB_INSTRUCTION_SRL_H:
        case GB_INSTRUCTION_SRL_HLI:
        case GB_INSTRUCTION_SRL_L:
            gb_processor_instruction_srl();
            break;
        case GB_INSTRUCTION_SWAP_A:
        case GB_INSTRUCTION_SWAP_B:
        case GB_INSTRUCTION_SWAP_C:
        case GB_INSTRUCTION_SWAP_D:
        case GB_INSTRUCTION_SWAP_E:
        case GB_INSTRUCTION_SWAP_H:
        case GB_INSTRUCTION_SWAP_HLI:
        case GB_INSTRUCTION_SWAP_L:
            gb_processor_instruction_swap();
            break;
        default:
            break;
        }
    }
}

static void gb_processor_service(void) {
    for (uint8_t interrupt = 0; interrupt < GB_INTERRUPT_MAX; ++interrupt) {
        gb_interrupt_e mask = static_cast<gb_interrupt_e>(1 << interrupt);
        if (processor.interrupt.enable.raw & processor.interrupt.flag.raw & mask) {
            processor.interrupt.flag.raw &= ~mask;
            processor.delay = 4;
            if (processor.halt_bug) {
                processor.halt_bug = false;
            } else {
                processor.delay += 16;
                processor.interrupt.delay = 0;
                processor.interrupt.enabled = false;
                gb_bus_write(--processor.sp.word, processor.pc.high);
                gb_bus_write(--processor.sp.word, processor.pc.low);
                processor.pc.word = (8 * interrupt) + 0x40;
            }
            break;
        }
    }
}

void gb_processor_interrupt(gb_interrupt_e interrupt) {
    gb_processor_write(GB_PROCESSOR_INTERRUPT_FLAG, processor.interrupt.flag.raw | (1 << interrupt));
}

uint8_t gb_processor_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_PROCESSOR_INTERRUPT_ENABLE:
        result = processor.interrupt.enable.raw;
        break;
    case GB_PROCESSOR_INTERRUPT_FLAG:
        result = processor.interrupt.flag.raw;
        break;
    default:
        break;
    }
    return result;
}

void gb_processor_reset(void) {
    memset(&processor, 0, sizeof(processor));
    processor.interrupt.flag.raw = 0xE0;
}

gb_error_e gb_processor_step(void) {
    if (!processor.delay) {
        if (processor.interrupt.delay && !--processor.interrupt.delay) {
            processor.interrupt.enabled = true;
        }
        if (processor.interrupt.enable.raw & processor.interrupt.flag.raw & 0x1F) {
            processor.halted = false;
            if (processor.interrupt.enabled) {
                gb_processor_service();
            } else if (!processor.stopped) {
                gb_processor_execute();
            } else {
                processor.delay = 4;
            }
        } else if (!processor.halted && !processor.stopped) {
            gb_processor_execute();
        } else {
            processor.delay = 4;
        }
    }
    --processor.delay;
    return processor.status;
}

bool gb_processor_stopped(void) {
    return processor.stopped;
}

void gb_processor_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_PROCESSOR_INTERRUPT_ENABLE:
        processor.interrupt.enable.raw = data;
        break;
    case GB_PROCESSOR_INTERRUPT_FLAG:
        processor.interrupt.flag.raw = data | 0xE0;
        if (processor.interrupt.flag.input) {
            processor.stopped = false;
        }
        break;
    default:
        break;
    }
}
