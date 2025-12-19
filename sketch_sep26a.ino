/*
ESP32 WebServer — L298N (sem PWM) + Ultrassom (HC-SR04)
Interface Scratch COM MOVIMENTOS TEMPORIZADOS
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ===== CONSTANTES GERAIS =====
const char* WIFI_SSID = "IF_CATARINENSE";
const char* WIFI_PASS = "ifcatarinens";
const char* AP_SSID = "ESP32-Carrinho";
const char* AP_PASS = "12345678";

// ===== CONSTANTES DOS PINOS =====
const int A_IN1 = 27;
const int A_IN2 = 32;
const int B_IN1 = 33;
const int B_IN2 = 25;
const int TRIG_PIN = 14;
const int ECHO_PIN = 12;

// ===== CONSTANTES DO ULTRASSOM =====
const float SOUND_SPEED_CM_PER_US = 0.0343f;
const unsigned long PULSE_TIMEOUT = 30000UL;

// ===== CONSTANTES DE MOVIMENTO =====
const String BLOCO_FRENTE = "forward";
const String BLOCO_RE = "back";
const String BLOCO_ESQUERDA = "left";
const String BLOCO_DIREITA = "right";
const String BLOCO_PARAR = "stop";

const String TIPO_MOVIMENTO = "move";
const String TIPO_CURVA = "turn";
const String TIPO_CONTROLE = "control";
const String TIPO_CONDICIONAL = "condition";

// ===== VARIÁVEIS GLOBAIS =====
bool isRunning = false;

// ===== MOVIMENTO =====
void parar() {
  digitalWrite(A_IN1, LOW); digitalWrite(A_IN2, LOW);
  digitalWrite(B_IN1, LOW); digitalWrite(B_IN2, LOW);
}

void frente(int tempo = 300) {  // Reduzido de 500 para 300ms
  digitalWrite(A_IN1, LOW); digitalWrite(A_IN2, HIGH);
  digitalWrite(B_IN1, LOW); digitalWrite(B_IN2, HIGH);
  if (tempo > 0) {
    delay(tempo);
    parar();
  }
}

void re(int tempo = 300) {  // Reduzido de 500 para 300ms
  digitalWrite(A_IN1, HIGH); digitalWrite(A_IN2, LOW);
  digitalWrite(B_IN1, HIGH); digitalWrite(B_IN2, LOW);
  if (tempo > 0) {
    delay(tempo);
    parar();
  }
}

void esquerda(int tempo = 150) {  // Reduzido de 300 para 150ms
  digitalWrite(A_IN1, LOW); digitalWrite(A_IN2, LOW);
  digitalWrite(B_IN1, LOW); digitalWrite(B_IN2, HIGH);
  if (tempo > 0) {
    delay(tempo);
    parar();
  }
}

void direita(int tempo = 150) {  // Reduzido de 300 para 150ms
  digitalWrite(A_IN1, LOW); digitalWrite(A_IN2, HIGH);
  digitalWrite(B_IN1, LOW); digitalWrite(B_IN2, LOW);
  if (tempo > 0) {
    delay(tempo);
    parar();
  }
}

// ===== Leitura do HC-SR04 =====
float readDistanceCm(){
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, PULSE_TIMEOUT);
  if(us == 0) return 9999.0f;
  return (us * SOUND_SPEED_CM_PER_US) / 2.0f;
}

// ===== Web Server =====
WebServer server(80);

const char PAGE_INDEX[] PROGMEM = R"HTML(
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Carrinho ESP32 — Scratch</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { 
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
  background: linear-gradient(135deg, #1e3c72, #2a5298);
  color: white;
  min-height: 100vh;
  padding: 20px;
}

.container {
  display: grid;
  grid-template-columns: 250px 1fr 300px;
  gap: 20px;
  max-width: 1400px;
  margin: 0 auto;
}

.panel {
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(10px);
  border-radius: 15px;
  padding: 20px;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.panel h3 {
  color: #ffd700;
  margin-bottom: 15px;
  text-align: center;
  font-size: 1.2em;
}

.program-area {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 15px;
  padding: 20px;
  min-height: 400px;
  border: 2px dashed rgba(255, 255, 255, 0.3);
}

.block {
  background: #4CAF50;
  color: white;
  padding: 12px 15px;
  margin: 8px 0;
  border-radius: 8px;
  cursor: grab;
  user-select: none;
  border-left: 5px solid #2E7D32;
  box-shadow: 0 2px 5px rgba(0,0,0,0.2);
  transition: all 0.2s;
}

.block:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 8px rgba(0,0,0,0.3);
}

.block.motion { background: #2196F3; border-left-color: #0D47A1; }
.block.control { background: #FF9800; border-left-color: #E65100; }
.block.condition { background: #9C27B0; border-left-color: #6A1B9A; }

.block input, .block select {
  background: rgba(255, 255, 255, 0.9);
  border: none;
  border-radius: 4px;
  padding: 4px 8px;
  margin: 0 5px;
  width: 60px;
  text-align: center;
}

.block select {
  width: 70px;
}

.repeat-content {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 5px;
  padding: 10px;
  margin-top: 10px;
  min-height: 80px;
  border: 2px dashed rgba(255,255,255,0.3);
}

.buttons {
  display: flex;
  gap: 10px;
  margin-top: 20px;
}

button {
  flex: 1;
  padding: 12px;
  border: none;
  border-radius: 8px;
  font-weight: bold;
  cursor: pointer;
  transition: all 0.3s;
  font-size: 1em;
}

.btn-run { background: #4CAF50; color: white; }
.btn-stop { background: #f44336; color: white; }
.btn-clear { background: #ff9800; color: white; }

button:hover {
  transform: scale(1.05);
  opacity: 0.9;
}

.sensor-display {
  text-align: center;
  padding: 15px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 10px;
  margin-top: 15px;
}

.sensor-value {
  font-size: 2em;
  font-weight: bold;
  color: #ffd700;
  margin: 10px 0;
}

.executing {
  animation: pulse 1s infinite;
  box-shadow: 0 0 20px #4CAF50;
}

@keyframes pulse {
  0% { opacity: 1; }
  50% { opacity: 0.7; }
  100% { opacity: 1; }
}

.drop-zone {
  min-height: 50px;
  border: 2px dashed #4CAF50;
  border-radius: 5px;
  margin: 5px 0;
  transition: all 0.3s;
  padding: 10px;
  text-align: center;
  color: rgba(255,255,255,0.6);
}

.drop-zone.active {
  background: rgba(76, 175, 80, 0.2);
  border-color: #4CAF50;
  color: white;
}

.program-block { margin: 5px 0; }

.condition-block {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 5px;
}

.condition-symbol {
  font-weight: bold;
  font-size: 1.1em;
  color: #ffd700;
}

.remove-btn {
  margin-left: auto;
  background: red;
  color: white;
  border: none;
  border-radius: 3px;
  padding: 2px 6px;
  cursor: pointer;
}
</style>
</head>
<body>
<div class="container">
  <!-- Painel de Blocos -->
  <div class="panel">
    <h3>🏃 Blocos de Movimento</h3>
    <div class="block motion" data-type="move" data-action="forward" draggable="true">
      ▶️ Andar para Frente <input type="number" value="0.3" min="0.1" max="10" step="0.1"> segundos
    </div>
    <div class="block motion" data-type="move" data-action="back" draggable="true">
      ◀️ Andar para Trás <input type="number" value="0.3" min="0.1" max="10" step="0.1"> segundos
    </div>
    <div class="block motion" data-type="turn" data-action="left" draggable="true">
      ↪️ Virar Esquerda <input type="number" value="0.15" min="0.1" max="10" step="0.1"> segundos
    </div>
    <div class="block motion" data-type="turn" data-action="right" draggable="true">
      ↩️ Virar Direita <input type="number" value="0.15" min="0.1" max="10" step="0.1"> segundos
    </div>
    <div class="block motion" data-type="stop" data-action="stop" draggable="true">
      ⏹️ Parar Movimento
    </div>
    
    <h3>🔄 Blocos de Controle</h3>
    <div class="block control" data-type="repeat" draggable="true">
      🔁 Repetir <input type="number" value="3" min="1" max="10"> vezes
    </div>
    <div class="block condition" data-type="if" draggable="true">
      ❓ Se 
      <select class="condition-variable">
        <option value="distance">distância</option>
      </select>
      <select class="condition-operator">
        <option value="<">&lt;</option>
        <option value=">">&gt;</option>
        <option value="<=">&lt;=</option>
        <option value=">=">&gt;=</option>
        <option value="==">==</option>
      </select>
      <input type="number" class="condition-value" value="20" min="1" max="100"> cm
    </div>
    <div class="block condition" data-type="while" draggable="true">
      🔁 Enquanto 
      <select class="condition-variable">
        <option value="distance">distância</option>
      </select>
      <select class="condition-operator">
        <option value="<">&lt;</option>
        <option value=">">&gt;</option>
        <option value="<=">&lt;=</option>
        <option value=">=">&gt;=</option>
        <option value="==">==</option>
      </select>
      <input type="number" class="condition-value" value="30" min="1" max="100"> cm
    </div>
    <div class="block control" data-type="wait" draggable="true">
      ⏱️ Esperar <input type="number" value="0.5" min="0.1" max="10" step="0.1"> segundos
    </div>
  </div>

  <!-- Área de Programação -->
  <div class="panel">
    <h3>🧩 Área de Programação</h3>
    <div class="program-area" id="programArea">
      <div class="drop-zone" id="startZone">
        Arraste os blocos para aqui ➡️
      </div>
    </div>
    
    <div class="buttons">
      <button class="btn-run" onclick="runProgram()">▶️ Executar</button>
      <button class="btn-stop" onclick="stopProgram()">⏹️ Parar Tudo</button>
      <button class="btn-clear" onclick="clearProgram()">🗑️ Limpar</button>
    </div>
  </div>

  <!-- Painel de Informações -->
  <div class="panel">
    <h3>📊 Informações</h3>
    <div class="sensor-display">
      <div>📏 Distância do Sensor</div>
      <div class="sensor-value" id="distanceValue">-- cm</div>
      <div>Ultrassom HC-SR04</div>
    </div>
  </div>
</div>

<script>
let currentDropTarget = null;
let isRunning = false;

// Configurar blocos arrastáveis do painel
document.querySelectorAll('.panel .block').forEach(block => {
  block.addEventListener('dragstart', e => {
    const type = block.dataset.type;
    const action = block.dataset.action || '';
    
    // Para blocos condicionais, capturar todos os valores atuais
    if (type === 'if' || type === 'while') {
      const variableSelect = block.querySelector('.condition-variable');
      const operatorSelect = block.querySelector('.condition-operator');
      const valueInput = block.querySelector('.condition-value');
      
      const data = {
        type: type,
        variable: variableSelect.value,
        operator: operatorSelect.value,
        value: valueInput.value
      };
      e.dataTransfer.setData('text/plain', JSON.stringify(data));
    } else {
      e.dataTransfer.setData('text/plain', type + ':' + action);
    }
    
    block.classList.add('dragging');
  });
  
  block.addEventListener('dragend', () => block.classList.remove('dragging'));
});

// Sistema de drop
document.addEventListener('dragover', e => {
  e.preventDefault();
  let target = e.target;
  while (target && !target.classList.contains('drop-zone') && target.id !== 'programArea') {
    target = target.parentElement;
  }
  if (target) {
    currentDropTarget = target;
    target.classList.add('active');
  }
});

document.addEventListener('dragleave', e => {
  if (currentDropTarget && !currentDropTarget.contains(e.relatedTarget)) {
    currentDropTarget.classList.remove('active');
    currentDropTarget = null;
  }
});

document.addEventListener('drop', e => {
  e.preventDefault();
  if (!currentDropTarget) return;
  currentDropTarget.classList.remove('active');
  
  const data = e.dataTransfer.getData('text/plain');
  let newBlock;
  
  try {
    // Tentar parsear como JSON (blocos condicionais)
    const jsonData = JSON.parse(data);
    newBlock = createConditionBlock(jsonData.type, jsonData.variable, jsonData.operator, jsonData.value);
  } catch {
    // Se não for JSON, usar o formato antigo
    const [type, action] = data.split(':');
    newBlock = createBlock(type, action);
  }
  
  if (currentDropTarget.id === 'programArea') {
    const startZone = document.getElementById('startZone');
    if (startZone) startZone.remove();
    currentDropTarget.appendChild(newBlock);
  } else if (currentDropTarget.classList.contains('repeat-content')) {
    currentDropTarget.appendChild(newBlock);
  }
  currentDropTarget = null;
});

// Criar bloco normal
function createBlock(type, action) {
  const block = document.createElement('div');
  block.className = `block ${type} program-block`;
  block.dataset.type = type;
  if (action) block.dataset.action = action;

  let content = '';
  if (action === 'forward') content = `▶️ Andar para Frente <input type="number" value="0.3" step="0.1"> segundos`;
  else if (action === 'back') content = `◀️ Andar para Trás <input type="number" value="0.3" step="0.1"> segundos`;
  else if (action === 'left') content = `↪️ Virar Esquerda <input type="number" value="0.15" step="0.1"> segundos`;
  else if (action === 'right') content = `↩️ Virar Direita <input type="number" value="0.15" step="0.1"> segundos`;
  else if (action === 'stop') content = `⏹️ Parar Movimento`;
  else if (type === 'repeat') content = `🔁 Repetir <input type="number" value="3"> vezes`;
  else if (type === 'wait') content = `⏱️ Esperar <input type="number" value="0.5" step="0.1"> segundos`;

  block.innerHTML = content + `<button class="remove-btn" onclick="removeBlock(this.parentElement)">X</button>`;

  if (['repeat'].includes(type)) {
    const inner = document.createElement('div');
    inner.className = 'repeat-content drop-zone';
    inner.innerHTML = 'Arraste blocos de movimento aqui';
    block.appendChild(inner);
  }

  return block;
}

// Criar bloco condicional (if/while)
function createConditionBlock(type, variable, operator, value) {
  const block = document.createElement('div');
  block.className = `block condition program-block`;
  block.dataset.type = type;
  
  const symbol = type === 'if' ? '❓' : '🔁';
  const text = type === 'if' ? 'Se' : 'Enquanto';

  block.innerHTML = `
    <div class="condition-block">
      ${symbol} ${text} 
      <select class="condition-variable">
        <option value="distance" ${variable === 'distance' ? 'selected' : ''}>distância</option>
      </select>
      <select class="condition-operator">
        <option value="<" ${operator === '<' ? 'selected' : ''}>&lt;</option>
        <option value=">" ${operator === '>' ? 'selected' : ''}>&gt;</option>
        <option value="<=" ${operator === '<=' ? 'selected' : ''}>&le;</option>
        <option value=">=" ${operator === '>=' ? 'selected' : ''}>&ge;</option>
        <option value="==" ${operator === '==' ? 'selected' : ''}>=</option>
      </select>
      <input type="number" class="condition-value" value="${value}" min="1" max="100"> cm
      <button class="remove-btn" onclick="removeBlock(this.parentElement.parentElement)">X</button>
    </div>
    <div class="repeat-content drop-zone">
      Arraste blocos para executar aqui
    </div>
  `;

  return block;
}

function removeBlock(block) {
  if (confirm('Remover este bloco?')) block.remove();
}

// Função para obter os valores ATUAIS do bloco condicional
function getConditionValues(block) {
  const variableSelect = block.querySelector('.condition-variable');
  const operatorSelect = block.querySelector('.condition-operator');
  const valueInput = block.querySelector('.condition-value');
  
  return {
    variable: variableSelect ? variableSelect.value : 'distance',
    operator: operatorSelect ? operatorSelect.value : '<',
    value: valueInput ? parseFloat(valueInput.value) : 20
  };
}

async function executeBlock(block) {
  if (!isRunning) return;
  
  const type = block.dataset.type;
  const action = block.dataset.action;

  block.classList.add('executing');

  try {
    if (type === 'move' || type === 'turn') {
      const timeInput = block.querySelector('input[type="number"]');
      const timeSeconds = timeInput ? parseFloat(timeInput.value) : 0.3;
      const timeMilliseconds = Math.round(timeSeconds * 1000);
      await fetch(`/api/motors?action=${action}&time=${timeMilliseconds}`);
      await new Promise(r => setTimeout(r, 300));
    } 
    else if (action === 'stop') {
      await fetch(`/api/motors?action=stop`);
    }
    else if (type === 'repeat') {
      const timesInput = block.querySelector('input[type="number"]');
      const times = timesInput ? parseInt(timesInput.value) : 3;
      const repeatContent = block.querySelector('.repeat-content');
      
      for (let i = 0; i < times && isRunning; i++) {
        const innerBlocks = repeatContent.querySelectorAll('.program-block');
        for (const innerBlock of innerBlocks) {
          if (!isRunning) break;
          await executeBlock(innerBlock);
        }
      }
    }
    else if (type === 'if') {
      const condition = getConditionValues(block);
      const inner = block.querySelector('.repeat-content');
      
      let conditionMet = false;
      
      if (condition.variable === 'distance') {
        const dist = await getDistance();
        conditionMet = evaluateCondition(dist, condition.operator, condition.value);
      }
      
      if (conditionMet) {
        const innerBlocks = inner.querySelectorAll('.program-block');
        for (const innerBlock of innerBlocks) {
          if (!isRunning) break;
          await executeBlock(innerBlock);
        }
      }
    }
    else if (type === 'while') {
      const inner = block.querySelector('.repeat-content');
      
      while (isRunning) {
        const cond = getConditionValues(block);
        const dist = await getDistance();
        const ok = evaluateCondition(dist, cond.operator, cond.value);
        
        if (!ok) break;
        
        const innerBlocks = inner.querySelectorAll('.program-block');
        for (const b of innerBlocks) {
          if (!isRunning) break;
          await executeBlock(b);
        }
        
        // Pequena pausa para não sobrecarregar
        await new Promise(r => setTimeout(r, 150));
      }
    }
    else if (type === 'wait') {
      const secondsInput = block.querySelector('input[type="number"]');
      const seconds = secondsInput ? parseFloat(secondsInput.value) : 0.5;
      await new Promise(r => setTimeout(r, seconds * 1000));
    }
  } catch (err) { 
    console.error('Erro executando bloco:', err); 
  }

  block.classList.remove('executing');
}

function evaluateCondition(value, operator, compareTo) {
  switch (operator) {
    case '<': return value < compareTo;
    case '>': return value > compareTo;
    case '<=': return value <= compareTo;
    case '>=': return value >= compareTo;
    case '==': return Math.abs(value - compareTo) < 0.1;
    default: return false;
  }
}

async function getDistance() {
  try {
    const res = await fetch('/api/dist');
    const data = await res.json();
    return data.cm;
  } catch { 
    console.error('Erro obtendo distância');
    return 9999; 
  }
}

async function runProgram() {
  if (isRunning) return;
  
  const blocks = document.querySelectorAll('#programArea .program-block');
  if (!blocks.length) return alert('Arraste alguns blocos primeiro!');
  
  isRunning = true;
  const runButton = document.querySelector('.btn-run');
  runButton.disabled = true;
  runButton.textContent = '⏸️ Executando...';
  
  try {
    const mainBlocks = document.querySelectorAll('#programArea > .program-block');
    
    for (const block of mainBlocks) {
      if (!isRunning) break;
      await executeBlock(block);
    }
  } catch (error) {
    console.error('Erro executando programa:', error);
  } finally {
    isRunning = false;
    runButton.disabled = false;
    runButton.textContent = '▶️ Executar';
  }
}

function stopProgram() {
  isRunning = false;
  fetch('/api/motors?action=stop');
  document.querySelectorAll('.executing').forEach(el => {
    el.classList.remove('executing');
  });
  const runButton = document.querySelector('.btn-run');
  runButton.disabled = false;
  runButton.textContent = '▶️ Executar';
}

function clearProgram() {
  if (confirm('Limpar todo o programa?')) {
    document.getElementById('programArea').innerHTML = '<div class="drop-zone" id="startZone">Arraste os blocos para aqui ➡️</div>';
  }
}

async function updateSensor() {
  try {
    const res = await fetch('/api/dist');
    const d = await res.json();
    document.getElementById('distanceValue').textContent = d.cm.toFixed(1) + ' cm';
  } catch (error) {
    console.error('Erro atualizando sensor:', error);
  }
}

setInterval(updateSensor, 500);
updateSensor();
</script>
</body>
</html>
)HTML";

void handleRoot(){ server.send_P(200,"text/html; charset=utf-8",PAGE_INDEX); }

void handleMotors(){
  String action = server.hasArg("action") ? server.arg("action") : "";
  int tempo = server.hasArg("time") ? server.arg("time").toInt() : 300;
  
  action.toLowerCase();
  
  if(action == BLOCO_FRENTE) frente(tempo);
  else if(action == BLOCO_RE) re(tempo);
  else if(action == BLOCO_ESQUERDA) esquerda(tempo);
  else if(action == BLOCO_DIREITA) direita(tempo);
  else if(action == BLOCO_PARAR) parar();
  else { server.send(400,"text/plain","acao invalida"); return; }
  
  server.send(200,"text/plain","ok");
}

void handleDist(){
  float d = readDistanceCm();
  String j = "{\"cm\":" + String(d,1) + ",\"ts\":" + String(millis()) + "}";
  server.send(200, "application/json", j);
}

void handleNotFound(){ server.send(404,"text/plain; charset=utf-8","Nao encontrado"); }

bool startWiFiSTA(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  uint32_t t0=millis();
  while(WiFi.status()!=WL_CONNECTED && (millis()-t0)<15000) delay(200);
  return WiFi.status()==WL_CONNECTED;
}

void startWiFiAP(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASS);
}

void setup(){
  pinMode(A_IN1, OUTPUT); pinMode(A_IN2, OUTPUT);
  pinMode(B_IN1, OUTPUT); pinMode(B_IN2, OUTPUT);
  parar();

  pinMode(TRIG_PIN, OUTPUT); digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT); 

  Serial.begin(115200); delay(200);

  if(startWiFiSTA()){
    Serial.print("[WiFi] Conectado em "); Serial.println(WIFI_SSID);
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    MDNS.begin("carrinho");
  }else{
    Serial.println("[WiFi] STA falhou -> AP");
    startWiFiAP();
    Serial.print("AP SSID: "); Serial.println(AP_SSID);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    MDNS.begin("carrinho");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/motors", HTTP_GET, handleMotors);
  server.on("/api/dist", HTTP_GET, handleDist);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[HTTP] Server iniciado :80");
}

void loop(){
  server.handleClient();
}