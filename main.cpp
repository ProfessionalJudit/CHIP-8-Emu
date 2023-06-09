#include <iostream> //For debug
#include <cstdlib>
#include <stdlib.h>
#include <fstream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

bool debug = false;
bool terminal = false;
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
// Entire fontset, cool isn't it?
unsigned char key[16];
unsigned char chip8_fontset[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
int main(int argc, char const *argv[])
{
    // Arguments
    std::cout << argc;
    if (argc > 1 && std::string(argv[1]) == "debug")
    {
        debug = true;
        if (argc > 2 && std::string(argv[2]) == "terminal")
        {
            terminal = true;
        }
    }

    // init
    sf::RenderWindow window(sf::VideoMode(640, 320), "Chip-8 Emu"); // SFML

    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer

    // Load fontset
    for (int i = 0; i < 80; ++i)
        memory[i] = chip8_fontset[i];
    // Set key regitsers to 0
    for (size_t i = 0; i < 16; i++)
        key[i] = 0;

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;
    // load rom
    // Open the rom in binary mode, pass each byte into "c", then writr c into memory[i]
    std::ifstream f("test.ch8", std::ios::binary | std::ios::in);
    char c;
    int j = 512;
    for (int i = 0x200; f.get(c); i++)
    {
        if (j >= 4096)
        {
            return 1; // file size too big memory space over so exit
        }
        memory[i] = (uint8_t)c;
        j++;
    }
    // For debug purposes
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
        case 0x0000:
            switch (opcode >> 4 & 0x00FF)
            {
            case 0x00EE:
                sp--;           // Decrement stack pointer
                pc = stack[sp]; // Set pc to the sp value
                pc += 2;
                break;
            }
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
            // 0x5XY0 5 = instruction, X = V register num, y = register num
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
            // std::cout << (opcode & 0x00FF) << "\n";
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
        case 0x9000:
            // 0x5XY0 5 = instruction, X = V register num, y = register num
            // If v[X] == NN, skip the next instruction
            // Get X/register num (opcode >> 8 & 0x000F)
            // Get NN/Value (opcode & 0x00FF)
            pc += 2;
            if (V[opcode >> 8 & 0x000F] != V[opcode >> 4 & 0x000F])
                pc += 2;
            break;
        case 0xB000:
            // BNNN, Jump to NNN + V0
            pc = opcode & 0x0FFF + V[0];
        case 0xC000:
            // CXNN, X = register num, NN number
            // Sets VX to NN & rand()
            V[opcode >> 8 & 0x000F] = (opcode & 0x00FF) + rand() % 256;
            pc += 2;
        case 0xD000:
        {
            // Draw to screen
            // DXYN, n = number of rows
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;
            // Set VF to 0, this means by default there is no colision betewwn pixels
            V[0xF] = 0;
            // This fors iterates trough the number of rwos the sprite has
            for (size_t yline = 0; yline < height; yline++)
            {
                // Current pixel is the value of I (pinting to the sprite) + the num of rows
                pixel = memory[I + yline];
                // iterate trough the 8 bits a sprite row has
                for (size_t xline = 0; xline < 8; xline++)
                {
                    // Detect if pixel is set to 1
                    // A trick (i saw at a guide, math is hard..), is using 0x80 and bit shift it to check
                    // each individual bit
                    //  0x80 = 1000 0000
                    // If we shift one by one we can compare each bit
                    if ((pixel & (0x80 >> xline)) != 0)
                    {
                        // if its not 0, we check for colision, and draw it
                        // x + line = x coodrd, y + yline * 64, is the y coord (y + yline is the row number,
                        // if we dont multyply by 64 we dont get the actual pixel)
                        // May seem dumb to comment this, but math is hard and i got imsomnia :v
                        // Whem drawing sprites on top of eachother, the pixels get inverted
                        if (gfx[(x + xline + ((y + yline) * 64))] == 1)
                            // If there is colission, VF to 1
                            V[0xF] = 1;
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }
            pc += 2;
        }
        break;
        case 0xE000:
            // Check key presses
            switch (opcode & 0x00FF)
            {
            // EX9E: Skips the next instruction if the key stored in VX is pressed
            case 0x009E:
                if (key[V[(opcode & 0x0F00) >> 8]] != 0)
                    pc += 2;
                pc += 2;
                break;
            case 0x00A1:
                // EX9E: Skips the next instruction if the key stored in VX is not pressed
                if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                    pc += 2;
                pc += 2;
                break;
            default:
                break;
            }
        case 0xF000:
            switch (opcode & 0x00FF)
            {
            case 0x0007:
                // Set vx to delat timer
                V[opcode >> 8 & 0x000F] = delay_timer;
                pc += 2;
                break;
            case 0x000A:
                // Halt until key pressed, unimplemented
                pc += 2;
                break;
            case 0x0015:
                // Delay timer = VX
                delay_timer = V[opcode >> 8 & 0x000F];
                pc += 2;
                break;
            case 0x0018:
                // Sound timer = VX
                sound_timer = V[opcode >> 8 & 0x000F];
                pc += 2;
                break;
            case 0x001E:
                // Adds VX to I. VF is not affected
                if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) // VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
                    V[0xF] = 1;
                else
                    V[0xF] = 0;
                I += V[opcode >> 8 & 0x000F];
                pc += 2;
                break;
            case 0x0029:
                // Sets I to the location of the characcter in vx
                I = memory[V[opcode >> 8 & 0x000F]] * 0x5;
                pc += 2;
                break;
            case 0x0033:
                // Too sleepy to understand/explain how to converto decimal to bcd
                memory[I] = (V[opcode >> 8 & 0x000F] / 100) % 10;
                memory[I + 1] = (V[opcode >> 8 & 0x000F] / 10) % 10;
                memory[I + 2] = V[opcode >> 8 & 0x000F] % 10;
                pc += 2;
                break;
            case 0x0055:
                // Stores registers in memory
                {
                    unsigned short j = 1;
                    for (size_t i = 0; i <= opcode >> 8 & 0x000F; i++)
                    {
                        memory[I + j] = V[i];
                        j++;
                    }
                }
                I += ((opcode & 0x0F00) >> 8) + 1;
                pc += 2;
                break;
            case 0x0065:
                // Stores memory in regiters
                for (int i = 0; i < 16; ++i)
                    V[i] = memory[I + i];
                pc += 2;
                break;
            }
            break;
        default:
            std::cout << "\nUnknown Opcode " << std::hex << opcode;
            break;
        }
        // Clear screen
        if (opcode == 0xE0)
        {
            // std::cout << "Screen cleared\n";
            for (size_t i = 0; i < 64 * 32; i++)
            {
                gfx[i] = 0;
            }
            pc += 2;
        }
        if (opcode == 0xEE)
        {
            sp--;           // Decrement stack pointer
            pc = stack[sp]; // Set pc to the sp value
            pc += 2;
        }

        if (delay_timer > 0)
            --delay_timer;
        if (sound_timer > 0)
        {
            if (sound_timer == 1)
                std::cout << "BEEP!\n";
            --sound_timer;
        }
        if (debug)
        {
            std::cout << "\nCycle: " << count << "\n";
            std::cout << "Opcode: " << std::hex << opcode << "\n";
            std::cout << "I: " << std::hex << I << "\n";
            std::cout << "PC: " << std::hex << pc << "\n";
            std::cout << "Registers: ";
            /**/
            for (size_t i = 0; i < 16; i++)
            {
                std::cout << std::hex << int(V[i]) << " | ";
            }
            std::cout << "\n";
            /**/

            count++;
            if (terminal)
            {
                std::string toPrint = "";
                short j = 0;
                for (size_t i = 0; i < 32 * 64; i++)
                {
                    if (gfx[i] != 0)
                    {
                        toPrint += '#';
                    }
                    else
                    {
                        toPrint += ' ';
                    }

                    if (j == 64)
                    {
                        toPrint += '\n';
                        j = 0;
                    }
                    j++;
                }
                std::cout << toPrint;
                try
                {
                    system("cls");
                }
                catch (const std::exception &e)
                {
                    system("clear");
                }
            }
        }
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Num1)
                {
                    key[0] = 1;
                }else{
                    key[0] = 0;
                }
                if (event.key.code == sf::Keyboard::Num2)
                {
                    key[1] = 1;
                }else{
                    key[1] = 0;
                }
                if (event.key.code == sf::Keyboard::Num3)
                {
                    key[2] = 1;
                }else{
                    key[2] = 0;
                }
                if (event.key.code == sf::Keyboard::Num4)
                {
                    key[3] = 1;
                }else{
                    key[3] = 0;
                }
                if (event.key.code == sf::Keyboard::Q)
                {
                    key[4] = 1;
                }else{
                    key[4] = 0;
                }
                if (event.key.code == sf::Keyboard::W)
                {
                    key[5] = 1;
                }else{
                    key[5] = 0;
                }
                if (event.key.code == sf::Keyboard::E)
                {
                    key[6] = 1;
                }else{
                    key[6] = 0;
                }
                if (event.key.code == sf::Keyboard::R)
                {
                    key[7] = 1;
                }else{
                    key[7] = 0;
                }
                if (event.key.code == sf::Keyboard::A)
                {
                    key[8] = 1;
                }else{
                    key[8] = 0;
                }
                if (event.key.code == sf::Keyboard::S)
                {
                    key[9] = 1;
                }else{
                    key[9] = 0;
                }
                if (event.key.code == sf::Keyboard::D)
                {
                    key[10] = 1;
                }else{
                    key[10] = 0;
                }
                if (event.key.code == sf::Keyboard::F)
                {
                    key[11] = 1;
                }else{
                    key[11] = 0;
                }
                if (event.key.code == sf::Keyboard::Z)
                {
                    key[12] = 1;
                }else{
                    key[12] = 0;
                }
                if (event.key.code == sf::Keyboard::X)
                {
                    key[13] = 1;
                }else{
                    key[13] = 0;
                }
                if (event.key.code == sf::Keyboard::C)
                {
                    key[14] = 1;
                }else{
                    key[14] = 0;
                }
                if (event.key.code == sf::Keyboard::V)
                {
                    key[15] = 1;
                }else{
                    key[15] = 0;
                }
            }
            
        }
        window.clear(sf::Color::Black);
        //----
        int x = 0;
        int y = 0;
        
        for (size_t i = 0; i < 32 * 64; i++)
        {
            if (gfx[i] != 0)
            {
                sf::RectangleShape rectangle(sf::Vector2f(10.f, 10.f));
                rectangle.setPosition(x, y);
                window.draw(rectangle);
            }
            x += 10;
            if (x == 640)
            {
                x = 0;
                y += 10;
            }
            
        }
        //----
        window.display();
    }

    return 0;
}