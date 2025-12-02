#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "lwip/tcp.h"
#include "acc_giro.h"
#include "ff.h"
#include "hw_config.h"
#include "rtc.h"

// ================= CONFIGURACOES WI-FI =================
#define WIFI_SSID "computador"
#define WIFI_PASS "12345678"

// ================= CONFIGURACOES I2C =================
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// ================= VARIAVEIS GLOBAIS =================
int16_t accel[3] = {0, 0, 0};
int16_t gyro[3] = {0, 0, 0};

// Controle de SD Card
bool sd_montado = false;
FATFS fs;
FIL arquivo;

// Controle de captura
typedef enum {
    OPERACAO_NORMAL_AXIAL,
    OPERACAO_NORMAL_RADIAL,
    FALTA_FASE_AXIAL,
    FALTA_FASE_RADIAL
} TipoOperacao;

bool captura_ativa = false;
TipoOperacao modo_operacao = OPERACAO_NORMAL_AXIAL;
int amostras_capturadas = 0;
int total_amostras = 1000;
int intervalo_ms = 10;
char nome_arquivo[64] = "";

// ================= PAGINA HTML =================
const char HTML_BODY[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Monitor de Vibracao - Motor</title>"
"<script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js'></script>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box;}"
"body{font-family:'Segoe UI',Tahoma,sans-serif;background:#0f172a;color:#e2e8f0;}"
".container{max-width:1600px;margin:0 auto;padding:20px;}"

// Header
".header{background:linear-gradient(135deg,#1e293b,#334155);padding:30px;border-radius:12px;margin-bottom:30px;box-shadow:0 4px 20px rgba(0,0,0,0.3);}"
".header h1{font-size:2.5em;color:#38bdf8;margin-bottom:10px;}"
".header p{color:#94a3b8;font-size:1.1em;}"

// Grid Principal
".main-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-bottom:20px;}"

// SD Card Panel
".sd-panel{background:#1e293b;padding:25px;border-radius:12px;box-shadow:0 4px 15px rgba(0,0,0,0.2);margin-bottom:20px;}"
".sd-panel h2{color:#38bdf8;margin-bottom:20px;font-size:1.5em;border-bottom:2px solid #334155;padding-bottom:10px;}"
".sd-status{display:flex;align-items:center;gap:15px;margin-bottom:20px;padding:15px;background:#0f172a;border-radius:8px;}"
".sd-indicator{width:20px;height:20px;border-radius:50%;}"
".sd-indicator.connected{background:#10b981;box-shadow:0 0 10px #10b981;}"
".sd-indicator.disconnected{background:#ef4444;box-shadow:0 0 10px #ef4444;}"
".sd-info{flex:1;}"
".sd-info h4{color:#94a3b8;font-size:0.9em;margin-bottom:5px;}"
".sd-info p{color:#e2e8f0;font-size:1.2em;font-weight:bold;}"

// Controle de Captura
".capture-panel{background:#1e293b;padding:25px;border-radius:12px;box-shadow:0 4px 15px rgba(0,0,0,0.2);}"
".capture-panel h2{color:#38bdf8;margin-bottom:20px;font-size:1.5em;border-bottom:2px solid #334155;padding-bottom:10px;}"
".mode-selector{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-bottom:20px;}"
".mode-btn{padding:20px;border:2px solid #334155;border-radius:8px;background:#0f172a;cursor:pointer;transition:all 0.3s;text-align:center;}"
".mode-btn:hover{border-color:#38bdf8;transform:translateY(-2px);}"
".mode-btn.active{background:#1e40af;border-color:#38bdf8;box-shadow:0 0 20px rgba(56,189,248,0.3);}"
".mode-btn h3{color:#38bdf8;margin-bottom:8px;font-size:1.1em;}"
".mode-btn p{color:#94a3b8;font-size:0.9em;}"
".mode-icon{font-size:2em;margin-bottom:10px;}"

// Botoes de Controle
".control-buttons{display:flex;gap:10px;margin-top:20px;}"
".btn{flex:1;padding:15px;border:none;border-radius:8px;font-size:1em;font-weight:bold;cursor:pointer;transition:all 0.3s;}"
".btn-mount{background:#8b5cf6;color:white;}"
".btn-mount:hover{background:#7c3aed;transform:translateY(-2px);}"
".btn-unmount{background:#64748b;color:white;}"
".btn-unmount:hover{background:#475569;transform:translateY(-2px);}"
".btn-start{background:#10b981;color:white;}"
".btn-start:hover{background:#059669;transform:translateY(-2px);}"
".btn-stop{background:#ef4444;color:white;}"
".btn-stop:hover{background:#dc2626;transform:translateY(-2px);}"
".btn-download{background:#3b82f6;color:white;}"
".btn-download:hover{background:#2563eb;transform:translateY(-2px);}"
".btn:disabled{opacity:0.5;cursor:not-allowed;transform:none;}"

// Status
".status-display{background:#0f172a;padding:15px;border-radius:8px;margin-top:15px;border-left:4px solid #ef4444;}"
".status-display.active{border-color:#10b981;}"
".status-display h4{color:#94a3b8;font-size:0.9em;margin-bottom:5px;}"
".status-display p{color:#e2e8f0;font-size:1.3em;font-weight:bold;}"

// Info Cards
".info-panel{background:#1e293b;padding:25px;border-radius:12px;box-shadow:0 4px 15px rgba(0,0,0,0.2);}"
".info-panel h2{color:#38bdf8;margin-bottom:20px;font-size:1.5em;border-bottom:2px solid #334155;padding-bottom:10px;}"
".info-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:15px;}"
".info-card{background:#0f172a;padding:20px;border-radius:8px;border-left:4px solid #38bdf8;}"
".info-card h4{color:#94a3b8;font-size:0.85em;text-transform:uppercase;margin-bottom:8px;letter-spacing:1px;}"
".info-card p{color:#e2e8f0;font-size:1.8em;font-weight:bold;}"
".info-card span{color:#94a3b8;font-size:0.6em;font-weight:normal;}"

// Valores em Tempo Real
".values-section{margin-top:20px;}"
".values-section h2{color:#38bdf8;margin-bottom:20px;font-size:1.8em;}"
".values-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:15px;margin-bottom:20px;}"
".value-card{background:#1e293b;padding:20px;border-radius:12px;text-align:center;border-top:4px solid;}"
".value-card.accel-x{border-color:#ef4444;}"
".value-card.accel-y{border-color:#3b82f6;}"
".value-card.accel-z{border-color:#10b981;}"
".value-card.gyro-x{border-color:#8b5cf6;}"
".value-card.gyro-y{border-color:#f59e0b;}"
".value-card.gyro-z{border-color:#06b6d4;}"
".value-card h3{color:#94a3b8;font-size:0.9em;margin-bottom:10px;text-transform:uppercase;}"
".value-large{font-size:2.5em;color:#e2e8f0;font-weight:bold;margin:10px 0;}"
".value-unit{color:#64748b;font-size:0.9em;}"

// Graficos
".charts-section{margin-top:20px;}"
".chart-container{background:#1e293b;padding:25px;border-radius:12px;margin-bottom:20px;box-shadow:0 4px 15px rgba(0,0,0,0.2);}"
".chart-container h3{color:#38bdf8;margin-bottom:15px;font-size:1.3em;}"
"canvas{max-width:100%;height:350px!important;}"

// Configuracoes
".config-section{background:#1e293b;padding:25px;border-radius:12px;margin-top:20px;}"
".config-section h2{color:#38bdf8;margin-bottom:20px;font-size:1.5em;}"
".config-row{display:flex;align-items:center;gap:15px;margin-bottom:15px;}"
".config-row label{color:#94a3b8;min-width:200px;}"
".config-row input{background:#0f172a;border:2px solid #334155;color:#e2e8f0;padding:10px;border-radius:6px;width:150px;}"
".config-row input:focus{outline:none;border-color:#38bdf8;}"

// Mensagens
".message{padding:15px;border-radius:8px;margin-top:15px;font-weight:bold;}"
".message.success{background:#10b981;color:white;}"
".message.error{background:#ef4444;color:white;}"
".message.info{background:#3b82f6;color:white;}"

"@media (max-width:1200px){.main-grid{grid-template-columns:1fr;}.values-grid{grid-template-columns:repeat(2,1fr);}}"
"</style>"
"</head><body>"
"<div class='container'>"

// Header
"<div class='header'>"
"<h1>Sistema de Monitoramento de Vibracao - Motor Eletrico</h1>"
"<p>Analise de Operacao Normal e Falta de Fase - Medicoes Axiais e Radiais</p>"
"</div>"

// SD Card Panel
"<div class='sd-panel'>"
"<h2>Cartao SD</h2>"
"<div class='sd-status'>"
"<div class='sd-indicator disconnected' id='sd-indicator'></div>"
"<div class='sd-info'>"
"<h4>STATUS DO CARTAO</h4>"
"<p id='sd-status-text'>Desconectado</p>"
"</div>"
"</div>"
"<div class='control-buttons'>"
"<button class='btn btn-mount' onclick='montarSD()' id='btn-mount'>Montar SD</button>"
"<button class='btn btn-unmount' onclick='desmontarSD()' id='btn-unmount' disabled>Desmontar SD</button>"
"</div>"
"<div class='control-buttons' style='margin-top:10px'>"
"<button class='btn' style='background:#f59e0b' onclick='testarConexao()'>Testar Conexao</button>"
"</div>"
"</div>"

// Grid Principal
"<div class='main-grid'>"

// Painel de Controle
"<div class='capture-panel'>"
"<h2>Controle de Coleta</h2>"

// Seletor de Modo
"<div class='mode-selector'>"
"<div class='mode-btn active' onclick='selecionarModo(0)' id='modo-0'>"
"<div class='mode-icon'>&#8597;</div>"
"<h3>Normal - Axial</h3>"
"<p>Motor em operacao normal<br>Direcao axial</p>"
"</div>"
"<div class='mode-btn' onclick='selecionarModo(1)' id='modo-1'>"
"<div class='mode-icon'>&#8596;</div>"
"<h3>Normal - Radial</h3>"
"<p>Motor em operacao normal<br>Direcao radial</p>"
"</div>"
"<div class='mode-btn' onclick='selecionarModo(2)' id='modo-2'>"
"<div class='mode-icon'>&#9888;</div>"
"<h3>Falta Fase - Axial</h3>"
"<p>Motor com falta de fase<br>Direcao axial</p>"
"</div>"
"<div class='mode-btn' onclick='selecionarModo(3)' id='modo-3'>"
"<div class='mode-icon'>&#9888;</div>"
"<h3>Falta Fase - Radial</h3>"
"<p>Motor com falta de fase<br>Direcao radial</p>"
"</div>"
"</div>"

// Botoes
"<div class='control-buttons'>"
"<button class='btn btn-start' onclick='iniciarCaptura()' id='btn-start' disabled>INICIAR</button>"
"<button class='btn btn-stop' onclick='pararCaptura()' id='btn-stop' disabled>PARAR</button>"
"</div>"

// Status
"<div class='status-display' id='status-display'>"
"<h4>STATUS</h4>"
"<p id='status-text'>Aguardando SD</p>"
"</div>"

"<div id='message'></div>"
"</div>"

// Painel de Informacoes
"<div class='info-panel'>"
"<h2>Informacoes da Coleta</h2>"
"<div class='info-grid'>"
"<div class='info-card'>"
"<h4>Amostras</h4>"
"<p id='info-amostras'>0 <span>/ 1000</span></p>"
"</div>"
"<div class='info-card'>"
"<h4>Taxa de Amostragem</h4>"
"<p id='info-taxa'>100 <span>Hz</span></p>"
"</div>"
"<div class='info-card'>"
"<h4>Modo de Operacao</h4>"
"<p style='font-size:1.2em' id='info-modo'>Normal - Axial</p>"
"</div>"
"<div class='info-card'>"
"<h4>Arquivo Atual</h4>"
"<p style='font-size:0.9em' id='info-arquivo'>---</p>"
"</div>"
"</div>"
"</div>"

"</div>"

// Valores em Tempo Real
"<div class='values-section'>"
"<h2>Leituras em Tempo Real</h2>"
"<div class='values-grid'>"
"<div class='value-card accel-x'>"
"<h3>Aceleracao X</h3>"
"<div class='value-large' id='accel-x'>0.00</div>"
"<div class='value-unit'>m/s²</div>"
"</div>"
"<div class='value-card accel-y'>"
"<h3>Aceleracao Y</h3>"
"<div class='value-large' id='accel-y'>0.00</div>"
"<div class='value-unit'>m/s²</div>"
"</div>"
"<div class='value-card accel-z'>"
"<h3>Aceleracao Z</h3>"
"<div class='value-large' id='accel-z'>0.00</div>"
"<div class='value-unit'>m/s²</div>"
"</div>"
"<div class='value-card gyro-x'>"
"<h3>Giroscopio X</h3>"
"<div class='value-large' id='gyro-x'>0.00</div>"
"<div class='value-unit'>°/s</div>"
"</div>"
"<div class='value-card gyro-y'>"
"<h3>Giroscopio Y</h3>"
"<div class='value-large' id='gyro-y'>0.00</div>"
"<div class='value-unit'>°/s</div>"
"</div>"
"<div class='value-card gyro-z'>"
"<h3>Giroscopio Z</h3>"
"<div class='value-large' id='gyro-z'>0.00</div>"
"<div class='value-unit'>°/s</div>"
"</div>"
"</div>"
"</div>"

// Graficos
"<div class='charts-section'>"
"<div class='chart-container'>"
"<h3>Acelerometro (m/s²)</h3>"
"<canvas id='graficoAccel'></canvas>"
"</div>"
"<div class='chart-container'>"
"<h3>Giroscopio (°/s)</h3>"
"<canvas id='graficoGyro'></canvas>"
"</div>"
"</div>"

// Configuracoes
"<div class='config-section'>"
"<h2>Configuracoes de Captura</h2>"
"<div class='config-row'>"
"<label>Total de Amostras:</label>"
"<input id='total_amostras' type='number' value='1000'>"
"<button class='btn btn-start' onclick='atualizarConfig()'>Atualizar</button>"
"</div>"
"<div class='config-row'>"
"<label>Intervalo (ms):</label>"
"<input id='intervalo_ms' type='number' value='10'>"
"<span style='color:#64748b;margin-left:10px'>Taxa: <span id='taxa-calc'>100</span> Hz</span>"
"</div>"
"</div>"

"</div>"

"<script>"
// Variaveis globais
"let labels=[];"
"let accelXData=[],accelYData=[],accelZData=[];"
"let gyroXData=[],gyroYData=[],gyroZData=[];"
"let maxDataPoints=100;"
"let graficoAccel,graficoGyro;"
"let modoAtual=0;"
"let sdMontado=false;"
"const modoNomes=['Normal-Axial','Normal-Radial','FaltaFase-Axial','FaltaFase-Radial'];"

// Criar graficos
"function criarGraficos(){"
"if(typeof Chart==='undefined'){setTimeout(criarGraficos,100);return;}"
"Chart.defaults.color='#94a3b8';"
"Chart.defaults.borderColor='#334155';"

"const ctxAccel=document.getElementById('graficoAccel').getContext('2d');"
"graficoAccel=new Chart(ctxAccel,{"
"type:'line',"
"data:{"
"labels:labels,"
"datasets:[{"
"label:'X',data:accelXData,borderColor:'#ef4444',backgroundColor:'rgba(239,68,68,0.1)',borderWidth:2,tension:0.3"
"},{"
"label:'Y',data:accelYData,borderColor:'#3b82f6',backgroundColor:'rgba(59,130,246,0.1)',borderWidth:2,tension:0.3"
"},{"
"label:'Z',data:accelZData,borderColor:'#10b981',backgroundColor:'rgba(16,185,129,0.1)',borderWidth:2,tension:0.3"
"}]"
"},"
"options:{"
"responsive:true,"
"maintainAspectRatio:false,"
"interaction:{mode:'index',intersect:false},"
"scales:{y:{grid:{color:'#334155'}}},"
"plugins:{legend:{position:'top'}}"
"}"
"});"

"const ctxGyro=document.getElementById('graficoGyro').getContext('2d');"
"graficoGyro=new Chart(ctxGyro,{"
"type:'line',"
"data:{"
"labels:labels,"
"datasets:[{"
"label:'X',data:gyroXData,borderColor:'#8b5cf6',backgroundColor:'rgba(139,92,246,0.1)',borderWidth:2,tension:0.3"
"},{"
"label:'Y',data:gyroYData,borderColor:'#f59e0b',backgroundColor:'rgba(245,158,11,0.1)',borderWidth:2,tension:0.3"
"},{"
"label:'Z',data:gyroZData,borderColor:'#06b6d4',backgroundColor:'rgba(6,182,212,0.1)',borderWidth:2,tension:0.3"
"}]"
"},"
"options:{"
"responsive:true,"
"maintainAspectRatio:false,"
"interaction:{mode:'index',intersect:false},"
"scales:{y:{grid:{color:'#334155'}}},"
"plugins:{legend:{position:'top'}}"
"}"
"});"
"}"

// Atualizar dados
"function atualizarDados(){"
"fetch('/dados_mpu').then(r=>r.json()).then(data=>{"
"if(labels.length>=maxDataPoints){"
"labels.shift();"
"accelXData.shift();accelYData.shift();accelZData.shift();"
"gyroXData.shift();gyroYData.shift();gyroZData.shift();"
"}"

"let agora=new Date().toLocaleTimeString();"
"labels.push(agora);"
"accelXData.push(data.accel_x);accelYData.push(data.accel_y);accelZData.push(data.accel_z);"
"gyroXData.push(data.gyro_x);gyroYData.push(data.gyro_y);gyroZData.push(data.gyro_z);"

// Atualizar cards
"document.getElementById('accel-x').textContent=data.accel_x.toFixed(2);"
"document.getElementById('accel-y').textContent=data.accel_y.toFixed(2);"
"document.getElementById('accel-z').textContent=data.accel_z.toFixed(2);"
"document.getElementById('gyro-x').textContent=data.gyro_x.toFixed(2);"
"document.getElementById('gyro-y').textContent=data.gyro_y.toFixed(2);"
"document.getElementById('gyro-z').textContent=data.gyro_z.toFixed(2);"

// Info
"document.getElementById('info-amostras').innerHTML=data.amostras+' <span>/ '+data.total+'</span>';"
"document.getElementById('info-taxa').innerHTML=(1000/data.intervalo).toFixed(0)+' <span>Hz</span>';"
"if(data.arquivo)document.getElementById('info-arquivo').textContent=data.arquivo;"

// Status SD
"sdMontado=data.sd_montado;"
"if(sdMontado){"
"document.getElementById('sd-indicator').className='sd-indicator connected';"
"document.getElementById('sd-status-text').textContent='Conectado';"
"document.getElementById('btn-mount').disabled=true;"
"document.getElementById('btn-unmount').disabled=false;"
"document.getElementById('btn-start').disabled=false;"
"}else{"
"document.getElementById('sd-indicator').className='sd-indicator disconnected';"
"document.getElementById('sd-status-text').textContent='Desconectado';"
"document.getElementById('btn-mount').disabled=false;"
"document.getElementById('btn-unmount').disabled=true;"
"document.getElementById('btn-start').disabled=true;"
"}"

// Status captura
"if(data.captura_ativa){"
"document.getElementById('status-display').classList.add('active');"
"document.getElementById('status-text').textContent='CAPTURANDO ('+Math.round(data.amostras/data.total*100)+'%)';"
"document.getElementById('btn-start').disabled=true;"
"document.getElementById('btn-stop').disabled=false;"
"}else{"
"document.getElementById('status-display').classList.remove('active');"
"if(sdMontado){"
"document.getElementById('status-text').textContent='PRONTO';"
"}else{"
"document.getElementById('status-text').textContent='Aguardando SD';"
"}"
"document.getElementById('btn-start').disabled=!sdMontado;"
"document.getElementById('btn-stop').disabled=true;"
"}"

"graficoAccel.update('none');graficoGyro.update('none');"
"}).catch(err=>console.error('Erro:',err));"
"}"

// Controle SD
"function montarSD(){"
"fetch('/sd_mount').then(r=>r.text()).then(msg=>{"
"mostrarMsg(msg,msg.includes('Erro')?'error':'success');"
"});"
"}"

"function desmontarSD(){"
"fetch('/sd_unmount').then(r=>r.text()).then(msg=>{"
"mostrarMsg(msg,'info');"
"});"
"}"

// Selecionar modo
"function selecionarModo(modo){"
"modoAtual=modo;"
"for(let i=0;i<4;i++){"
"document.getElementById('modo-'+i).classList.remove('active');"
"}"
"document.getElementById('modo-'+modo).classList.add('active');"
"document.getElementById('info-modo').textContent=modoNomes[modo];"
"fetch('/set_modo?modo='+modo);"
"}"

// Controle de captura
"function iniciarCaptura(){"
"if(!sdMontado){alert('Monte o cartao SD primeiro!');return;}"
"fetch('/captura_start').then(r=>r.text()).then(msg=>{"
"mostrarMsg(msg,msg.includes('Erro')?'error':'success');"
"});"
"}"

"function pararCaptura(){"
"fetch('/captura_stop').then(r=>r.text()).then(msg=>{"
"mostrarMsg(msg,'info');"
"});"
"}"

// Atualizar config
"function atualizarConfig(){"
"let total=document.getElementById('total_amostras').value||1000;"
"let intervalo=document.getElementById('intervalo_ms').value||10;"
"fetch(`/parametros?total=${total}&intervalo=${intervalo}`).then(r=>r.text()).then(msg=>{"
"mostrarMsg(msg,'success');"
"document.getElementById('taxa-calc').textContent=(1000/intervalo).toFixed(0);"
"});"
"}"

// Calcular taxa ao digitar
"document.addEventListener('DOMContentLoaded',function(){"
"document.getElementById('intervalo_ms').addEventListener('input',function(){"
"let intervalo=this.value||10;"
"document.getElementById('taxa-calc').textContent=(1000/intervalo).toFixed(0);"
"});"
"});"

// Mostrar mensagem
"function mostrarMsg(msg,tipo){"
"document.getElementById('message').innerHTML=`<div class='message ${tipo}'>${msg}</div>`;"
"setTimeout(()=>document.getElementById('message').innerHTML='',4000);"
"}"

// Inicializacao
"window.onload=function(){"
"criarGraficos();"
"atualizarDados();"
"setInterval(atualizarDados,100);"
"};"
"</script>"
"</body></html>";

// ================= ESTRUTURA HTTP =================
struct http_state {
    char response[20000];
    size_t len;
    size_t sent;
};

// ================= FUNCOES AUXILIARES =================
void converter_valores_mpu(float *accel_conv, float *gyro_conv) {
    for (int i = 0; i < 3; i++) {
        float accel_g = accel[i] / 16384.0f;
        accel_conv[i] = accel_g * 9.81f;
        gyro_conv[i] = gyro[i] / 131.0f;
    }
}

bool montar_sd() {
    printf("[DEBUG] Tentando montar SD Card...\n");
    
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr == FR_OK) {
        sd_montado = true;
        printf("[OK] SD Card montado com sucesso!\n");
        return true;
    } else {
        sd_montado = false;
        printf("[ERRO] Falha ao montar SD Card!\n");
        printf("[ERRO] Codigo de erro: %d\n", fr);
        return false;
    }
}

void desmontar_sd() {
    printf("[DEBUG] Desmontando SD Card...\n");
    
    if (captura_ativa) {
        printf("[INFO] Fechando arquivo de captura...\n");
        f_sync(&arquivo);
        f_close(&arquivo);
        captura_ativa = false;
        printf("[OK] Arquivo fechado\n");
    }
    
    f_unmount("");
    sd_montado = false;
    printf("[OK] SD Card desmontado\n");
}

bool criar_arquivo_captura() {
    printf("[DEBUG] Criando arquivo de captura...\n");
    
    const char* modo_str[] = {"Normal_Axial", "Normal_Radial", 
                               "FaltaFase_Axial", "FaltaFase_Radial"};
    
    // Gerar nome do arquivo com timestamp
    uint32_t timestamp = to_ms_since_boot(get_absolute_time()) / 1000;
    snprintf(nome_arquivo, sizeof(nome_arquivo), 
             "%s_%lu.csv", modo_str[modo_operacao], timestamp);
    
    printf("[INFO] Nome do arquivo: %s\n", nome_arquivo);
    
    FRESULT fr = f_open(&arquivo, nome_arquivo, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("[ERRO] Nao foi possivel criar arquivo!\n");
        printf("[ERRO] Codigo: %d\n", fr);
        return false;
    }
    
    // Escrever cabeçalho
    f_printf(&arquivo, "Tempo_ms;Modo;Accel_X;Accel_Y;Accel_Z;Giro_X;Giro_Y;Giro_Z\n");
    f_sync(&arquivo);
    
    printf("[OK] Arquivo criado: %s\n", nome_arquivo);
    return true;
}

void salvar_dados_sd(uint32_t tempo_ms, float *accel_conv, float *gyro_conv) {
    const char* modo_str[] = {"Normal_Axial", "Normal_Radial", 
                               "FaltaFase_Axial", "FaltaFase_Radial"};
    
    f_printf(&arquivo, "%lu;%s;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
             tempo_ms,
             modo_str[modo_operacao],
             accel_conv[0], accel_conv[1], accel_conv[2],
             gyro_conv[0], gyro_conv[1], gyro_conv[2]);
    
    // Sincronizar a cada 10 amostras para não perder dados
    if (amostras_capturadas % 10 == 0) {
        f_sync(&arquivo);
        if (amostras_capturadas % 100 == 0) {
            printf("[INFO] %d amostras gravadas...\n", amostras_capturadas);
        }
    }
}

// ================= CALLBACKS HTTP =================
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    
    if (hs->sent < hs->len) {
        size_t remaining = hs->len - hs->sent;
        size_t to_send = remaining > tcp_sndbuf(tpcb) ? tcp_sndbuf(tpcb) : remaining;
        tcp_write(tpcb, hs->response + hs->sent, to_send, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    } else {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    
    // Log da requisição recebida
    char req_line[128];
    int len = (p->len < 127) ? p->len : 127;
    memcpy(req_line, req, len);
    req_line[len] = '\0';
    // Pega só a primeira linha
    char *newline = strchr(req_line, '\r');
    if (newline) *newline = '\0';
    printf("[HTTP] Requisicao: %s\n", req_line);
    
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    // Pagina principal
    if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /index.html", 15) == 0) {
        int content_length = strlen(HTML_BODY);
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            content_length, HTML_BODY);
        
        tcp_arg(tpcb, hs);
        tcp_sent(tpcb, http_sent);
        size_t first_chunk = hs->len > tcp_sndbuf(tpcb) ? tcp_sndbuf(tpcb) : hs->len;
        tcp_write(tpcb, hs->response, first_chunk, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        pbuf_free(p);
        return ERR_OK;
    }
    
    // Dados do MPU
    else if (strstr(req, "GET /dados_mpu")) {
        ler_acc_giro(accel, gyro);
        float accel_conv[3], gyro_conv[3];
        converter_valores_mpu(accel_conv, gyro_conv);
        
        char json[600];
        int json_len = snprintf(json, sizeof(json),
            "{\"accel_x\":%.2f,\"accel_y\":%.2f,\"accel_z\":%.2f,"
            "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
            "\"captura_ativa\":%s,\"amostras\":%d,\"total\":%d,\"intervalo\":%d,"
            "\"modo\":%d,\"sd_montado\":%s,\"arquivo\":\"%s\"}",
            accel_conv[0], accel_conv[1], accel_conv[2],
            gyro_conv[0], gyro_conv[1], gyro_conv[2],
            captura_ativa ? "true" : "false",
            amostras_capturadas, total_amostras, intervalo_ms, 
            modo_operacao,
            sd_montado ? "true" : "false",
            nome_arquivo);

        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", json_len, json);
    }
    
    // Endpoint de teste
    else if (strstr(req, "GET /ping")) {
        printf("[HTTP] Ping recebido!\n");
        const char *resp = "pong";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Montar SD
    else if (strstr(req, "GET /sd_mount")) {
        printf("\n[WEB] Requisicao recebida: Montar SD\n");
        const char *resp;
        if (montar_sd()) {
            resp = "SD Card montado com sucesso!";
        } else {
            resp = "Erro ao montar SD Card!";
        }
        printf("[WEB] Resposta enviada: %s\n\n", resp);
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Desmontar SD
    else if (strstr(req, "GET /sd_unmount")) {
        printf("\n[WEB] Requisicao recebida: Desmontar SD\n");
        desmontar_sd();
        const char *resp = "SD Card desmontado!";
        printf("[WEB] Resposta enviada: %s\n\n", resp);
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Definir modo de operacao
    else if (strstr(req, "GET /set_modo")) {
        printf("\n[WEB] Requisicao recebida: Definir modo\n");
        char *modo_str = strstr(req, "modo=");
        if (modo_str) {
            int modo = atoi(modo_str + 5);
            if (modo >= 0 && modo <= 3) {
                modo_operacao = (TipoOperacao)modo;
                printf("[INFO] Modo alterado para: %d\n", modo);
            }
        }
        const char *resp = "Modo atualizado";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Iniciar captura
    else if (strstr(req, "GET /captura_start")) {
        printf("[DEBUG] Requisicao de inicio de captura recebida\n");
        
        const char *resp;
        if (!sd_montado) {
            printf("[ERRO] SD nao montado\n");
            resp = "Erro: Monte o SD Card primeiro!";
        } else if (captura_ativa) {
            printf("[AVISO] Captura ja ativa\n");
            resp = "Erro: Captura ja esta ativa!";
        } else {
            printf("[INFO] Iniciando captura...\n");
            if (criar_arquivo_captura()) {
                captura_ativa = true;
                amostras_capturadas = 0;
                resp = "Captura iniciada com sucesso!";
                printf("[OK] Captura iniciada: %s\n", nome_arquivo);
            } else {
                resp = "Erro ao criar arquivo no SD!";
                printf("[ERRO] Falha ao criar arquivo\n");
            }
        }
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Parar captura
    else if (strstr(req, "GET /captura_stop")) {
        printf("[DEBUG] Requisicao de parada de captura recebida\n");
        
        char resp[100];
        if (captura_ativa) {
            printf("[INFO] Finalizando captura...\n");
            f_sync(&arquivo);
            f_close(&arquivo);
            captura_ativa = false;
            printf("[OK] Captura finalizada: %d amostras salvas em %s\n", 
                   amostras_capturadas, nome_arquivo);
            snprintf(resp, sizeof(resp), "Captura parada. %d amostras salvas em %s", 
                     amostras_capturadas, nome_arquivo);
        } else {
            printf("[AVISO] Nenhuma captura ativa\n");
            snprintf(resp, sizeof(resp), "Nenhuma captura ativa");
        }
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Atualizar parametros
    else if (strstr(req, "GET /parametros")) {
        char *total_str = strstr(req, "total=");
        char *intervalo_str = strstr(req, "intervalo=");

        if (total_str) total_amostras = atoi(total_str + 6);
        if (intervalo_str) intervalo_ms = atoi(intervalo_str + 10);

        const char *resp = "Parametros atualizados!";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }
    
    // Pagina nao encontrada
    else {
        const char *resp = "404 - Pagina nao encontrada";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", (int)strlen(resp), resp);
    }

    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) return;
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
}

// ================= FUNCAO PRINCIPAL =================
int main() {
    stdio_init_all();
    
    printf("\n");
    printf("==========================================\n");
    printf("  MONITOR DE VIBRACAO - MOTOR ELETRICO  \n");
    printf("==========================================\n\n");
    
    printf("[INIT] Inicializando sistema...\n");
    
    // Inicializar I2C
    printf("[INIT] Configurando I2C...\n");
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    printf("[OK] I2C inicializado (400 kHz)\n");
    
    // Inicializar MPU6050
    printf("[INIT] Inicializando MPU6050...\n");
    iniciar_i2c_IMU();
    printf("[OK] MPU6050 inicializado\n");
    
    // Inicializar SD Card
    printf("[INIT] Inicializando interface SD Card...\n");
    time_init();
    printf("[OK] Sistema de tempo inicializado\n");
    
    // Inicializar Wi-Fi
    printf("[INIT] Inicializando Wi-Fi...\n");
    if (cyw43_arch_init()) {
        printf("[ERRO] Falha ao inicializar Wi-Fi!\n");
        return 1;
    }
    
    cyw43_arch_enable_sta_mode();
    printf("[INIT] Conectando ao Wi-Fi: %s\n", WIFI_SSID);
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("[ERRO] Falha na conexao Wi-Fi!\n");
        return 1;
    }
    
    printf("[OK] Wi-Fi conectado\n");
    
    // Obter e exibir IP
    uint32_t ip_addr = cyw43_state.netif[0].ip_addr.addr;
    uint8_t *ip = (uint8_t *)&ip_addr;
    
    printf("\n");
    printf("==========================================\n");
    printf("       SISTEMA PRONTO!                   \n");
    printf("==========================================\n");
    printf("  Acesse: http://%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    printf("  Taxa padrao: 100 Hz (10ms)\n");
    printf("  Amostras padrao: 1000\n");
    printf("  Monte o SD Card pela interface web\n");
    printf("==========================================\n\n");
    
    // Iniciar servidor HTTP
    printf("[INIT] Iniciando servidor HTTP na porta 80...\n");
    start_http_server();
    printf("[OK] Servidor HTTP ativo\n\n");
    printf("[INFO] Aguardando requisicoes...\n\n");
    
    // Loop principal
    uint32_t last_print = 0;
    uint32_t tempo_inicio_captura = 0;
    
    while (1) {
        cyw43_arch_poll();
        
        // Ler MPU6050
        ler_acc_giro(accel, gyro);
        
        // Se captura ativa, salvar no SD
        if (captura_ativa) {
            float accel_conv[3], gyro_conv[3];
            converter_valores_mpu(accel_conv, gyro_conv);
            
            uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
            if (amostras_capturadas == 0) {
                tempo_inicio_captura = tempo_atual;
            }
            
            uint32_t tempo_ms = tempo_atual - tempo_inicio_captura;
            salvar_dados_sd(tempo_ms, accel_conv, gyro_conv);
            
            amostras_capturadas++;
            
            // Parar automaticamente ao atingir limite
            if (amostras_capturadas >= total_amostras) {
                f_sync(&arquivo);
                f_close(&arquivo);
                captura_ativa = false;
                printf("Captura concluida automaticamente: %d amostras salvas em %s\n", 
                       amostras_capturadas, nome_arquivo);
            }
            
            sleep_ms(intervalo_ms);
        } else {
            sleep_ms(100);
        }
        
        // Log periodico no console
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_print > 5000) {
            last_print = now;
            
            float accel_conv[3], gyro_conv[3];
            converter_valores_mpu(accel_conv, gyro_conv);
            
            const char* modo_str[] = {"Normal-Axial", "Normal-Radial", 
                                       "FaltaFase-Axial", "FaltaFase-Radial"};
            
            printf("[%lu] SD:%s | %s | A: X=%.2f Y=%.2f Z=%.2f | G: X=%.2f Y=%.2f Z=%.2f\n",
                   now / 1000,
                   sd_montado ? "OK" : "NO",
                   modo_str[modo_operacao],
                   accel_conv[0], accel_conv[1], accel_conv[2],
                   gyro_conv[0], gyro_conv[1], gyro_conv[2]);
            
            if (captura_ativa) {
                printf("    Capturando: %d/%d amostras (%.1f%%)\n", 
                       amostras_capturadas, total_amostras,
                       (float)amostras_capturadas / total_amostras * 100);
            }
        }
    }

    return 0;
}