#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//Register A is called the Accumalator
//Condition Code Z = Zero Flag(Set to true when the result equals zero)
//Condition Code S = Sign Flag(Set to true when bit 7(the most signifigant bit) of the math instruction is set)
//Condition Code P = Parity Flag(set when the answer has even parity, cleared when the answer has odd parity)
//Condition Code CY = Carry Flag(set to true when the instruction is set the to a carry out or borrow into the high order bit)
//Condition Code AC = Auxillary Carry Flag(Used in binary code math NOT USED IN SPACE INVADERS!)

//When you see if(x & 0xff) that is a mask it filters out everything except for all of the exsential bits


//0xff = 11111111
//0x80 = 10000000

//KEY:
//NTS = Not Sure About It fidget with it if you don't get the expected output


struct ConditionCodes {

    uint8_t z:1;
    uint8_t s:1;
    uint8_t p:1;
    uint8_t cy:1;
    uint8_t ac:1;
    uint8_t pad:3;

};


typedef struct State8080{


    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memory;
    struct ConditionCodes cc;
    uint8_t int_enable;

} State8080;


void UnimplementedInstruction(State8080* state){
    printf("Error: Unimplemented Instruction\n");
    exit(1);
}

void Emulate8080OP(State8080* state){
    unsigned char *opcodes = state->memory[state->pc];

    switch(*opcodes){
        case 0x00: break; //NOP
        case 0x01: //LXI B
            state->b = opcodes[2];
            state->c = opcodes[1];
            state->pc += 2;
            break;
        case 0x02: UnimplementedInstruction(state); break; //STAX B
        case 0x03: UnimplementedInstruction(state); break;//INX B
        case 0x04: UnimplementedInstruction(state); break;//INR B
        //.........
        
        //Todo: Create More Opcodes

        case 0x40://MOV B
            state->b = state->b;
            break;
        case 0x41://MOV C
            state->b = state->c;
            break;
        case 0x42://MOV D
            state->b = state->d;
            break;
        case 0x43://MOV E
            state->b = state->e;
            break;

        //Logic Group

        //CMA Flips bits so a 1 will be flipped to a 0 and the other way around
        //XOR best explained here: http://www.emulator101.com/logical-group.html

        case 0x0f://RRC
        {
            uint8_t x = state->a;
            state->a = ((x & 1) << 7) | (x >> 1);
            state->cc.cy = (1 == (x & 1));
        } break;


        case 0x1f://RAR
        {
            uint8_t x = state->a;
            state->a = (state->cc.cy << 7) | (x >> 1);
            state->cc.cy = (1 == (x & 1));
        } break;


        case 0x2f://CMA (NOT)
            state->a = ~state->a;
            break;
        
        case 0xe6://ANI
            {
                uint8_t x = state->a & opcodes[1];
                state->cc.z = (x == 0);
                state->cc.s = ((x & 0x80) == 0x80);
                state->cc.cy = 0;
                state->cc.p = Parity(x, 8);
                state->a = x;
                state->pc++;
            } break;


        case 0xee://XRI
            {
                uint8_t x = state->a ^ opcodes[1];
                state->cc.z = (x == 0);
                state->cc.s = ((x & 0x80) == 0x80);
                state->cc.cy = 0;
                state->cc.p = Parity(x, 8);
                state->a = x;
                state->pc++;
            }

        case 0xf6://ORI
             {
                uint8_t x = state->a | opcodes[1];
                state->cc.z = (x == 0);
                state->cc.s = ((x & 0x80) == 0x80);
                state->cc.cy = 0;
                state->cc.p = Parity(x, 8);
                state->a = x;
                state->pc++;
             } break;


        //Branch Group

        case 0xc2://JNZ
            if(0 == state->cc.z)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;
        case 0xc3://JMP
            state->pc = (opcodes[2] << 8) | opcodes[1];
            break;
        case 0xca://JZ
            if(1 == state->cc.z)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;

        case 0xd2://JNC
            if(0 == state->cc.cy)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;

        case 0xda://JC
            if(1 == state->cc.cy)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;
        case 0xe2://JPO
            if(0 == state->cc.p)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;

        case 0xea://JPE
            if(1 == state->cc.p)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;
        
        case 0xf2://JP
            if(1 == state->cc.s)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;

        case 0xfa://JM
            if(0 == state->cc.s)
                state->pc = (opcodes[2] << 8) | opcodes[1];
            else
                state->pc += 2;
            break;
        case 0xc0://RNZ

             if(state->cc.z == 0){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else{
                state->pc += 0;
            } break;

        case 0xc4://CNZ
            if(state->cc.z == 0){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;

        case 0xc7:{ //RST 0
            uint16_t ret = 0x0;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;

        case 0xc8://RZ
            if(state->cc.z == 1){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else{
                state->pc += 0;
            } break;

        case 0xc9://RET
            state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
            state->sp += 2;
            break;

        case 0xcc://CZ
            if(state->cc.z == 1){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;

        case 0xcd://CALL
            {
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } break;

        case 0xcf:{ //RST 1
            uint16_t ret = 0x8;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;


        case 0xd0://RNC
            if(state->cc.cy == 0){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;
        
        case 0xd4://CNC
            if(state->cc.cy == 0){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;

        
        case 0xd7:{ //RST 2
            uint16_t ret = 0x10;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;


        case 0xd8://RC
            if(state->cc.cy == 1){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;
            
        
        case 0xdc://CC
            if(state->cc.cy == 1){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;

        case 0xdf:{ //RST 3
            uint16_t ret = 0x18;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;
        
        
        case 0xe0://RPO
            if(state->cc.p == 1){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;


        case 0xe4://CPO
            if(state->cc.p == 0){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;
        

        case 0xe7:{ //RST 4
            uint16_t ret = 0x20;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;


        case 0xe8://RPE
            if(state->cc.p == 1){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;


        case 0xe9://PCHL
            state->pc = state->l | (state->h << 8);
            break;
        
        case 0xec://CPE
            if(state->cc.p == 1){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;


        case 0xef:{ //RST 5
            uint16_t ret = 0x28;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;

        case 0xf0://RP
            if(state->cc.s == 1){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;


        case 0xf4://CP
            if(state->cc.s == 1){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;


        case 0xf7:{ //RST 6
            uint16_t ret = 0x30;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;


        case 0xf8://CM
            case 0xf0://RP
            if(state->cc.s == 0){
                state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                state->sp += 2;
            } else {
                state->pc += 0;
            } break;


        case 0xfc://CM
            if(state->cc.s == 0){
                uint16_t ret = state->pc+2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret& 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcodes[2] << 8) | opcodes[1];
            } else {
                state->pc += 2;
            } break;
        
        case 0xff:{ //RST 7
            uint16_t ret = 0x38;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret& 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcodes[2] << 8) | opcodes[1];
         } break;


        //Maths Group

        //Finish Up the Maths

        case 0x80://ADD B
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;

                //This is masking the answer to see if the mask equals zero if so it will set the Condition Code Z to true other wise it will clear it
                if((answer & 0xff) == 0)
                    state->cc.z = 1;

                else
                    state->cc.z = 0;

                //This checks if bit 7 is true and if so it will set Condition Code S to true other wise it will clear it
                if(answer & 0x80)
                    state->cc.s = 1;
                else
                    state->cc.s = 0;

                //This checks to see if the answer is bigger than 0xff and 0xff = 11111111 so if that happens it means there is an overflow and it has to set the carry flag to true otherwise it sets it to false
                if(answer > 0xff)
                    state->cc.cy = 1;
                else
                    state->cc.cy = 0;

                //This sets the Condition Code P to true if the mask of answer and 0xff is true otherwise it will be set to false 
                state->cc.p = Parity(answer & 0xff);

                state->a = answer & 0xff;
                
            }

        case 0x81://ADD C
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x82://ADD D
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->d;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x83://ADD E
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->e;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }hbu

        case 0x84://ADD H
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->h;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }
        
        case 0x85://ADD L
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->l;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x86://ADD M
            {
                uint16_t offset = (state->h<<8) | (state->l);   
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->memory[offset];
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x87://ADD A
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->a;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x88://ADC B
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->b + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x89://ADC C
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->c + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x8a://ADC D
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->d + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }
        case 0x8b://ADC E
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->e + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x8c://ADC H
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->h + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x8d://ADC L
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->l + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }
        
        case 0x8e://ADC M
            {
                uint16_t offset = (state->h<<8) | (state->l);
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->memory[offset] + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x8f://ADC A
            {
                uint16_t answer = (uint16_t) state->a + (uint16_t) state->a + (uint16_t) state->cc.cy;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x90://SUB B
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->b;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }

        case 0x91://SUB C
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->c;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            }          
        
        //NTS
        case 0x92://SUB D
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->d;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 

        case 0x93://SUB E
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->e;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 
        
        //NTS
        case 0x94://SUB H
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->h;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 

        
        case 0x95://SUB L
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->l;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 

        case 0x96://SUB M
            {
                uint16_t offset = (state->h<<8) | (state->l);
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->memory[offset];
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 
        case 0x97://SUB A
            {
                uint16_t answer = (uint16_t) state->a - (uint16_t) state->a;
                state->cc.z = ((answer & 0xff) == 0);
                state->cc.s = (answer & 0x80);
                state->cc.cy = (answer > 0xff);
                state->cc.p = Parity(answer & 0xff);
                state->a = answer & 0xff;
            } 
    }

    state->pc+=1;

}

