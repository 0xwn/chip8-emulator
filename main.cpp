#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <string>
#include "chip8.h"


const int AUDIO_FREQUENCY = 44100;
const int AUDIO_SAMPLES = 1024;
const int TONE_HZ = 440;
const Sint16 AUDIO_AMPLITUDE = 3000; 
const int SCREEN_SCALE = 25; // fator de escala
const int SDL_WINDOW_WIDTH  =  DISPLAY_WIDTH * SCREEN_SCALE;
const int SDL_WINDOW_HEIGHT =  DISPLAY_HEIGHT * SCREEN_SCALE;

//cores da janela
const SDL_Color COLOR_BACKGROUND = {0, 0, 0, 255};       // Preto
const SDL_Color COLOR_FOREGROUND = {255, 97, 0, 1};


struct AudioState {
    bool is_beeping = false;
    double wave_pos = 0.0;
    int samples_per_wave = 0;
};

void SDLCALL audioCallback (void* userdata, Uint8* stream, int len){
    AudioState* audio_state = static_cast<AudioState*>(userdata);
    Sint16* buffer = reinterpret_cast<Sint16*>(stream);
    int length = len / sizeof(Sint16);

    if(audio_state == nullptr || audio_state->samples_per_wave == 0){
        SDL_memset(stream, 0, len);
        return;
    }

    for(int i = 0; i < length; ++i){
        if (audio_state->is_beeping){
            buffer[i] = (audio_state->wave_pos < audio_state->samples_per_wave / 2.0) ? AUDIO_AMPLITUDE : -AUDIO_AMPLITUDE;
        } else {
            buffer[i] = 0;
        }

        audio_state->wave_pos += 1.0;
        if(audio_state->wave_pos >= audio_state->samples_per_wave) {
            audio_state->wave_pos -= audio_state->samples_per_wave;
        }
    }

}


int main(int argc, char* argv[]){

    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <caminho_para_rom.ch8>" << std::endl;
        return 1;
    }

    std::string rom_path = argv[1];


    //init sdl
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) { // init de video e audio
        std::cerr << "Erro ao inicializar SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // criação da janela

    SDL_Window* window = SDL_CreateWindow(
        "Emulador CHIP-8 C++",           
        SDL_WINDOWPOS_CENTERED,         
        SDL_WINDOWPOS_CENTERED,         
        SDL_WINDOW_WIDTH,          
        SDL_WINDOW_HEIGHT,        
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Erro ao criar janela SDL: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // -------- criação do renderer ----------

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Erro ao criar renderizador SDL (tentando software fallback): " << SDL_GetError() << std::endl;
        // Fallback para renderizador de software se o acelerado falhar
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
         if (!renderer) {
             std::cerr << "Erro ao criar renderizador SDL (software): " << SDL_GetError() << std::endl;
             SDL_DestroyWindow(window);
             SDL_Quit();
             return 1;
         }
    }

    std::cout << "SDL inicializado com sucesso." << std::endl;
    std::cout << "Janela: " << SDL_WINDOW_WIDTH << "x" << SDL_WINDOW_HEIGHT << " (Escala: " << SCREEN_SCALE << "x)" << std::endl;


    AudioState audio_state;
    audio_state.samples_per_wave = AUDIO_FREQUENCY / TONE_HZ;

    SDL_AudioSpec want, have;
    SDL_AudioDeviceID audio_device;

    SDL_zero(want);

    want.freq = AUDIO_FREQUENCY;
    want.format = AUDIO_S16SYS;

    want.channels = 1;
    want.samples = AUDIO_SAMPLES;
    want.callback = audioCallback;
    want.userdata = &audio_state;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (audio_device == 0){
        std::cerr << "Erro ao abrir o dispositivo de audio: " << SDL_GetError() << std::endl;
    } else {
        if(want.format != have.format) {
            std::cerr << "aviso: formato de audio nao suportado" << std::endl;
        }

        std::cout << "dispositivo de audio aberto. Freq: " << have.freq << std::endl;

        SDL_PauseAudioDevice(audio_device, 0);
    }

    // --------- instancia chip8 ---------------
    Chip8 chip8_instance;
    if (!chip8_instance.loadRom(rom_path)) {
        std::cerr << "Falha ao carregar a ROM. Encerrando." << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1; // Retorna erro se não conseguiu carregar
    }


    // --- keymap Teclado -> Tecla CHIP-8 ---
    std::unordered_map<SDL_Keycode, uint8_t> keymap = {
        {SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC}, // 1 2 3 C
        {SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD}, // Q W E R -> 4 5 6 D
        {SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE}, // A S D F -> 7 8 9 E
        {SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF}  // Z X C V -> A 0 B F
    };

    // --- config do tempo ---

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    const double TARGET_CPU_HZ = 700.0;
    const Duration cpu_cycle_interval(1.0 / TARGET_CPU_HZ);
    TimePoint last_cpu_cycle_time = Clock::now();
    Duration cpu_time_accumulator = Duration(0.0);

    const double TIMER_HZ = 60.0;
    const Duration timer_interval(1.0 / TIMER_HZ);
    TimePoint last_timer_update_time = Clock::now();



    // ---- loop principal -----

    bool is_running = true;
    SDL_Event event;

    while (is_running) {
        TimePoint current_time = Clock::now();
        Duration delta_time = current_time - last_cpu_cycle_time;
        last_cpu_cycle_time = current_time;
        cpu_time_accumulator += delta_time;

        // --- eventos da SDL ---
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: // evento de fechar a janela
                    is_running = false;
                    break;
                
                case SDL_KEYDOWN: { // tecla pra baixo(pressionada)
                    SDL_Keycode key = event.key.keysym.sym;
                    // ESC pra sair
                    if (key == SDLK_ESCAPE) {
                        is_running = false;
                        break;
                    }
                    // verifica se a tecla está mapeada
                    auto it = keymap.find(key);
                    if (it != keymap.end()) {
                        uint8_t chip8_key = it->second;
                        chip8_instance.setKeyPressed(chip8_key);
                        chip8_instance.handleKeyPressEvent(chip8_key); // Notifica Fx0A
                    }
                    break;
                }
                case SDL_KEYUP: { // tecla pra cima(tecla solta)
                    SDL_Keycode key = event.key.keysym.sym;
                    // Verifica se a tecla está mapeada
                    auto it = keymap.find(key);
                    if (it != keymap.end()) {
                         uint8_t chip8_key = it->second;
                         chip8_instance.setKeyReleased(chip8_key);
                    }
                    break;
                }
            } // Fim do switch(event.type)
        } // Fim do while(SDL_PollEvent)


        // atualizar timers (60 HZ) ---
        Duration elapsed_since_last_timer = current_time - last_timer_update_time;
        if (elapsed_since_last_timer >= timer_interval) {
            chip8_instance.updateTimers();
            bool should_beep_now = (chip8_instance.getSoundTimer() > 0);
            if (audio_device != 0){
                SDL_LockAudioDevice(audio_device);

                audio_state.is_beeping = should_beep_now;

                SDL_UnlockAudioDevice(audio_device);
            }
            last_timer_update_time = last_timer_update_time + std::chrono::duration_cast<Clock::duration>(timer_interval);
        }



        // --- executar os ciclos do chip-8  ---
        // Controlado pela velocidade alvo (TARGET_CPU_HZ)
        while (cpu_time_accumulator >= cpu_cycle_interval) {
            chip8_instance.cycle();
            cpu_time_accumulator -= cpu_cycle_interval;
        }


        // --- Renderizar Display ---
        if (chip8_instance.display_updated) {
            // Limpa a tela do renderizador com a cor de fundo
            SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
            SDL_RenderClear(renderer);

            // Define a cor para os pixels acesos
            SDL_SetRenderDrawColor(renderer, COLOR_FOREGROUND.r, COLOR_FOREGROUND.g, COLOR_FOREGROUND.b, COLOR_FOREGROUND.a);

            // Itera sobre o buffer CHIP-8
            for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
                for (int x = 0; x < DISPLAY_WIDTH; ++x) {
                    // Acessa o buffer diretamente (ou use getPixel)
                    // Índice = x + y * Largura
                    if (chip8_instance.display_buffer[x + y * DISPLAY_WIDTH]) {
                         // Cria o retângulo escalado para o pixel CHIP-8
                        SDL_Rect pixel_rect = {
                            x * SCREEN_SCALE, // Posição X na janela
                            y * SCREEN_SCALE, // Posição Y na janela
                            SCREEN_SCALE,     // Largura do retângulo
                            SCREEN_SCALE  
                        };
                        // Desenha o retângulo preenchido
                        SDL_RenderFillRect(renderer, &pixel_rect);
                    }
                }
            }
            // Reseta a flag de atualização
            chip8_instance.display_updated = false;

             // Atualiza a janela com o conteúdo do renderizador
            SDL_RenderPresent(renderer);

        } // Fim do if(display_updated)

    }
}