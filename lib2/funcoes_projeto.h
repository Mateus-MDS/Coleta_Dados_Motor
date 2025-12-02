#ifndef FUNCOES_PROJETO_H
#define FUNCOES_PROJETO_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"

#include "iniciar_sd.h"

#include "display.h"
#include "acc_giro.h"
#include "led_buzzer_buttons.h"

extern char nome_arquivo[32];
extern int numero_coleta;

extern const int total_amostras;
extern const int intervalo_ms;

// Função para capturar dados do acelerômetro e giroscópio e salvar em arquivo CSV
void capture_imu_data_and_save()
{
    printf("\nCapturando dados do acelerômetro e giroscópio. Aguarde a finalização...\n");
    
    int tempo_total_s = (total_amostras * intervalo_ms) / 1000;
    printf("Tempo estimado: %d segundos\n", tempo_total_s);
    ssd1306_fill(&ssd, false);
    escrever(&ssd, "tempo restante", 5, 5, cor);
    char tempo_str[20];
    snprintf(tempo_str, sizeof(tempo_str), "%d segundos", tempo_total_s);
    escrever(&ssd, tempo_str, 5, 20, cor);
    
    // Gera nome do arquivo como tarefa1.csv, tarefa2.csv, etc.
    char nome_base[32];
    strncpy(nome_base, nome_arquivo, sizeof(nome_base));
    nome_base[sizeof(nome_base)-1] = '\0';
    char *ext = strstr(nome_base, ".csv");
    if (ext) *ext = '\0';
    char nome_arquivo_incrementado[32];
    snprintf(nome_arquivo_incrementado, sizeof(nome_arquivo_incrementado), "%s%d.csv", nome_base, numero_coleta);
    numero_coleta++;
    
    FIL file;
    printf("Arquivo de saída: %s\n", nome_arquivo_incrementado);
    FRESULT res = f_open(&file, nome_arquivo_incrementado, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Não foi possível abrir o arquivo para escrita. Monte o cartão.\n");
        ssd1306_fill(&ssd, false);
        escrever(&ssd, "ERRO: SD ausente", 5, 5, cor);
        escrever(&ssd, "Monte o cartão!", 5, 20, cor);
        return;
    }
    
    UINT bw;
    int16_t accel[3], gyro[3];
    
    // Escreve o cabeçalho do arquivo CSV com ponto e vírgula
    char cabecalho[] = "Amostra;Aceleracao_X;Aceleracao_Y;Aceleracao_Z;Giro_X;Giro_Y;Giro_Z\n";
    res = f_write(&file, cabecalho, strlen(cabecalho), &bw);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível escrever cabeçalho no arquivo.\n");
        ssd1306_fill(&ssd, false);
        escrever(&ssd, "ERRO ao gravar", 5, 5, cor);
        escrever(&ssd, "cabeçalho!", 5, 20, cor);
        f_close(&file);
        return;
    }
    
    absolute_time_t start_time = get_absolute_time();
    for (int i = 0; i < total_amostras; i++)
    {
        ler_acc_giro(accel, gyro); // Função definida em acc_giro.h/lib2
        
        // Formata os dados com ponto e vírgula separando cada valor
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%d;%d;%d;%d;%d;%d;%d\n", 
                 i + 1, accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2]);
        
        res = f_write(&file, buffer, strlen(buffer), &bw);
        if (res != FR_OK)
        {
            printf("[ERRO] Não foi possível escrever no arquivo. Monte o cartão.\n");
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "ERRO ao gravar", 5, 5, cor);
            escrever(&ssd, "Monte o cartão!", 5, 20, cor);
            f_close(&file);
            return;
        }
        
        // Exibe tempo restante a cada 50 amostras e atualiza display
        if ((i + 1) % 50 == 0 || i == 0) {
            absolute_time_t now = get_absolute_time();
            int elapsed_ms = to_ms_since_boot(now) - to_ms_since_boot(start_time);
            int remaining_ms = (total_amostras - (i + 1)) * intervalo_ms;
            int remaining_s = remaining_ms / 1000;
            printf("Amostra %d/%d - Tempo restante estimado: %d segundos\n", i + 1, total_amostras, remaining_s);
            
            // Atualiza display
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Amostra:", 5, 5, cor);
            char amostra_str[15];
            snprintf(amostra_str, sizeof(amostra_str), "%d/%d", i + 1, total_amostras);
            escrever(&ssd, amostra_str, 70, 5, cor);
            
            escrever(&ssd, "Tempo restante:", 5, 20, cor);
            char tempo_str[20];
            snprintf(tempo_str, sizeof(tempo_str), "%d segundos", remaining_s);
            escrever(&ssd, tempo_str, 5, 35, cor);
        }
        
        sleep_ms(intervalo_ms);
    }
    
    f_close(&file);
    printf("\nDados do acelerômetro e giroscópio salvos no arquivo %s.\n\n", nome_arquivo_incrementado);
    ssd1306_fill(&ssd, false);
    escrever(&ssd, "Dados salvos!", 10, 2, cor);
    
    // Exibe informações do arquivo criado
    printf("Formato do arquivo:\n");
    printf("- Cabeçalho: Amostra;Aceleracao_X;Aceleracao_Y;Aceleracao_Z;Giro_X;Giro_Y;Giro_Z\n");
    printf("- Dados organizados em colunas separadas por ponto e vírgula\n");
    printf("- Pronto para importar em Excel ou Google Sheets (usar ; como separador)\n");
}

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *nome_arquivo)
{
    // Monta o nome do arquivo com índice mais recente
    char nome_base[32];
    strncpy(nome_base, nome_arquivo, sizeof(nome_base));
    nome_base[sizeof(nome_base)-1] = '\0';
    char *ext = strstr(nome_base, ".csv");
    if (ext) *ext = '\0';
    char nome_arquivo_ler[32];
    snprintf(nome_arquivo_ler, sizeof(nome_arquivo_ler), "%s%d.csv", nome_base, numero_coleta-1);
    
    FIL file;
    FRESULT res = f_open(&file, nome_arquivo_ler, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o cartão está montado ou se o arquivo existe.\n");
        ssd1306_fill(&ssd, false);
        escrever(&ssd, "ERRO leitura SD", 5, 5, cor);
        escrever(&ssd, "Verifique o cartão!", 5, 20, cor);
        return;
    }
    
    char buffer[128];
    UINT br;
    printf("Conteúdo do arquivo %s:\n", nome_arquivo_ler);
    printf("========================================\n");
    
    // Lê e exibe o cabeçalho
    if (f_gets(buffer, sizeof(buffer), &file) != NULL) {
        printf("CABEÇALHO: %s", buffer);
    }
    
    // Lê e exibe as primeiras 5 linhas de dados como exemplo
    printf("\nPRIMEIRAS 5 AMOSTRAS:\n");
    for (int i = 0; i < 5; i++) {
        if (f_gets(buffer, sizeof(buffer), &file) != NULL) {
            printf("%s", buffer);
        } else {
            break;
        }
    }
    
    // Se houver mais dados, informa quantas amostras existem
    int total_linhas = 1; // já contamos o cabeçalho
    while (f_gets(buffer, sizeof(buffer), &file) != NULL) {
        total_linhas++;
    }
    
    printf("========================================\n");
    printf("Total de amostras no arquivo: %d\n", total_linhas - 1); // subtrai o cabeçalho
    printf("Formato: CSV com 7 colunas separadas por ponto e vírgula\n");
    printf("(Amostra; Aceleracao_X; Y; Z; Giro_X; Y; Z)\n");
    
    f_close(&file);
    printf("\nLeitura do arquivo %s concluída.\n\n", nome_arquivo_ler);
}

static void run_help()
{
    printf("\nComandos disponíveis:\n\n");
    printf("Digite 'a' para montar o cartão SD\n");
    printf("Digite 'b' para desmontar o cartão SD\n");
    printf("Digite 'c' para listar arquivos\n");
    printf("Digite 'd' para mostrar o conteúdo do arquivo\n");
    printf("Digite 'e' para obter o espaço livre no cartão SD\n");
    printf("Digite 'f' para capturar dados do IMU e salvar no arquivo\n");
    printf("Digite 'g' para formatar o cartão SD\n");
    printf("Digite 'h' para exibir os comandos disponíveis\n");
    printf("\nEscolha o comando:  ");
}

typedef void (*p_fn_t)();
typedef struct
{
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc, "setrtc <DD> <MM> <YY> <hh> <mm> <ss>: Set Real Time Clock"},
    {"format", run_format, "format [<drive#:>]: Formata o cartão SD"},
    {"mount", run_mount, "mount [<drive#:>]: Monta o cartão SD"},
    {"unmount", run_unmount, "unmount <drive#:>: Desmonta o cartão SD"},
    {"getfree", run_getfree, "getfree [<drive#:>]: Espaço livre"},
    {"ls", run_ls, "ls: Lista arquivos"},
    {"cat", run_cat, "cat <filename>: Mostra conteúdo do arquivo"},
    {"help", run_help, "help: Mostra comandos disponíveis"}};

static void process_stdio(int cRxedChar)
{
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar); // eco
    stdio_flush();
    if (cRxedChar == '\r')
    {
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd))
        {
            printf("> ");
            stdio_flush();
            return;
        }
        char *cmdn = strtok(cmd, " ");
        if (cmdn)
        {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i)
            {
                if (0 == strcmp(cmds[i].command, cmdn))
                {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i)
                printf("Comando \"%s\" não encontrado\n", cmdn);
        }
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    }
    else
    {
        if (cRxedChar == '\b' || cRxedChar == (char)127)
        {
            if (ix > 0)
            {
                ix--;
                cmd[ix] = '\0';
            }
        }
        else
        {
            if (ix < sizeof cmd - 1)
            {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}

#define debounce_delay 300
volatile uint tempo_interrupcao = 0;

// Variáveis de estado para captura e montagem do SD
volatile bool captura_ativa = false;
volatile bool sd_montado = false;
volatile bool mount_request_flag = false;
volatile bool capture_request_flag = false;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint tempo_atual = to_ms_since_boot(get_absolute_time());

    // Corrige lógica: só executa ação se passou o tempo de debounce
    if (tempo_atual - tempo_interrupcao > debounce_delay) {
        if (gpio == botaoA) {
            // Alterna captura de dados
            if (!captura_ativa) {
                printf("Botão A: Solicitando captura de dados...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Capturando...", 10, 2, cor);
                capture_request_flag = true;
            } else {
                // Não há mecanismo de parada imediata, apenas exibe mensagem
                printf("Botão A: Parar captura (não implementado interrupção imediata).\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Parar: aguarde", 10, 2, cor);
            }
        } else if (gpio == botaoB) {
            // Alterna montagem/desmontagem do SD
            if (!sd_montado) {
                printf("Botão B: Solicitando montagem do SD...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Montando SD...", 10, 2, cor);
                mount_request_flag = true;
            } else {
                printf("Botão B: Desmontando SD...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Desmontando SD...", 10, 2, cor);
                run_unmount_default();
                sd_montado = false;
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "SD Desmontado", 10, 2, cor);
            }
        } else if (gpio == botaoJ) {
            printf("Botão J pressionado. Reiniciando o dispositivo...\n");
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Reiniciando...", 10, 2, cor);
            reset_usb_boot(0, 0);
        }
        tempo_interrupcao = tempo_atual; // Atualiza o tempo da última interrupção
    }
}

#endif