/*
 * ╔══════════════════════════════════════════════════════════╗
 * ║       ESP32 GAME CONSOLE  —  by JAGP Y DFTZ ❤            ║
 * ║   Menú con animaciones + Snake + Space Invaders + Tetris ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * Pines:
 *   TFT_CS=5  TFT_DC=2  TFT_RST=4
 *   UP=32  DOWN=33  LEFT=25  RIGHT=26
 *   A=27   B=14     START=12  SELECT=13
 *   LED_GREEN=16  LED_RED=17
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ═══════════════════════════════════════════════════════════
//  HARDWARE
// ═══════════════════════════════════════════════════════════
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_LEFT    25
#define BTN_RIGHT   26
#define BTN_A       27
#define BTN_B       14
#define BTN_START   12
#define BTN_SELECT  13
#define LED_GREEN   16
#define LED_RED     17

#define SCREEN_W  128
#define SCREEN_H  160

// Colores extra
#define COLOR_GOLD    0xFEA0
#define COLOR_PURPLE  0x780F
#define COLOR_ORANGE  0xFC60
#define COLOR_TEAL    0x07FF
#define COLOR_PINK    0xF81F
#define COLOR_DARK    0x18C3

// ═══════════════════════════════════════════════════════════
//  ESTADO GLOBAL DEL MENÚ
// ═══════════════════════════════════════════════════════════
int menuSel = 0;            // 0=Snake 1=SpaceInvaders 2=Tetris
bool inMenu  = true;

unsigned long lastMenuBtn = 0;
#define MENU_DEBOUNCE 200
bool menuNeedsFullRedraw = true;

// ═══════════════════════════════════════════════════════════
//  HELPERS COMUNES
// ═══════════════════════════════════════════════════════════
bool btnPressed(int pin) { return !digitalRead(pin); }

void ledsOff() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

// ═══════════════════════════════════════════════════════════
//  MENÚ  —  Animaciones y dibujo
// ═══════════════════════════════════════════════════════════

// Imprime texto centrado horizontalmente en la pantalla
void printCentered(const char* text, int y, int textSize = 1) {
  int charW = 6 * textSize;
  int totalW = strlen(text) * charW;
  int x = (SCREEN_W - totalW) / 2;
  if (x < 0) x = 0;
  tft.setTextSize(textSize);
  tft.setCursor(x, y);
  tft.print(text);
}

// Dibuja una estrella pequeña
void drawStar(int x, int y, uint16_t c) {
  tft.drawPixel(x, y, c);
  tft.drawPixel(x+1, y, c);
  tft.drawPixel(x, y+1, c);
}

// Dibuja fondo estrellado estático
void drawStarField() {
  uint16_t stars[] = {
    0x0205,0x0F10,0x1A07,0x250B,0x3012,0x3B04,0x460A,0x5115,
    0x5C03,0x670E,0x7209,0x7D13,0x8806,0x9311,0x9E01,0xA908,
    0xB40C,0xBF16,0xCA05,0xD50F,0xE002,0xEB0A,0xF607,0xFF10
  };
  for (int i = 0; i < 24; i++) {
    int sx = (stars[i] >> 8) & 0xFF;
    int sy = stars[i] & 0xFF;
    tft.drawPixel(sx % SCREEN_W, sy % SCREEN_H, ST77XX_WHITE);
  }
}

// Dibuja la nave del jugador (Space Invaders) como icono
void drawIconShip(int x, int y, uint16_t c) {
  tft.fillRect(x+3, y,   2, 2, c);
  tft.fillRect(x+1, y+2, 6, 2, c);
  tft.fillRect(x,   y+4, 8, 2, c);
}

// Dibuja un bloque de Tetris (2 celdas)
void drawIconTetris(int x, int y, uint16_t c) {
  tft.fillRect(x,   y,   6, 6, c);
  tft.fillRect(x+7, y,   6, 6, c);
  tft.fillRect(x+7, y+7, 6, 6, c);
  tft.fillRect(x+14,y+7, 6, 6, 0xFC00);
}

// Dibuja una serpiente pequeña
void drawIconSnake(int x, int y, uint16_t c) {
  tft.fillRect(x,    y+2, 4, 4, c);
  tft.fillRect(x+4,  y+2, 4, 4, c);
  tft.fillRect(x+8,  y+2, 4, 4, c);
  tft.fillRect(x+8,  y+6, 4, 4, c);
  tft.fillRect(x+8,  y+10,4, 4, c);
  // Ojo
  tft.drawPixel(x+1, y+3, ST77XX_BLACK);
}

// Dibuja un invader pequeño
void drawIconInvader(int x, int y, uint16_t c) {
  tft.fillRect(x+2, y,   6, 2, c);
  tft.fillRect(x,   y+2, 10,3, c);
  tft.fillRect(x+1, y+5, 3, 2, c);
  tft.fillRect(x+6, y+5, 3, 2, c);
  // Ojos
  tft.drawPixel(x+2, y+3, ST77XX_BLACK);
  tft.drawPixel(x+7, y+3, ST77XX_BLACK);
}

// Barra de selección con gradiente simulado
void drawSelectionBar(int y, int gameIdx) {
  uint16_t cols[] = {0x07E0, 0xF800, 0x07FF};
  uint16_t c = cols[gameIdx];
  tft.fillRoundRect(4, y-2, SCREEN_W-8, 26, 4, COLOR_DARK);
  tft.drawRoundRect(4, y-2, SCREEN_W-8, 26, 4, c);
  tft.drawRoundRect(5, y-1, SCREEN_W-10, 24, 3, c);
}

// Triángulo selector
void drawArrow(int y, uint16_t c) {
  tft.fillTriangle(SCREEN_W-10, y+4, SCREEN_W-10, y+18, SCREEN_W-4, y+11, c);
}

const char* gameNames[]  = {" SNAKE",  " SPACE INVADERS", " TETRIS"};
const char* gameDesc[]   = {"Come&crece", "Invasores", "Encaja piezas"};
uint16_t    gameColors[] = {0x07E0, 0xF800, 0x07FF};

// Dibuja una sola fila del menú (icono + nombre + descripción + barra si está seleccionada)
void drawMenuRow(int i, bool selected) {
  int yStart = 28;
  int yStep  = 29;
  int yy = yStart + i * yStep;

  // Borra el área de la fila antes de redibujar
  tft.fillRect(0, yy-2, SCREEN_W, 28, ST77XX_BLACK);

  if (selected) {
    drawSelectionBar(yy, i);
    drawArrow(yy, gameColors[i]);
  }

  // Icono
  uint16_t iconColor = selected ? ST77XX_YELLOW : gameColors[i];
  if (i == 0) drawIconSnake(10, yy+2, iconColor);
  if (i == 1) drawIconInvader(10, yy+2, iconColor);
  if (i == 2) drawIconTetris(10, yy+2, iconColor);

  // Nombre
  tft.setTextSize(1);
  tft.setTextColor(selected ? gameColors[i] : ST77XX_WHITE);
  tft.setCursor(36, yy+2);
  tft.print(gameNames[i] + 2);

  // Descripción
  tft.setTextColor(selected ? ST77XX_WHITE : 0x8410);
  tft.setCursor(36, yy+13);
  tft.print(gameDesc[i]);
}

void drawMenuFrame(int sel, int animFrame) {
  static int prevSel = -1;
  bool fullRedraw = menuNeedsFullRedraw || (prevSel == -1);
  if (fullRedraw) menuNeedsFullRedraw = false;

  if (fullRedraw) {
    tft.fillScreen(ST77XX_BLACK);

    // Fondo — líneas sutiles
    for (int i = 0; i < SCREEN_H; i += 16) {
      tft.drawFastHLine(0, i, SCREEN_W, COLOR_DARK);
    }
    drawStarField();

    // ── Título ──────────────────────────────────────────────
    tft.setTextColor(COLOR_PURPLE);
    printCentered("GAME HUB", 6, 2);
    uint16_t titleColor = (animFrame % 2 == 0) ? COLOR_GOLD : ST77XX_YELLOW;
    tft.setTextColor(titleColor);
    printCentered("GAME HUB", 5, 2);

    // Línea decorativa
    tft.drawFastHLine(0, 22, SCREEN_W, COLOR_GOLD);
    tft.drawFastHLine(0, 23, SCREEN_W, COLOR_DARK);

    // Dibuja las 3 filas completas
    for (int i = 0; i < 3; i++) drawMenuRow(i, i == sel);

    // ── Instrucciones ───────────────────────────────────────
    tft.drawFastHLine(0, SCREEN_H-12, SCREEN_W, COLOR_DARK);
    tft.setTextColor(0x8410);
    if (animFrame % 4 < 2) {
      printCentered("UP/DN:Mover  START:Jugar", SCREEN_H-10);
    } else {
      printCentered("SELECT:Volver al menu", SCREEN_H-10);
    }

  } else {
    // Solo redibuja el título (destello) y las filas que cambiaron
    uint16_t titleColor = (animFrame % 2 == 0) ? COLOR_GOLD : ST77XX_YELLOW;
    tft.setTextColor(COLOR_PURPLE);
    printCentered("GAME HUB", 6, 2);
    tft.setTextColor(titleColor);
    printCentered("GAME HUB", 5, 2);

    if (sel != prevSel) {
      drawMenuRow(prevSel, false);
      drawMenuRow(sel, true);
    }

    // Instrucciones (solo si cambia el frame par/impar)
    tft.fillRect(0, SCREEN_H-11, SCREEN_W, 11, ST77XX_BLACK);
    tft.setTextColor(0x8410);
    if (animFrame % 4 < 2) {
      printCentered("UP/DN:Mover  START:Jugar", SCREEN_H-10);
    } else {
      printCentered("SELECT:Volver al menu", SCREEN_H-10);
    }
  }

  prevSel = sel;
}

// Animación de entrada (barrido)
void menuIntroAnim() {
  for (int x = SCREEN_W; x >= 0; x -= 8) {
    tft.fillScreen(ST77XX_BLACK);
    // Barra de barrido brillante
    for (int w = 0; w < 8; w++) {
      uint8_t brightness = (w < 4) ? 255 : 128;
      tft.drawFastVLine(x + w, 0, SCREEN_H, (w == 0) ? COLOR_GOLD : COLOR_PURPLE);
    }
    delay(15);
  }
  // Dibuja el menú inicial
  drawMenuFrame(menuSel, 0);
}

// Animación de transición al salir al menú
void menuReturnAnim() {
  tft.fillScreen(ST77XX_BLACK);
  ledsOff();
  drawMenuFrame(menuSel, 0);
}
// Animación de inicio de juego (flash + zoom)
void gameStartAnim(int gameIdx) {
  uint16_t c = gameColors[gameIdx];
  for (int i = 0; i < 4; i++) {
    tft.fillScreen(c);  delay(60);
    tft.fillScreen(ST77XX_BLACK); delay(60);
  }
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(c);
  printCentered(gameNames[gameIdx] + 2, 50, 2);
  delay(600);
  tft.fillScreen(ST77XX_BLACK);
}

// ═══════════════════════════════════════════════════════════
//  SNAKE (Gusano)
// ═══════════════════════════════════════════════════════════
#define CELL_SIZE  8
#define GRID_W     (SCREEN_W / CELL_SIZE)
#define GRID_H     (SCREEN_H / CELL_SIZE)

struct Segment { int x; int y; };

Segment snake[100];
int snakeLength;
int dirX, dirY, nextDirX, nextDirY;
int foodX, foodY;
bool snakeGameOver;
unsigned long lastSnakeMove = 0;
int snakeMoveDelay = 120;

void snakeSpawnFood() {
  bool valid = false;
  while (!valid) {
    valid = true;
    foodX = random(GRID_W);
    foodY = random(GRID_H);
    for (int i = 0; i < snakeLength; i++)
      if (snake[i].x == foodX && snake[i].y == foodY) { valid = false; break; }
  }
}

void snakeReset() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  snakeLength = 3;
  snake[0] = {10, 8}; snake[1] = {9, 8}; snake[2] = {8, 8};
  dirX = 1; dirY = 0; nextDirX = 1; nextDirY = 0;
  snakeGameOver = false;
  snakeSpawnFood();
  tft.fillScreen(ST77XX_BLACK);
  // Marco decorativo
  tft.drawRect(0, 0, SCREEN_W, SCREEN_H, 0x07E0);
}

void snakeDrawCell(int x, int y, uint16_t color) {
  tft.fillRect(x * CELL_SIZE + 1, y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, color);
}

void snakeDraw() {
  // Comida con brillo
  tft.fillRect(foodX * CELL_SIZE + 1, foodY * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, ST77XX_RED);
  tft.drawPixel(foodX * CELL_SIZE + 2, foodY * CELL_SIZE + 2, ST77XX_WHITE);

  for (int i = 0; i < snakeLength; i++) {
    if (i == 0) {
      // Cabeza con detalles
      tft.fillRect(snake[i].x * CELL_SIZE + 1, snake[i].y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, ST77XX_YELLOW);
      tft.drawPixel(snake[i].x * CELL_SIZE + 2, snake[i].y * CELL_SIZE + 2, ST77XX_BLACK);
      tft.drawPixel(snake[i].x * CELL_SIZE + 5, snake[i].y * CELL_SIZE + 2, ST77XX_BLACK);
    } else {
      // Cuerpo con gradiente: más verde cerca de la cabeza
      uint8_t g = max(3, 6 - i / 5);
      snakeDrawCell(snake[i].x, snake[i].y, tft.color565(0, g * 40, 0));
    }
  }

  // HUD
  tft.fillRect(0, 0, 80, 8, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD);
  tft.setCursor(2, 1);
  tft.print("Score:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(snakeLength - 3);
}

void snakeReadButtons() {
  if (btnPressed(BTN_UP)    && dirY == 0) { nextDirX = 0;  nextDirY = -1; }
  if (btnPressed(BTN_DOWN)  && dirY == 0) { nextDirX = 0;  nextDirY =  1; }
  if (btnPressed(BTN_LEFT)  && dirX == 0) { nextDirX = -1; nextDirY =  0; }
  if (btnPressed(BTN_RIGHT) && dirX == 0) { nextDirX =  1; nextDirY =  0; }
}

void snakeMove() {
  dirX = nextDirX; dirY = nextDirY;
  Segment newHead = snake[0];
  newHead.x += dirX; newHead.y += dirY;

  if (newHead.x < 0 || newHead.x >= GRID_W || newHead.y < 0 || newHead.y >= GRID_H) {
    snakeGameOver = true; return;
  }
  for (int i = 0; i < snakeLength; i++)
    if (newHead.x == snake[i].x && newHead.y == snake[i].y) { snakeGameOver = true; return; }

  bool ate = (newHead.x == foodX && newHead.y == foodY);
  if (ate) {
    for (int i = snakeLength; i > 0; i--) snake[i] = snake[i-1];
    snake[0] = newHead;
    if (snakeLength < 99) snakeLength++;
    snakeSpawnFood();
    if (snakeMoveDelay > 50) snakeMoveDelay -= 2;
    // Flash LED
    digitalWrite(LED_GREEN, LOW); delay(50); digitalWrite(LED_GREEN, HIGH);
  } else {
    snakeDrawCell(snake[snakeLength-1].x, snake[snakeLength-1].y, ST77XX_BLACK);
    for (int i = snakeLength-1; i > 0; i--) snake[i] = snake[i-1];
    snake[0] = newHead;
  }
}

void snakeShowGameOver() {
  digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
  tft.fillScreen(ST77XX_BLACK);
  // Borde rojo
  tft.drawRect(2, 2, SCREEN_W-4, SCREEN_H-4, ST77XX_RED);
  tft.setTextColor(ST77XX_RED); tft.setTextSize(2);
  tft.setCursor(35, 20); tft.print("GAME");
  tft.setCursor(35, 42); tft.print("OVER");
  tft.setTextSize(1); tft.setTextColor(COLOR_GOLD);
  tft.setCursor(20, 70); tft.print("Puntaje: "); tft.print(snakeLength - 3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 90);  tft.print("START  = Reiniciar");
  tft.setCursor(15, 103); tft.print("SELECT = Menu");
}

void playSnake() {
  gameStartAnim(0);
  snakeMoveDelay = 120;
  snakeReset();

  while (true) {
    // SELECT = volver al menú
    if (btnPressed(BTN_SELECT)) { delay(300); return; }

    if (snakeGameOver) {
      snakeShowGameOver();
      while (true) {
        if (btnPressed(BTN_START))  { delay(300); snakeMoveDelay = 120; snakeReset(); break; }
        if (btnPressed(BTN_SELECT)) { delay(300); return; }
        delay(10);
      }
      continue;
    }

    snakeReadButtons();
    if (millis() - lastSnakeMove >= (unsigned long)snakeMoveDelay) {
      lastSnakeMove = millis();
      snakeMove();
      snakeDraw();
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  SPACE INVADERS
// ═══════════════════════════════════════════════════════════
#define PLAYER_W       10
#define PLAYER_H        6
#define PLAYER_Y       (SCREEN_H - 14)
#define PLAYER_SPEED    3
#define BULLET_W        2
#define BULLET_H        5
#define BULLET_SPEED    6
#define ENEMY_COLS      8
#define ENEMY_ROWS      3
#define ENEMY_W         10
#define ENEMY_H         7
#define ENEMY_PAD_X     4
#define ENEMY_PAD_Y     4
#define ENEMY_START_Y   10
#define ENEMY_BULLET_W  2
#define ENEMY_BULLET_H  5
#define ENEMY_BULLET_SPEED 3
#define MAX_ENEMY_BULLETS  3

struct Bullet { int x, y; bool active; };

int      siPlayerX;
Bullet   siPlayerBullet;
bool     siEnemies[ENEMY_ROWS][ENEMY_COLS];
int      siEnemyOffsetX, siEnemyOffsetY, siEnemyDirX;
int      siEnemyMoveTimer, siEnemyMoveDelay, siEnemiesAlive;
Bullet   siEnemyBullets[MAX_ENEMY_BULLETS];
int      siScore, siLives;
bool     siGameOver, siGameWon;

unsigned long siLastLeft = 0, siLastRight = 0, siLastShoot = 0;
#define SI_DEBOUNCE_MOVE  80
#define SI_DEBOUNCE_SHOOT 300

void siDrawPlayer(int x, uint16_t c) {
  tft.fillRect(x, PLAYER_Y + 2, PLAYER_W, PLAYER_H - 2, c);
  tft.fillRect(x + PLAYER_W/2 - 1, PLAYER_Y, 2, 3, c);
  // Motor
  tft.drawPixel(x, PLAYER_Y + PLAYER_H, COLOR_ORANGE);
  tft.drawPixel(x + PLAYER_W-1, PLAYER_Y + PLAYER_H, COLOR_ORANGE);
}

void siDrawEnemy(int px, int py, int row, uint16_t color) {
  if (color == ST77XX_BLACK) {
    tft.fillRect(px-1, py-1, ENEMY_W+2, ENEMY_H+2, ST77XX_BLACK); return;
  }
  uint16_t c = (row == 0) ? ST77XX_RED : (row == 1) ? COLOR_ORANGE : 0x07E0;
  tft.fillRect(px+2, py,   6, 2, c);
  tft.fillRect(px,   py+2, ENEMY_W, 3, c);
  tft.fillRect(px+1, py+5, 3, 2, c);
  tft.fillRect(px+6, py+5, 3, 2, c);
}

void siDrawHUD() {
  tft.fillRect(0, SCREEN_H-10, SCREEN_W, 10, ST77XX_BLACK);
  tft.drawFastHLine(0, SCREEN_H-11, SCREEN_W, COLOR_TEAL);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD); tft.setCursor(2, SCREEN_H-9);
  tft.print("Score:"); tft.setTextColor(ST77XX_WHITE); tft.print(siScore);
  tft.setTextColor(COLOR_PINK); tft.setCursor(95, SCREEN_H-9);
  tft.print("Vidas:"); tft.setTextColor(ST77XX_WHITE); tft.print(siLives);
}

void siRedrawAllEnemies() {
  for (int r = 0; r < ENEMY_ROWS; r++)
    for (int c = 0; c < ENEMY_COLS; c++) {
      int px = siEnemyOffsetX + c * (ENEMY_W + ENEMY_PAD_X);
      int py = siEnemyOffsetY + r * (ENEMY_H + ENEMY_PAD_Y);
      if (siEnemies[r][c]) siDrawEnemy(px, py, r, 0xFFFF);
      else tft.fillRect(px-1, py-1, ENEMY_W+2, ENEMY_H+2, ST77XX_BLACK);
    }
}

void siReset() {
  siPlayerX = SCREEN_W/2 - PLAYER_W/2;
  siPlayerBullet.active = false;
  siEnemyOffsetX = 5; siEnemyOffsetY = ENEMY_START_Y;
  siEnemyDirX = 1; siEnemyMoveDelay = 400;
  siEnemyMoveTimer = 0; siEnemiesAlive = ENEMY_ROWS * ENEMY_COLS;
  for (int r = 0; r < ENEMY_ROWS; r++)
    for (int c = 0; c < ENEMY_COLS; c++) siEnemies[r][c] = true;
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) siEnemyBullets[i].active = false;
  siScore = 0; siLives = 3; siGameOver = false; siGameWon = false;
  tft.fillScreen(ST77XX_BLACK);
  // Estrellas de fondo
  for (int i = 0; i < 20; i++)
    tft.drawPixel(random(SCREEN_W), random(SCREEN_H - 15), ST77XX_WHITE);
  siRedrawAllEnemies();
  siDrawPlayer(siPlayerX, COLOR_TEAL);
  siDrawHUD();
  digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, HIGH);
}

void siEnemyShoot() {
  int slot = -1;
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) if (!siEnemyBullets[i].active) { slot = i; break; }
  if (slot == -1) return;
  int attempts = 0;
  while (attempts < 20) {
    int col = random(ENEMY_COLS);
    for (int r = ENEMY_ROWS-1; r >= 0; r--) {
      if (siEnemies[r][col]) {
        int px = siEnemyOffsetX + col * (ENEMY_W + ENEMY_PAD_X);
        int py = siEnemyOffsetY + r  * (ENEMY_H + ENEMY_PAD_Y);
        siEnemyBullets[slot] = {px + ENEMY_W/2 - 1, py + ENEMY_H, true};
        return;
      }
    }
    attempts++;
  }
}

void playSpaceInvaders() {
  gameStartAnim(1);
  siReset();
  siLastLeft = siLastRight = siLastShoot = 0;

  while (true) {
    if (btnPressed(BTN_SELECT)) { delay(300); ledsOff(); return; }

    if (siGameOver || siGameWon) {
      tft.fillScreen(ST77XX_BLACK);
      tft.drawRect(2,2,SCREEN_W-4,SCREEN_H-4, siGameWon ? 0x07E0 : ST77XX_RED);
      if (siGameWon) {
        tft.setTextColor(0x07E0); tft.setTextSize(2);
        tft.setCursor(30, 25); tft.print("YOU");
        tft.setCursor(30, 47); tft.print("WIN!");
        digitalWrite(LED_GREEN, HIGH); digitalWrite(LED_RED, LOW);
      } else {
        tft.setTextColor(ST77XX_RED); tft.setTextSize(2);
        tft.setCursor(25, 25); tft.print("GAME");
        tft.setCursor(25, 47); tft.print("OVER");
        digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      }
      tft.setTextSize(1); tft.setTextColor(COLOR_GOLD);
      tft.setCursor(20, 82); tft.print("Score: "); tft.print(siScore);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(15, 97);  tft.print("START  = Reiniciar");
      tft.setCursor(15, 110); tft.print("SELECT = Menu");

      while (true) {
        if (btnPressed(BTN_START))  { delay(300); siReset(); break; }
        if (btnPressed(BTN_SELECT)) { delay(300); ledsOff(); return; }
        delay(10);
      }
      continue;
    }

    unsigned long now = millis();

    if (btnPressed(BTN_LEFT) && now - siLastLeft > SI_DEBOUNCE_MOVE) {
      siLastLeft = now;
      tft.fillRect(siPlayerX, PLAYER_Y, PLAYER_W, PLAYER_H+1, ST77XX_BLACK);
      siPlayerX -= PLAYER_SPEED;
      if (siPlayerX < 0) siPlayerX = 0;
      siDrawPlayer(siPlayerX, COLOR_TEAL);
    }
    if (btnPressed(BTN_RIGHT) && now - siLastRight > SI_DEBOUNCE_MOVE) {
      siLastRight = now;
      tft.fillRect(siPlayerX, PLAYER_Y, PLAYER_W, PLAYER_H+1, ST77XX_BLACK);
      siPlayerX += PLAYER_SPEED;
      if (siPlayerX > SCREEN_W - PLAYER_W) siPlayerX = SCREEN_W - PLAYER_W;
      siDrawPlayer(siPlayerX, COLOR_TEAL);
    }
    if ((btnPressed(BTN_A) || btnPressed(BTN_B)) && now - siLastShoot > SI_DEBOUNCE_SHOOT) {
      siLastShoot = now;
      if (!siPlayerBullet.active) {
        siPlayerBullet = {siPlayerX + PLAYER_W/2 - 1, PLAYER_Y - BULLET_H, true};
      }
    }

    // Bala jugador
    if (siPlayerBullet.active) {
      tft.fillRect(siPlayerBullet.x, siPlayerBullet.y, BULLET_W, BULLET_H, ST77XX_BLACK);
      siPlayerBullet.y -= BULLET_SPEED;
      if (siPlayerBullet.y < 0) {
        siPlayerBullet.active = false;
      } else {
        bool hit = false;
        for (int r = 0; r < ENEMY_ROWS && !hit; r++) {
          for (int c = 0; c < ENEMY_COLS && !hit; c++) {
            if (!siEnemies[r][c]) continue;
            int ex = siEnemyOffsetX + c * (ENEMY_W + ENEMY_PAD_X);
            int ey = siEnemyOffsetY + r * (ENEMY_H + ENEMY_PAD_Y);
            if (siPlayerBullet.x + BULLET_W > ex && siPlayerBullet.x < ex + ENEMY_W &&
                siPlayerBullet.y + BULLET_H > ey && siPlayerBullet.y < ey + ENEMY_H) {
              siEnemies[r][c] = false; siEnemiesAlive--;
              tft.fillRect(ex-1, ey-1, ENEMY_W+2, ENEMY_H+2, ST77XX_BLACK);
              siPlayerBullet.active = false; hit = true;
              int pts[] = {30, 20, 10};
              siScore += pts[r];
              siDrawHUD();
              siEnemyMoveDelay = max(80, siEnemyMoveDelay - 10);
              if (siEnemiesAlive == 0) siGameWon = true;
            }
          }
        }
        if (siPlayerBullet.active)
          tft.fillRect(siPlayerBullet.x, siPlayerBullet.y, BULLET_W, BULLET_H, COLOR_GOLD);
      }
    }

    // Movimiento enemigos
    siEnemyMoveTimer += 16;
    if (siEnemyMoveTimer >= siEnemyMoveDelay) {
      siEnemyMoveTimer = 0;
      for (int r = 0; r < ENEMY_ROWS; r++)
        for (int c = 0; c < ENEMY_COLS; c++)
          if (siEnemies[r][c]) {
            int px = siEnemyOffsetX + c * (ENEMY_W + ENEMY_PAD_X);
            int py = siEnemyOffsetY + r * (ENEMY_H + ENEMY_PAD_Y);
            tft.fillRect(px-1, py-1, ENEMY_W+2, ENEMY_H+2, ST77XX_BLACK);
          }

      int nextX = siEnemyOffsetX + siEnemyDirX * 4;
      int maxX = SCREEN_W - (ENEMY_COLS * (ENEMY_W + ENEMY_PAD_X));
      bool bounce = (nextX < 0 || nextX > maxX);
      if (bounce) {
        siEnemyDirX = -siEnemyDirX;
        siEnemyOffsetY += 4;
        if (siEnemyOffsetY + ENEMY_ROWS * (ENEMY_H + ENEMY_PAD_Y) >= PLAYER_Y) { siGameOver = true; continue; }
      } else {
        siEnemyOffsetX = nextX;
      }

      for (int r = 0; r < ENEMY_ROWS; r++)
        for (int c = 0; c < ENEMY_COLS; c++)
          if (siEnemies[r][c])
            siDrawEnemy(siEnemyOffsetX + c*(ENEMY_W+ENEMY_PAD_X),
                        siEnemyOffsetY + r*(ENEMY_H+ENEMY_PAD_Y), r, 0xFFFF);

      if (random(4) == 0) siEnemyShoot();
    }

    // Balas enemigas
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
      if (!siEnemyBullets[i].active) continue;
      tft.fillRect(siEnemyBullets[i].x, siEnemyBullets[i].y, ENEMY_BULLET_W, ENEMY_BULLET_H, ST77XX_BLACK);
      siEnemyBullets[i].y += ENEMY_BULLET_SPEED;
      if (siEnemyBullets[i].y > SCREEN_H) { siEnemyBullets[i].active = false; continue; }
      if (siEnemyBullets[i].x + ENEMY_BULLET_W > siPlayerX &&
          siEnemyBullets[i].x < siPlayerX + PLAYER_W &&
          siEnemyBullets[i].y + ENEMY_BULLET_H > PLAYER_Y &&
          siEnemyBullets[i].y < PLAYER_Y + PLAYER_H) {
        siEnemyBullets[i].active = false;
        siLives--;
        siDrawHUD();
        if (siLives <= 0) { siGameOver = true; continue; }
        siDrawPlayer(siPlayerX, ST77XX_BLACK); delay(200);
        siDrawPlayer(siPlayerX, COLOR_TEAL);
        continue;
      }
      tft.fillRect(siEnemyBullets[i].x, siEnemyBullets[i].y, ENEMY_BULLET_W, ENEMY_BULLET_H, COLOR_PINK);
    }

    delay(16);
  }
}

// ═══════════════════════════════════════════════════════════
//  TETRIS
// ═══════════════════════════════════════════════════════════
#define T_CELL     8
#define T_BOARD_W  10
#define T_BOARD_H  16
#define T_BOARD_X  ((SCREEN_W - T_BOARD_W * T_CELL) / 2)
#define T_BOARD_Y  0

uint16_t T_COLORS[8] = {
  ST77XX_BLACK,
  COLOR_TEAL,   // I
  ST77XX_YELLOW,// O
  COLOR_PINK,   // T
  0x001F,       // S - azul
  ST77XX_RED,   // Z
  0x07E0,       // J - verde
  COLOR_ORANGE  // L
};

const int8_t T_PIECES[7][4][4][2] = {
  {{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}, {{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}},
  {{{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}}},
  {{{-1,0},{0,0},{1,0},{0,1}}, {{0,-1},{0,0},{0,1},{-1,0}},{{-1,0},{0,0},{1,0},{0,-1}},{{0,-1},{0,0},{0,1},{1,0}}},
  {{{0,0},{1,0},{-1,1},{0,1}}, {{0,-1},{0,0},{1,0},{1,1}}, {{0,0},{1,0},{-1,1},{0,1}}, {{0,-1},{0,0},{1,0},{1,1}}},
  {{{-1,0},{0,0},{0,1},{1,1}}, {{1,-1},{0,0},{1,0},{0,1}}, {{-1,0},{0,0},{0,1},{1,1}}, {{1,-1},{0,0},{1,0},{0,1}}},
  {{{-1,0},{0,0},{1,0},{-1,1}},{{0,-1},{0,0},{0,1},{-1,-1}},{{-1,0},{0,0},{1,0},{1,-1}},{{0,-1},{0,0},{0,1},{1,1}}},
  {{{-1,0},{0,0},{1,0},{1,1}}, {{0,-1},{0,0},{0,1},{-1,1}},{{-1,0},{0,0},{1,0},{-1,-1}},{{0,-1},{0,0},{0,1},{1,-1}}}
};

uint8_t tBoard[T_BOARD_H][T_BOARD_W];
int tPieceType, tPieceRot, tPieceX, tPieceY;
int tNextType;
bool tGameOver;
unsigned long tLastFall;
int tFallDelay;
long tScore;
int tLevel, tLinesCleared;

unsigned long tLastBtnA=0, tLastBtnB=0, tLastLeft=0, tLastRight=0, tLastDown=0, tLastUp=0;
#define T_DEBOUNCE      150
#define T_DEBOUNCE_MOVE 100

void tDrawCell(int x, int y, uint16_t color) {
  tft.fillRect(T_BOARD_X + x*T_CELL + 1, T_BOARD_Y + y*T_CELL + 1,
               T_CELL - 2, T_CELL - 2, color);
  if (color != ST77XX_BLACK) {
    // Brillo top-left
    tft.drawPixel(T_BOARD_X + x*T_CELL + 1, T_BOARD_Y + y*T_CELL + 1, ST77XX_WHITE);
  }
}

void tDrawBorder() {
  tft.drawRect(T_BOARD_X-1, T_BOARD_Y-1, T_BOARD_W*T_CELL+2, T_BOARD_H*T_CELL+2, COLOR_GOLD);
  tft.drawRect(T_BOARD_X-2, T_BOARD_Y-2, T_BOARD_W*T_CELL+4, T_BOARD_H*T_CELL+4, COLOR_DARK);
}

void tDrawScoreArea() {
  int sx = T_BOARD_X + T_BOARD_W * T_CELL + 5;
  tft.fillRect(sx-1, 0, SCREEN_W - sx + 1, SCREEN_H, ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD); tft.setCursor(sx, 4);  tft.print("SCORE");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(sx, 14); tft.print(tScore);

  tft.setTextColor(COLOR_TEAL); tft.setCursor(sx, 32); tft.print("LEVEL");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(sx, 42); tft.print(tLevel);

  tft.setTextColor(COLOR_PINK); tft.setCursor(sx, 58); tft.print("NEXT");
  // Miniatura de la siguiente pieza
  for (int r = 0; r < 4; r++) {
    int dx = T_PIECES[tNextType][0][r][0];
    int dy = T_PIECES[tNextType][0][r][1];
    tft.fillRect(sx + (dx+1)*6, 70 + (dy+1)*6, 5, 5, T_COLORS[tNextType+1]);
  }

  // Líneas
  tft.setTextColor(0x8410); tft.setCursor(sx, 100); tft.print("LINES");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(sx, 110); tft.print(tLinesCleared);
}

bool tCheckCollision(int type, int rot, int cx, int cy) {
  for (int i = 0; i < 4; i++) {
    int nx = cx + T_PIECES[type][rot][i][0];
    int ny = cy + T_PIECES[type][rot][i][1];
    if (nx < 0 || nx >= T_BOARD_W || ny >= T_BOARD_H) return true;
    if (ny >= 0 && tBoard[ny][nx] != 0) return true;
  }
  return false;
}

void tSpawnPiece() {
  tPieceType = tNextType; tPieceRot = 0;
  tPieceX = T_BOARD_W/2; tPieceY = 1;
  tNextType = random(7);
  if (tCheckCollision(tPieceType, tPieceRot, tPieceX, tPieceY)) tGameOver = true;
}

void tDrawPiece(uint16_t color) {
  for (int i = 0; i < 4; i++) {
    int nx = tPieceX + T_PIECES[tPieceType][tPieceRot][i][0];
    int ny = tPieceY + T_PIECES[tPieceType][tPieceRot][i][1];
    if (ny >= 0) tDrawCell(nx, ny, color);
  }
}

void tLockPiece() {
  for (int i = 0; i < 4; i++) {
    int nx = tPieceX + T_PIECES[tPieceType][tPieceRot][i][0];
    int ny = tPieceY + T_PIECES[tPieceType][tPieceRot][i][1];
    if (ny >= 0) tBoard[ny][nx] = tPieceType + 1;
  }
}

void tRedrawBoard() {
  for (int y = 0; y < T_BOARD_H; y++)
    for (int x = 0; x < T_BOARD_W; x++)
      tDrawCell(x, y, T_COLORS[tBoard[y][x]]);
}

void tClearLines() {
  int cleared = 0;
  for (int y = T_BOARD_H-1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < T_BOARD_W; x++) if (tBoard[y][x] == 0) { full = false; break; }
    if (full) {
      cleared++;
      // Flash de línea
      for (int x = 0; x < T_BOARD_W; x++) tDrawCell(x, y, ST77XX_WHITE);
      delay(60);
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < T_BOARD_W; x++) tBoard[yy][x] = tBoard[yy-1][x];
      for (int x = 0; x < T_BOARD_W; x++) tBoard[0][x] = 0;
      y++;
    }
  }
  if (cleared > 0) {
    tLinesCleared += cleared;
    int pts[] = {0, 40, 100, 300, 1200};
    tScore += (long)pts[cleared] * (tLevel + 1);
    tLevel = tLinesCleared / 10;
    tFallDelay = max(100, 800 - tLevel * 70);
    tRedrawBoard();
    tDrawBorder();
    tDrawScoreArea();
  }
}

void tReset() {
  memset(tBoard, 0, sizeof(tBoard));
  tScore = 0; tLevel = 0; tLinesCleared = 0; tFallDelay = 800; tGameOver = false;
  tNextType = random(7);
  tft.fillScreen(ST77XX_BLACK);
  tDrawBorder();
  tRedrawBoard();
  tSpawnPiece();
  tDrawPiece(T_COLORS[tPieceType+1]);
  tDrawScoreArea();
  tLastFall = millis();
  digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, HIGH);
}

void playTetris() {
  gameStartAnim(2);
  tReset();
  tLastBtnA = tLastBtnB = tLastLeft = tLastRight = tLastDown = tLastUp = 0;

  while (true) {
    if (btnPressed(BTN_SELECT)) { delay(300); ledsOff(); return; }

    if (tGameOver) {
      digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      tft.fillScreen(ST77XX_BLACK);
      tft.drawRect(2,2,SCREEN_W-4,SCREEN_H-4, ST77XX_RED);
      tft.setTextColor(ST77XX_RED); tft.setTextSize(2);
      tft.setCursor(25, 20); tft.print("GAME");
      tft.setCursor(25, 42); tft.print("OVER");
      tft.setTextSize(1);
      tft.setTextColor(COLOR_GOLD); tft.setCursor(20, 72); tft.print("Score: "); tft.print(tScore);
      tft.setTextColor(COLOR_TEAL); tft.setCursor(20, 85); tft.print("Level: "); tft.print(tLevel);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(15, 100); tft.print("START  = Reiniciar");
      tft.setCursor(15, 113); tft.print("SELECT = Menu");

      while (true) {
        if (btnPressed(BTN_START))  { delay(300); tReset(); break; }
        if (btnPressed(BTN_SELECT)) { delay(300); ledsOff(); return; }
        delay(10);
      }
      continue;
    }

    unsigned long now = millis();

    if (btnPressed(BTN_A) && now - tLastBtnA > T_DEBOUNCE) {
      tLastBtnA = now; tDrawPiece(ST77XX_BLACK);
      int nr = (tPieceRot+1) % 4;
      if (!tCheckCollision(tPieceType, nr, tPieceX, tPieceY)) tPieceRot = nr;
      tDrawPiece(T_COLORS[tPieceType+1]);
    }
    if (btnPressed(BTN_B) && now - tLastBtnB > T_DEBOUNCE) {
      tLastBtnB = now; tDrawPiece(ST77XX_BLACK);
      int nr = (tPieceRot+3) % 4;
      if (!tCheckCollision(tPieceType, nr, tPieceX, tPieceY)) tPieceRot = nr;
      tDrawPiece(T_COLORS[tPieceType+1]);
    }
    if (btnPressed(BTN_LEFT) && now - tLastLeft > T_DEBOUNCE_MOVE) {
      tLastLeft = now; tDrawPiece(ST77XX_BLACK);
      if (!tCheckCollision(tPieceType, tPieceRot, tPieceX-1, tPieceY)) tPieceX--;
      tDrawPiece(T_COLORS[tPieceType+1]);
    }
    if (btnPressed(BTN_RIGHT) && now - tLastRight > T_DEBOUNCE_MOVE) {
      tLastRight = now; tDrawPiece(ST77XX_BLACK);
      if (!tCheckCollision(tPieceType, tPieceRot, tPieceX+1, tPieceY)) tPieceX++;
      tDrawPiece(T_COLORS[tPieceType+1]);
    }
    if (btnPressed(BTN_DOWN) && now - tLastDown > T_DEBOUNCE_MOVE) {
      tLastDown = now; tDrawPiece(ST77XX_BLACK);
      if (!tCheckCollision(tPieceType, tPieceRot, tPieceX, tPieceY+1)) { tPieceY++; tLastFall = now; }
      tDrawPiece(T_COLORS[tPieceType+1]);
    }
    if (btnPressed(BTN_UP) && now - tLastUp > T_DEBOUNCE) {
      tLastUp = now; tDrawPiece(ST77XX_BLACK);
      while (!tCheckCollision(tPieceType, tPieceRot, tPieceX, tPieceY+1)) tPieceY++;
      tLockPiece(); tClearLines(); tSpawnPiece();
      if (!tGameOver) { tDrawPiece(T_COLORS[tPieceType+1]); tDrawScoreArea(); }
      tLastFall = now; continue;
    }
    if (now - tLastFall >= (unsigned long)tFallDelay) {
      tLastFall = now; tDrawPiece(ST77XX_BLACK);
      if (!tCheckCollision(tPieceType, tPieceRot, tPieceX, tPieceY+1)) {
        tPieceY++; tDrawPiece(T_COLORS[tPieceType+1]);
      } else {
        tDrawPiece(T_COLORS[tPieceType+1]);
        tLockPiece(); tClearLines(); tSpawnPiece();
        if (!tGameOver) { tDrawPiece(T_COLORS[tPieceType+1]); tDrawScoreArea(); }
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  SETUP & LOOP PRINCIPAL
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_A,      INPUT_PULLUP);
  pinMode(BTN_B,      INPUT_PULLUP);
  pinMode(BTN_START,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  ledsOff();

  randomSeed(micros());

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  // ── Splash screen de bienvenida ──────────────────────────
  // Fondo con degradado de columnas
  for (int x = 0; x < SCREEN_W; x++) {
    uint8_t b = x / 8;
    tft.drawFastVLine(x, 0, SCREEN_H, tft.color565(0, 0, b));
  }

 // Logo grande
tft.setTextColor(COLOR_GOLD);
printCentered("GAME HUB", 10, 2);

  // Subtítulo
tft.setTextColor(ST77XX_WHITE);
printCentered("Jhon&David Edition", 35);

  // Iconos de los 3 juegos
  drawIconSnake(8,   55, 0x07E0);
drawIconInvader(52, 55, ST77XX_RED);
drawIconTetris(96,  55, COLOR_TEAL);

  tft.setTextSize(1);
  tft.setTextColor(0x8410);
tft.setCursor(8,  75); tft.print("Snake");
tft.setCursor(50, 75); tft.print("Space");
tft.setCursor(90, 75); tft.print("Tetris");
  // Línea divisoria
  tft.drawFastHLine(10, 88, SCREEN_W-20, COLOR_DARK);

  // Parpadeante "presiona start"
for (int i = 0; i < 3; i++) {
  tft.setTextColor(COLOR_GOLD);
  printCentered("Presiona  START", 100);
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
  tft.setTextColor(ST77XX_BLACK);
  printCentered("Presiona  START", 100);
  digitalWrite(LED_GREEN, LOW);
  delay(400);
}
tft.setTextColor(COLOR_GOLD);
printCentered("Presiona  START", 100);
digitalWrite(LED_GREEN, HIGH);

  while (digitalRead(BTN_START)) { delay(10); }
  delay(300);
  ledsOff();

  // Entra al menú con animación
  menuIntroAnim();
}

void loop() {
  if (!inMenu) return; // nunca debería pasar, pero por si acaso

  // Animación de parpadeo del menú (actualiza cada 600ms)
  static unsigned long lastAnim = 0;
  static int animFrame = 0;
  if (millis() - lastAnim > 600) {
    lastAnim = millis();
    animFrame++;
    drawMenuFrame(menuSel, animFrame);
    // Pulso LED verde suave
    digitalWrite(LED_GREEN, animFrame % 2);
  }

  unsigned long now = millis();
  if (now - lastMenuBtn < MENU_DEBOUNCE) return;

  if (btnPressed(BTN_UP) || btnPressed(BTN_LEFT)) {
    lastMenuBtn = now;
    menuSel = (menuSel + 2) % 3; // -1 en circular
    drawMenuFrame(menuSel, animFrame);
  }
  else if (btnPressed(BTN_DOWN) || btnPressed(BTN_RIGHT)) {
    lastMenuBtn = now;
    menuSel = (menuSel + 1) % 3;
    drawMenuFrame(menuSel, animFrame);
  }
  else if (btnPressed(BTN_START) || btnPressed(BTN_A)) {
    lastMenuBtn = now;
    ledsOff();
    // Lanza el juego seleccionado
    switch (menuSel) {
      case 0: playSnake();         break;
      case 1: playSpaceInvaders(); break;
      case 2: playTetris();        break;
    }
    // Regresa al menú con animación
    menuReturnAnim();
    menuNeedsFullRedraw = true;
    animFrame = 0;
    lastAnim = 0;
  }
}
