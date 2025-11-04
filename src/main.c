#include <limits.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 800;
const int FIELD_RESOLUTION = 5;
const int COLNUM = SCREEN_WIDTH / FIELD_RESOLUTION;
const int ROWNUM = SCREEN_HEIGHT / FIELD_RESOLUTION;
const float MAX_VAL = 100.0f;
const float MIN_VAL = -100.0f;

const float DEFAULT_WAVE_SPEED = 100.0f;
const float DEFAULT_DAMPING = 0.995f;
const float MIN_DAMPING = 0.9f;
const float MAX_DAMPING = 1.0f;
const float DAMPING_STEP = 0.005f;

const int UI_PANEL_HEIGHT = 80;
const int TEXT_SIZE = 18;
const int TEXT_PADDING = 10;

typedef struct {
  Vector2 location;
  float width;
  float height;
} Wall;

// currrent and next are used for double buffers thing
// so it doesn't have to be calculated each frame
// look at swap buffers part in updateWaveField()
typedef struct {
  float *current;
  float *next;
  float *velocity;
  int width;
  int height;
} WaveField;

typedef struct {
  bool enabled;
  float amplitude;
  float period;
  float timer;
  int x;
  int yStart;
  int yEnd;
} Oscillator;

typedef struct {
  bool showUI;
  bool paused;
  bool wallsEnabled;
  float damping;
  float gamma;
} SimulationState;

// Global state
WaveField field;
Oscillator oscillator;
SimulationState state;
Wall walls[3];
int wallCount = 3;

int getFlatIndex(int col, int row) { return (row * COLNUM) + col; }

bool initWaveField(WaveField *f, int cols, int rows) {
  int size = cols * rows;
  f->width = cols;
  f->height = rows;

  f->current = (float *)malloc(size * sizeof(float));
  f->next = (float *)malloc(size * sizeof(float));
  f->velocity = (float *)malloc(size * sizeof(float));

  if (!f->current || !f->next || !f->velocity) {
    return false;
  }

  memset(f->current, 0, size * sizeof(float));
  memset(f->next, 0, size * sizeof(float));
  memset(f->velocity, 0, size * sizeof(float));

  return true;
}

void freeWaveField(WaveField *f) {
  free(f->current);
  free(f->next);
  free(f->velocity);
}

void resetWaveField(WaveField *f) {
  int size = f->width * f->height;
  memset(f->current, 0, size * sizeof(float));
  memset(f->next, 0, size * sizeof(float));
  memset(f->velocity, 0, size * sizeof(float));
}

void initWalls() {
  walls[0] = (Wall){{300, 0}, 20, 300};
  walls[1] = (Wall){{300, 340}, 20, 100};
  walls[2] = (Wall){{300, 480}, 20, 380};
}

bool isInWall(int x, int y) {
  if (!state.wallsEnabled)
    return false;

  for (int i = 0; i < wallCount; i++) {
    if (x >= walls[i].location.x && x <= walls[i].location.x + walls[i].width &&
        y >= walls[i].location.y &&
        y <= walls[i].location.y + walls[i].height) {
      return true;
    }
  }
  return false;
}

void handleInput() {
  int key = GetKeyPressed();
  switch (key) {
  case KEY_Q:
    CloseWindow();
    break;
  case KEY_R:
    resetWaveField(&field);
    break;
  case KEY_O:
    oscillator.enabled = !oscillator.enabled;
    break;
  case KEY_W:
    state.wallsEnabled = !state.wallsEnabled;
    break;
  case KEY_SPACE:
    state.paused = !state.paused;
    break;
  case KEY_H:
    state.showUI = !state.showUI;
    break;
  case KEY_EQUAL: // Plus key
    state.damping += DAMPING_STEP;
    if (state.damping > MAX_DAMPING)
      state.damping = MAX_DAMPING;
    break;
  case KEY_MINUS:
    state.damping -= DAMPING_STEP;
    if (state.damping < MIN_DAMPING)
      state.damping = MIN_DAMPING;
    break;
  case KEY_G:
    state.gamma += 0.1f;
    if (state.gamma > 2.0f)
      state.gamma = 2.0f;
    break;
  case KEY_F:
    state.gamma -= 0.1f;
    if (state.gamma < 0.1f)
      state.gamma = 0.1f;
    break;
  default:
    break;
  }

  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    oscillator.period += 0.01f * wheel;
    if (oscillator.period < 0.1f)
      oscillator.period = 0.1f;
  }
}

void updateOscillator(float dt) {
  if (!oscillator.enabled)
    return;

  oscillator.timer += dt;
  if (oscillator.timer >= oscillator.period) {
    oscillator.timer -= oscillator.period;
  }

  float phase = 2.0f * PI * oscillator.timer / oscillator.period;
  oscillator.amplitude =
      MAX_VAL * sinf(phase) + MAX_VAL * sinf(phase - 0.1f * PI);

  int col = oscillator.x / FIELD_RESOLUTION;
  for (int y = oscillator.yStart; y <= oscillator.yEnd; y++) {
    int row = y / FIELD_RESOLUTION;
    if (row >= 0 && row < ROWNUM && col >= 0 && col < COLNUM) {
      field.current[getFlatIndex(col, row)] = oscillator.amplitude;
    }
  }
}

void updateWaveField(float dt) {
  float cSquared = DEFAULT_WAVE_SPEED;
  // float dtSquared = dt * dt;

  for (int row = 0; row < ROWNUM; row++) {
    for (int col = 0; col < COLNUM; col++) {
      int idx = getFlatIndex(col, row);

      if (isInWall(col * FIELD_RESOLUTION, row * FIELD_RESOLUTION)) {
        field.velocity[idx] = 0;
        field.next[idx] = 0;
        continue;
      }

      float center = field.current[idx];
      float laplacian = 0;

      // 8-neighbor Laplacian
      float n = (row > 0) ? field.current[getFlatIndex(col, row - 1)] : center;
      float s = (row < ROWNUM - 1) ? field.current[getFlatIndex(col, row + 1)]
                                   : center;
      float e = (col < COLNUM - 1) ? field.current[getFlatIndex(col + 1, row)]
                                   : center;
      float w = (col > 0) ? field.current[getFlatIndex(col - 1, row)] : center;

      float ne = center, nw = center, se = center, sw = center;
      if (col < COLNUM - 1 && row > 0)
        ne = field.current[getFlatIndex(col + 1, row - 1)];
      if (col > 0 && row > 0)
        nw = field.current[getFlatIndex(col - 1, row - 1)];
      if (col < COLNUM - 1 && row < ROWNUM - 1)
        se = field.current[getFlatIndex(col + 1, row + 1)];
      if (col > 0 && row < ROWNUM - 1)
        sw = field.current[getFlatIndex(col - 1, row + 1)];

      // Weighted Laplacian
      // Why? Because diagonal directions are sqrt(2) times further away
      // than N, S, E, W
      // so... diagonals must be weaker
      laplacian = (n + s + e + w) - 4.0f * center;
      laplacian += 0.5f * ((ne + nw + se + sw) - 4.0f * center);

      field.velocity[idx] += cSquared * laplacian * dt;
      field.velocity[idx] *= state.damping;

      field.next[idx] = center + field.velocity[idx] * dt;

      if (field.next[idx] > MAX_VAL)
        field.next[idx] = MAX_VAL;
      if (field.next[idx] < MIN_VAL)
        field.next[idx] = MIN_VAL;
    }
  }

  // Swap buffers
  float *temp = field.current;
  field.current = field.next;
  field.next = temp;
}

Color getColor(int col, int row) {
  float val = field.current[getFlatIndex(col, row)];
  val = Clamp(val, MIN_VAL, MAX_VAL);

  float normalizedVal = val / MAX_VAL;

  // Apply gamma correction
  float sign = (normalizedVal >= 0) ? 1.0f : -1.0f;
  normalizedVal = sign * powf(fabsf(normalizedVal), state.gamma);

  unsigned char r = 0, g = 0, b = 0;

  if (normalizedVal >= 0) {
    r = (unsigned char)(255 * normalizedVal);
  } else {
    g = (unsigned char)(255 * -normalizedVal);
  }

  return (Color){r, g, b, 255};
}

void drawUI() {
  if (!state.showUI)
    return;

  DrawRectangle(0, SCREEN_HEIGHT - UI_PANEL_HEIGHT, SCREEN_WIDTH,
                UI_PANEL_HEIGHT, (Color){0, 0, 0, 180});

  char info[256];
  snprintf(info, sizeof(info),
           "FPS: %d | Period: %.2f | Damping: %.3f | Gamma: %.1f | %s | %s | "
           "Walls: %s",
           GetFPS(), oscillator.period, state.damping, state.gamma,
           state.paused ? "PAUSED" : "Running",
           oscillator.enabled ? "Osc: ON" : "Osc: OFF",
           state.wallsEnabled ? "ON" : "OFF");

  DrawText(info, TEXT_PADDING, SCREEN_HEIGHT - UI_PANEL_HEIGHT + TEXT_PADDING,
           TEXT_SIZE, WHITE);

  DrawText("Controls: [SPACE] Pause | [O] Toggle Oscillator | [W] Toggle Walls "
           "| [R] Reset | [Mouse +/-] Oscillator Period"
           "| [+/-] Damping",
           TEXT_PADDING,
           SCREEN_HEIGHT - UI_PANEL_HEIGHT + TEXT_PADDING + TEXT_SIZE + 5, 14,
           LIGHTGRAY);
  DrawText("             [G/F] Gamma | [H] Show/Hide UI | [Q] Quit",
           TEXT_PADDING,
           SCREEN_HEIGHT - UI_PANEL_HEIGHT + TEXT_PADDING + TEXT_SIZE + 5 + 16,
           14, LIGHTGRAY);
}

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "test");
  SetTargetFPS(60);

  if (!initWaveField(&field, COLNUM, ROWNUM)) {
    fprintf(stderr, "Failed to allocate memory for wave field\n");
    return EXIT_FAILURE;
  }

  initWalls();

  oscillator = (Oscillator){.enabled = true,
                            .amplitude = 0,
                            .period = 0.6f,
                            .timer = 0,
                            .x = SCREEN_WIDTH - 1,
                            .yStart = 300,
                            .yEnd = 500};

  state = (SimulationState){.showUI = true,
                            .paused = false,
                            .wallsEnabled = true,
                            .damping = DEFAULT_DAMPING,
                            .gamma = 0.5f};

  while (!WindowShouldClose()) {
    handleInput();

    if (!state.paused) {
      float dt = GetFrameTime();

      Vector2 cursor = GetMousePosition();
      int cursorCol = (int)(cursor.x / FIELD_RESOLUTION);
      int cursorRow = (int)(cursor.y / FIELD_RESOLUTION);

      if (cursorCol >= 0 && cursorCol < COLNUM && cursorRow >= 0 &&
          cursorRow < ROWNUM) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
          field.current[getFlatIndex(cursorCol, cursorRow)] = MAX_VAL;
        }
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
          field.current[getFlatIndex(cursorCol, cursorRow)] = MIN_VAL;
        }
      }

      updateOscillator(dt);
      updateWaveField(dt);
    }

    BeginDrawing();
    ClearBackground(BLACK);

    // Draw wave field
    for (int col = 0; col < COLNUM; col++) {
      for (int row = 0; row < ROWNUM; row++) {
        Color color = getColor(col, row);
        DrawRectangle(col * FIELD_RESOLUTION, row * FIELD_RESOLUTION,
                      FIELD_RESOLUTION, FIELD_RESOLUTION, color);
      }
    }

    // Draw walls
    if (state.wallsEnabled) {
      for (int i = 0; i < wallCount; i++) {
        DrawRectangle(walls[i].location.x, walls[i].location.y, walls[i].width,
                      walls[i].height, WHITE);
      }
    }

    // Draw UI
    drawUI();

    EndDrawing();
  }

  freeWaveField(&field);
  CloseWindow();
  return EXIT_SUCCESS;
}
