#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Definição dos GPIOs
#define LED_BLUE 11
#define LED_RED 12
#define LED_GREEN 13
#define BUTTON 5

// Estado do sistema
typedef enum
{
    ALL_OFF,
    ALL_ON,
    BLUE_OFF,
    RED_OFF,
    GREEN_OFF
} LedState;

volatile LedState current_state = ALL_OFF;
volatile bool button_enabled = true; // Controla se o botão pode ser pressionado

// Função de debounce para o botão
bool debounce_button()
{
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_time < 200)
    { // Debounce de 200ms
        return false;
    }

    last_time = current_time;
    return true;
}

// Função para mudar de estado após um tempo
int64_t change_led_state(alarm_id_t id, void *user_data)
{
    // Garante que a sequência só ocorra se um LED estiver ligado
    if (current_state == ALL_OFF)
    {
        return 0;
    }

    switch (current_state)
    {
    case ALL_ON:
        gpio_put(LED_BLUE, 0);
        current_state = BLUE_OFF;
        add_alarm_in_ms(3000, change_led_state, NULL, false);
        break;
    case BLUE_OFF:
        gpio_put(LED_RED, 0);
        current_state = RED_OFF;
        add_alarm_in_ms(3000, change_led_state, NULL, false);
        break;
    case RED_OFF:
        gpio_put(LED_GREEN, 0);
        current_state = GREEN_OFF;
        button_enabled = true; // Habilita o botão novamente
        break;
    default:
        break;
    }
    return 0; // O timer não se repete
}

// Callback do botão
void button_callback(uint gpio, uint32_t events)
{
    if (gpio == BUTTON && debounce_button() && button_enabled)
    {
        button_enabled = false; // Desabilita o botão durante a sequência

        // Liga todos os LEDs
        gpio_put(LED_BLUE, 1);
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        current_state = ALL_ON;

        // Inicia a sequência com um temporizador único
        add_alarm_in_ms(3000, change_led_state, NULL, false);
    }
}

int main()
{
    stdio_init_all();

    // Configuração dos LEDs
    gpio_init(LED_BLUE);
    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    // Inicializa todos os LEDs desligados
    gpio_put(LED_BLUE, 0);
    gpio_put(LED_RED, 0);
    gpio_put(LED_GREEN, 0);

    // Configuração do botão com pull-up interno
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_pull_up(BUTTON);

    // Configura interrupção do botão
    gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // Loop principal
    while (1)
    {
        tight_loop_contents(); // Mantém o loop principal ocupado
    }
}
