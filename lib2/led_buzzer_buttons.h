#ifndef LED_BUZZER_BUTTONS_H
#define LED_BUZZER_BUTTONS_H

#include "pico/stdlib.h"

#define led_red 13
#define led_green 12
#define led_blue 11

void iniciar_leds()
{
    gpio_init(led_red);
    gpio_set_dir(led_red, GPIO_OUT);
    gpio_init(led_green);
    gpio_set_dir(led_green, GPIO_OUT);
    gpio_init(led_blue);
    gpio_set_dir(led_blue, GPIO_OUT);
}

void leds(bool red, bool green, bool blue)
{
    gpio_put(led_red, red);
    gpio_put(led_green, green);
    gpio_put(led_blue, blue);
}

#include "pico/stdio.h"
#include <stdint.h>
#include "hardware/pwm.h"

#define buzzerA 21

void iniciar_buzzer()
{
    gpio_init(buzzerA);
    gpio_set_dir(buzzerA, GPIO_OUT);
}

void tocar_nota(int frequencia, int duracao) 
{
    if (frequencia == 0) {
        gpio_put(buzzerA, 0);
        return;
    }
    
    int periodo = 1000000 / frequencia;
    int metade_periodo = periodo / 2;
    
    for (int i = 0; i < duracao * 1000L / periodo; i++) {
        gpio_put(buzzerA, 1);
        sleep_us(metade_periodo);
        gpio_put(buzzerA, 0);
        sleep_us(metade_periodo);
    }
}

// Função para beep curto
void beep_curto() 
{
    tocar_nota(500, 300);
}

// Função para beep duplo
void beep_duplo() 
{
    tocar_nota(500, 80);
    sleep_ms(300);
    tocar_nota(500, 80);
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoA 5
#define botaoB 6
#define botaoJ 22

void iniciar_botoes()
{
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_init(botaoJ);
    gpio_set_dir(botaoJ, GPIO_IN);
    gpio_pull_up(botaoJ);
}

#endif