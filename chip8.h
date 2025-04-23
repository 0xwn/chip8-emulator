#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <array>
#include <random>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>



const unsigned int MEMORY_SIZE = 4096;
const unsigned int NUM_REGISTERS = 16;
const unsigned int START_ADDRESS = 0x200;
const unsigned int STACK_LEVELS = 16;
const unsigned int DISPLAY_WIDTH = 64; 
const unsigned int DISPLAY_HEIGHT = 32;
const unsigned int DISPLAY_SIZE = DISPLAY_WIDTH * DISPLAY_HEIGHT;
const unsigned int FONT_START_ADDRESS = 0x050;
const unsigned int FONT_END_ADDRESS = 0x0A0;
const unsigned int FONT_CHARACTER_SIZE = 5;



//fontes padrao do chip-8
const std::array<uint8_t, 80> fontset = {{
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
}};



class Chip8 {
    public:
        std::array<uint8_t, MEMORY_SIZE> memory;
        std::array<uint8_t, NUM_REGISTERS> V;
        std::array<uint16_t, STACK_LEVELS> stack;
        std::array<uint8_t, DISPLAY_SIZE> display_buffer;
        std::array<uint8_t, 16> keypad;


        uint16_t I;
        uint16_t sp; // stack pointer;
        uint16_t pc;
        uint8_t delay_timer;
        uint8_t sound_timer;
        bool key_pressed_wait; // ins FX0A
        uint8_t key_register; //Reg Vx para FX0A
        bool display_updated;

        Chip8(): rand_engine(std::random_device{}()),
        rand_dist(0, 255),
        key_pressed_wait(false),
        key_register(0)
        {
            pc = START_ADDRESS;
            memory.fill(0);
            V.fill(0);
            I = 0;
            stack.fill(0);
            sp = 0;

            display_buffer.fill(0);
            display_updated = false;

            delay_timer = 0;
            sound_timer = 0;

            keypad.fill(0);

            if (FONT_END_ADDRESS > FONT_START_ADDRESS){
                std::memcpy(&memory[FONT_START_ADDRESS], fontset.data(), fontset.size());
            } else {
                throw std::runtime_error("Font set overlaps with program memory start address!");
            }
        }

        bool loadRom(const std::string& filename){
            std::ifstream rom_file(filename, std::ios::binary | std::ios::ate);
            if (!rom_file.is_open()) {
                std::cerr << "Erro: Não foi possível abrir a ROM: " << filename << std::endl;
                return false;
            }
            std::streampos size = rom_file.tellg();
            if (size < 0) {
             std::cerr << "Erro: Não foi possível determinar o tamanho da ROM (tellg falhou)." << std::endl;
             rom_file.close();
             return false;
            }

            rom_file.seekg(0, std::ios::beg);
            const long max_rom_size = MEMORY_SIZE - START_ADDRESS;

            if (size > max_rom_size) {
                std::cerr << "Erro: ROM muito grande (" << size << " bytes). Máximo permitido: " << max_rom_size << " bytes." << std::endl;
                rom_file.close();
                return false;
            }

            char* buffer_start = reinterpret_cast<char*>(&memory[START_ADDRESS]);
            if (!rom_file.read(buffer_start, size)) {
                std::cerr << "Erro: Falha ao ler o conteúdo da ROM para a memória." << std::endl;
                rom_file.close();
                return false; // Pode ter lido parcialmente, mas consideramos falha
           }

           rom_file.close();
           std::cout << "ROM '" << filename << "' (" << size << " bytes) carregada com sucesso em 0x" << std::hex << START_ADDRESS << std::dec << "." << std::endl;
           return true;

           
        }

        uint8_t getSoundTimer() const {
            return sound_timer;
        }

        void updateTimers(){
            if(delay_timer > 0){
                --delay_timer;
            }

            if(sound_timer > 0){
                --sound_timer;
            }
        }
        
        void handleKeyPressEvent(uint8_t key_index){
            if(key_pressed_wait){
                if(key_index < 0xF){
                    V[key_register] = key_index;
                    key_pressed_wait = false;
                }
            }
        }

        bool isWaitingForKey() const {
            return key_pressed_wait;

        }

        void clearKeys(){
            keypad.fill(0);
        }

        void setKeyReleased(uint8_t key_index){
            if (key_index < 0xF){
                keypad[key_index] = 0;
            }
        }

        void setKeyPressed(uint8_t key_index){
            if (key_index < 0xF){
                keypad[key_index] = 1;
            }
        }

        void pushStack(uint16_t address){
            if (sp >= STACK_LEVELS){
                throw std::runtime_error("Stack Overflow!");
            }

            stack[sp] = address;
            sp++;
        }

        uint16_t popStack(){
            if (sp == 0){
                throw std::runtime_error("Stack Underflow!");
            }

            sp--;
            return stack[sp];
        }

        uint8_t getRandomByte(){
            return static_cast<uint8_t>(rand_dist(rand_engine));
        }

        void clearDisplay(){
            display_buffer.fill(0);
            display_updated = true;
        }
        
        bool setPixel(int x, int y, bool state){
            x %= DISPLAY_WIDTH;
            y %= DISPLAY_HEIGHT;

            size_t index = x + (y * DISPLAY_WIDTH);
            bool collision = false;


            bool original_state = (display_buffer[index] == 1);
            bool new_state = original_state ^ state;

            if (original_state && !new_state){
                collision = true;
            }

            display_buffer[index] = new_state ? 1 : 0;
            if (original_state != new_state){
                display_updated = true;
            }

            return collision;
        }
        
        bool getPixel(int x , int y) const {
            x %= DISPLAY_WIDTH;
            y %= DISPLAY_HEIGHT;
            size_t index = x + (y * DISPLAY_WIDTH);
            return display_buffer[index] == 1;
        }
        
        void cycle(){
            if (key_pressed_wait){
                return;
            }

            // fetch opcode
            if (pc > MEMORY_SIZE - 2){
                throw std::runtime_error("out memory during fetch!");
            }

            uint16_t opcode = (static_cast<uint16_t>(memory[pc] << 8 ) | memory[pc + 1]);

            pc += 2;


            //fetch
            uint16_t NNN = opcode & 0x0FFF;
            uint8_t NN = opcode & 0x00FF;
            uint8_t N = opcode & 0x000F;
            uint8_t X = (opcode & 0x0F00) >> 8;
            uint8_t Y = (opcode & 0x00F0) >> 4;
            printf("PC: 0x%04X | Opcode: 0x%04X | NNN: 0x%03X | NN: 0x%02X | N: 0x%X | X: 0x%X | Y: 0x%X\n", pc -2, opcode, NNN, NN, N, X, Y); 

            // decode struct
            switch(opcode & 0xF000){
                //00E0 (CLS), 00EE (RET)
                case 0x0000:
                    switch(NN){
                        case 0xE0:
                            printf("Opcode 00E0: CLS\n");
                            clearDisplay();
                            break;
                        
                        case 0xEE:

                            printf("Opcode 0xEE: RET\n");
                            pc = popStack();
                            break;
                        
                        default:
                            printf("Opcode 0NNN: SYS\n");
                            break;
                    }
                    break;

                case 0x1000: // 1NNN: JP addr - salt para NNN
                    printf("Opcode: 1NNN: JP 0x%03X\n", NNN);
                    pc = NNN;
                    break;

                case 0x2000:
                    pushStack(pc);
                    pc = NNN;
                    break;

                case 0x3000:
                    if (V[X] == NN){
                        pc += 2;
                    }
                    break;
                
                case 0x4000:
                    if (V[X] != NN){
                        pc += 2;
                    }
                    break;

                case 0x5000:
                    if (N == 0){
                        if (V[X] == V[Y]){
                            pc += 2;
                        }
                    } else {
                        printf("Opcode desconhecida (base 5xxx com nibbie final nao zero):0x%04X\n", opcode);
                    }
                    break;

                case 0x6000:
                    V[X] = NN;
                    break;

                case 0x7000:
                    V[X] += NN;
                    break;
                
                case 0x8000:
                    switch (N){
                        case 0x0:
                            V[X] = V[Y];
                            break;
                        case 0x1:
                            V[X] |= V[Y];
                            break;

                        case 0x2:
                            V[X] &= V[Y];
                            break;

                        case 0x3:
                            V[X] ^= V[Y];
                            break;
                        case 0x4:
                            {
                                uint16_t sum = static_cast<uint16_t>(V[X]) + 
                                static_cast<uint16_t>(V[Y]);
                                V[0xF] = (sum > 0xFF) ? 1 : 0;
                                V[X] = static_cast<uint8_t>(sum);

                            }
                            break;

                        case 0x5:
                            V[0xF] = (V[X] >= V[Y]) ? 1 : 0;
                            V[X] -= V[Y];
                            break;
                             
                        case 0x6:
                            V[0xF] = V[X] & 0x1;

                            V[X] >>= 1;
                            break;
                        

                        case 0x7:
                            V[0xF] = (V[Y] >= V[X]) ? 1 : 0;
                            V[X] = V[Y] - V[X];
                            break;
                        
                        case 0xE:
                            V[0xF] = (V[X] & 0x80) >> 7;
                            V[X] <<= 1;
                            break;

                        default:
                            printf("Opcode 0x8XXX desconhecida (N = %X): 0x%04X\n", N, opcode);
                        
                            break;
                    }
                    break;

                case 0x9000:
                    if (N == 0){
                        if(V[X] != V[Y]){
                            pc += 2;
                        }
                    } else {
                        printf("Opcode desconhecida (base 9XXX com nibble final nao zero): 0x%04X\n", opcode);
                    }
                    break;

                case 0xA000:
                    I = NNN;
                    break;

                case 0xB000:
                    pc = V[0] + NNN;
                    break;
                

                case 0xC000:
                    {
                        uint8_t random_byte = getRandomByte();
                        V[X] = random_byte & NN;
                    }
                    break;

                case 0xD000:
                    {
                        uint8_t coordX = V[X];
                        uint8_t coordY = V[Y];
                        uint8_t height = N;
                        V[0xF] = 0;

                        for(int yline = 0; yline < height; ++yline){
                            if(I + yline >= MEMORY_SIZE){
                                break;
                            }
                            uint8_t sprite_byte = memory[I + yline];
                            uint16_t screenY = (coordY + yline);


                            for (int xpixel = 0; xpixel < 8; ++xpixel){
                                if((sprite_byte & (0x80 >> xpixel)) != 0){
                                    uint16_t screenX = (coordX + xpixel);

                                    uint16_t wrappedX = screenX % DISPLAY_WIDTH;
                                    uint16_t wrappedY = screenY % DISPLAY_HEIGHT;

                                    size_t index = wrappedX + (wrappedY * DISPLAY_WIDTH);

                                    if (display_buffer[index] == 1){
                                        V[0xF] = 1;
                                    }

                                    display_buffer[index] ^= 1;
                                }
                            }
                        }

                        display_updated = true;

                    }
                    break;

                case 0xE000:
                        {
                            uint8_t key_code = V[X];

                            if (key_code > 0xF){
                                printf("Aviso: Tentativa de verificar tecla inválida (Vx=0x%X) em opcode ExXX (0x%04X)\n", key_code, opcode);
                                break;
                            }

                            switch (NN){
                                case 0x9E:
                                    if(keypad[key_code] == 1){
                                        pc += 2;
                                    }
                                    break;

                                case 0xA1:
                                    if(keypad[key_code] == 0){
                                        pc += 2;
                                    }
                                    break;
                                default:
                                    printf("Opcode EXXX desconhecida (NN = %02X): 0x%04X\n", NN, opcode);
                                    break;
                            }
                        }
                        break;


                case 0xF000:
                        switch(NN){
                            case 0x07:
                                V[X] = delay_timer;
                                break;

                            case 0x0A:
                                key_pressed_wait = true;
                                key_register = X;
                                break;

                            case 0x15:

                                delay_timer = V[X];
                                break;

                            case 0x18:
                                sound_timer = V[X];
                                break;

                            case 0x1E:
                                I += V[X];
                                break;

                            case 0x29:
                                {
                                    uint8_t digit = V[X] & 0x0F;
                                    I = FONT_START_ADDRESS + (digit * FONT_CHARACTER_SIZE);
                                }
                                break;

                            case 0x33:
                                {
                                    uint8_t value = V[X];
                                    if(I + 2 >= MEMORY_SIZE){
                                        fprintf(stderr, "Erro: Escrita BCD fora dos limites da memória (I=0x%X)\n", I);
                                    } else {
                                        memory[I] = value / 100; // centenas
                                        memory[I+1] = (value / 10) % 10; //dezenas
                                        memory[I+2] = value % 10; // unidades
                                    }
                                }
                                break;
                            
                            case 0x55:
                                if (I + X >= MEMORY_SIZE){
                                    fprintf(stderr, "Erro: Escrita LD [I], Vx fora dos limites (I=0x%X, X=%d)\n", I, X);
                                } else {
                                    for ( uint8_t i = 0; i <= X; ++i){
                                        memory[I + i] = V[i];
                                    }
                                }
                                break;

                            case 0x65:
                                if(I + X >= MEMORY_SIZE){
                                    fprintf(stderr, "Erro: Leitura LD Vx, [I] fora dos limites (I=0x%X, X=%d)\n", I, X);
                                } else {
                                    for (uint8_t i = 0; i <= X; ++i){
                                        V[i] = memory[I + i];
                                    }
                                }
                                break;

                            default:
                            printf("Opcode FXXX desconhecida (NN = %02X): 0x%04X\n", NN, opcode);
                            break;
                            
                        }
                        break;
                        
                default:
                    printf("Opcode Desconhecida ou não implementada: 0x%04X\n", opcode);
                    break;
            }

        }

    private:
        std::default_random_engine rand_engine;
        std::uniform_int_distribution<unsigned int> rand_dist;

};





#endif // CHIP8_H