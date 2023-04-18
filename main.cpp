#include <iostream> //For debug
bool debug = false;
// Define Vars that will hold the memory and registers
// To "emulate" the memory we'll use unsigned chars,
// An unsigned char end up being a 1 byte unsigned integer, so it is exactly what is needed
// Also, since we're using this variables as if they were ram, it would bt nice not to have...
// ... a bit wasted for the sign(+ | -), we wont be using it
unsigned char memory[4096]; // Memory, the chip-8 uses 4096 bytes of memeory
//                            Only   512 bytes were used for the "os"
unsigned char V[16]; // General use registers, 16 8-bit registers, names are V1 to VF
unsigned short I;    // Address register, 16bit(only 12 used), used in memory operations
//                Holds the address of the memory to be processed
//                Like, if we want to add a memory addres to the v1 register it would be like:
//                I points to mem address -> V1 Gets data from I
unsigned short opcode;      // Operation Code, the instruction to exec
unsigned short pc;          // Program Counter, 16bit, holds the address of the next instruction to be processed
unsigned char gfx[64 * 32]; // Screen, one byte per pixel
// Timers, 60hz timers, when not 0 they will count down to 0
unsigned char delay_timer; // Delay timer, used to time events
unsigned char sound_timer; // Sound timer, when it reaches 0, a "beep" is played
// Stack
unsigned short stack[16]; // Stack, has 16 levels
unsigned short sp;        // Stack pointer, points to the currently used stack level

int main(int argc, char const *argv[])
{
    // init
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer

    int count = 0;

    // Set value of 0xFFA
    while (true)
    {
        // Emulate 1 Cycle
        // Get instruction
        opcode = memory[pc] << 8 | memory[pc + 1];
        // Decode instruction
        // opcode & 0xF000 is an "and" operation to ...
        // ... just get the first byte, the one that contains the ...
        // ... actual instruction
        switch (opcode & 0xF000)
        {
        // "ANNN" Set I to NNN
        case 0xA000:
            // Emulate the A instruction, move 2 bytes to I
            // Merge 2 bytes and store them in a short
            // opcode now is equal instruction ( first byte ), data (remaining 3)
            // We do not need to copy the instruction,
            // We do the "and" operation tu substract it
            /* 1010 101010101010 op + data
             *  0000 111111111111 0x0FFF
             *  0000 101010101010 0x0 + data
             */
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        // Executes a subroutine (equivalent to a function~)
        case 0x2000:
            stack[sp] = pc;       // We store the program counter in the stack
            ++sp;                 // Increment the stack pointer
            pc = opcode & 0x0FFF; // Set the program counter to the subroutine addres
            break;
        // Returns from a subroutine ( sooo like a return; )
        // Go to the last stack level and set the pc to that
        case 0x00EE:
            sp--;           // Decrement stack pointer
            pc = stack[sp]; // Set pc to the sp value
            pc += 2;
            break;
        // Equality contitional
        case 0x3000:
            // 0x3XNN 3 = instruction, X = V register num, NN = value
            // If v[X] == NN, skip the next instruction
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            pc += 2;
            if (V[opcode >> 8 & 0x000F] == opcode & 0x00FF)
                pc += 2;
            break;
        // Inequality contitional
        case 0x4000:
            // 0x3XNN 4 = instruction, X = V register num, NN = value
            // If v[X] == NN, skip the next instruction
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            pc += 2;
            if (V[opcode >> 8 & 0x000F] != opcode & 0x00FF)
                pc += 2;
            break;
        // Equality contitional with registers
        case 0x5000:
            // 0x3XY0 5 = instruction, X = V register num, y = register num
            // If v[X] == NN, skip the next instruction
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            pc += 2;
            if (V[opcode >> 8 & 0x000F] == V[opcode >> 4 & 0x000F])
                pc += 2;
            break;
        case 0x6000:
            // 0x6XNN, X = V register num, NN = value
            // Assign NN to V[X]
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            V[(opcode >> 8 & 0x000F)] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000:
            // 0x7XNN, X = V register num, NN = value
            // Sum NN to V[X]
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            V[(opcode >> 8 & 0x000F)] += opcode & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            // 0x0XYN, X = V register num, Y = V register num, N = actual instrucion num
            // Multiple cases based on the last half byte
            switch (opcode >> 8 & 0x000F)
            {
            case 0x0000:
                // VX = VY
                V[opcode >> 8 & 0x000F] = V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0001:
                // Bitwise or between VX and VY
                V[opcode >> 8 & 0x000F] = V[opcode >> 8 & 0x000F] | V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0002:
                // Bitwise and between VX and VY
                V[opcode >> 8 & 0x000F] = V[opcode >> 8 & 0x000F] & V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0003:
                // Bitwise xor between VX and VY
                V[opcode >> 8 & 0x000F] = V[opcode >> 8 & 0x000F] ^ V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0004:
                // Adds VY to VX, if there is a carry set V[F] to 1
                if (V[opcode >> 8 & 0x000F] + V[opcode >> 4 & 0x000F] > 0xFF)
                    V[0xF] = 0x1;
                else
                    V[0xF] = 0x0;
                V[opcode >> 8 & 0x000F] += V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0005:
                // Subs VY to VX, if there is a carry set V[F] to 1
                if (V[opcode >> 8 & 0x000F] - V[opcode >> 4 & 0x000F] < 0x0)
                    V[0xF] = 0x1;
                else
                    V[0xF] = 0x0;
                V[opcode >> 8 & 0x000F] -= V[opcode >> 4 & 0x000F];
                pc += 2;
                break;
            case 0x0006:
                // Stores the least significant bit of VX in VF and then shifts VX to the right by 1
                V[0xF] = V[opcode >> 8 & 0x000F] & 0x0001;
                V[opcode >> 8 & 0x000F] >> 1;
                pc += 2;
                break;
            case 0x0007:
                // Sets VX to VY - VV, if there is a carry set V[F] to 1
                if (V[opcode >> 4 & 0x000F] - V[opcode >> 8 & 0x000F] < 0x0)
                    V[0xF] = 0x1;
                else
                    V[0xF] = 0x0;
                V[opcode >> 8 & 0x000F] = V[opcode >> 4 & 0x000F] - V[opcode >> 8 & 0x000F];
                pc += 2;
                break;
            case 0x000E:
                // Stores the most significant bit of VX in VF and then shifts VX to the left by 1
                V[0xF] = V[opcode >> 8 & 0x000F] >> 11 & 0x0001;
                V[opcode >> 8 & 0x000F] << 1;
                pc += 2;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        // Print opcode
        if (debug)
        {
            std::cout << "Cycle: " << count << "\n";
            std::cout << "Optcode: " << std::hex << opcode << "\nInstruction: " << (opcode & 0xF000) << "\n";
            std::cout << "I: " << std::hex << I << "\n";
            std::cout << "sp: " << std::hex << sp << "\n";
            std::cout << "PC: " << std::hex << pc << "\n\n";

            if (count == 1)
            {
                debug = false;
            }
        }
        count++;
    }

    return 0;
}
