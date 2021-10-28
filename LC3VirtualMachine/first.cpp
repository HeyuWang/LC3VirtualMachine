//
// Created by wangheyu on 2021/10/22.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>

//memory
uint16_t memory[UINT16_MAX];

//registers
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND,
    R_COUNT
};

uint16_t reg[R_COUNT];


//operations
enum {
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

//conditional flag
enum {
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

uint16_t sign_extend(uint16_t x, int bit_count) {
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}


void update_flags(uint16_t r) {
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    } else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        reg[R_COND] = FL_NEG;
    } else {
        reg[R_COND] = FL_POS;
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!read_image(argv[j])) {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    /* set the PC to starting position */
    /* 0x3000 is the default */
    enum {
        PC_START = 0x3000
    };
    reg[R_PC] = PC_START;

    int running = 1;

    while (running) {
        /* FETCH */
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
            case OP_ADD: {
                /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;
                /* first operand (SR1) */
                uint16_t r1 = (instr >> 6) & 0x7;
                /* whether we are in immediate mode */
                uint16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                } else {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }
                update_flags(r0);
            }
                break;
            case OP_AND: {
                /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;
                /* first operand (SR1) */
                uint16_t r1 = (instr >> 6) & 0x7;
                /* whether we are in immediate mode */
                uint16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] & imm5;
                } else {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
            }
                break;
            case OP_NOT: {
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t SR = (instr >> 6) & 0x7;
                reg[DR] = ~reg[SR];
                update_flags(DR);
            }
                break;
            case OP_BR: {
                uint16_t negativeFlag = (instr >> 11) & 0x1;
                uint16_t zeroFlag = (instr >> 10) & 0x1;
                uint16_t positiveFlag = (instr >> 9) & 0x1;
                if (negativeFlag || zeroFlag || positiveFlag) {
                    reg[R_PC] = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
                }
            }
                break;
            case OP_JMP: {
                uint16_t BaseR = (instr >> 6) & 0x7;
                reg[R_PC] = reg[BaseR];
            }
                break;
            case OP_JSR: {

            }
                break;
            case OP_LD: {
                /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;

                reg[r0] = mem_read(reg[R_PC] + sign_extend(instr & 0x1FF, 9));

                update_flags(r0);
            }
                break;
            case OP_LDI: {
                /* destination register (DR) */
                uint16_t r0 = (instr >> 9) & 0x7;
                /* PCoffset 9*/
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                /* add pc_offset to the current PC, look at that memory location to get the final address */
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
            }
                break;

            //load relative = load base+offset
            case OP_LDR: {
                uint16_t DR = (instr >> 9) & 0x3;
                uint16_t BaseR = (instr >> 6) & 0x7;
                uint16_t offset6 = sign_extend(instr & 0x1F, 6);

                reg[DR] = mem_read(reg[BaseR] + offset6);
                update_flags(DR);

            }
                break;

            //load effective address
            case OP_LEA: {
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t offset9 = sign_extend(instr & 0x1FF, 9);
                reg[DR] = reg[R_PC] + offset9;
                update_flags(DR);
            }
                break;
            //store:将寄存器里的值存到内存中
            case OP_ST: {
                uint16_t SR = (instr >> 9) & 0x7;
                uint16_t address = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
                mem_set(address, reg[SR]);
            }
                break;
            //store indirect
            case OP_STI: {
                uint16_t SR = (instr >> 9) & 0x7;
                uint16_t address = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
                mem_set(mem_read(address), reg[SR]);
            }
                break;
            //store relative base+
            case OP_STR: {
                uint16_t SR = (instr >> 9) & 0x7;
                uint16_t BaseR = (instr >> 6) & 0x7;
                uint16_t address = reg[BaseR] + sign_extend(instr & 0x3F, 6);
                mem_set(address, reg[SR]);
            }
                break;
            //system call
            case OP_TRAP: {
                reg[R_R7] = reg[R_PC]++;
                uint16_t trapvect8 = instr & 0xFF;
                reg[R_PC] = mem_read(trapvect8);
            }
                break;
                //return from interrupt
            case OP_RES:
            case OP_RTI:
            default: {
                //ignore
            }
                break;
        }
    }
}

