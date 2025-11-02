#include <limits.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 800;
const int FIELD_RESOLUTION = 5;
const int COLNUM = SCREEN_WIDTH / FIELD_RESOLUTION;
const int ROWNUM = SCREEN_HEIGHT / FIELD_RESOLUTION;
const float MAX_VAL = 1000;
const float MIN_VAL = -1000;

float *amplitudes;
float *dAmplitudesOverDt;

bool oscillatorOn = true;
float oscillatorAmplitude = MAX_VAL;
float oscillatorPeriod = 0.5;
float oscillatorTimer = 0;
Vector2 oscillatorPos = {(float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2};

int getFlatIndex(int colIndex, int rowIndex) {
  return (rowIndex * COLNUM) + colIndex;
}

void inputHandler() {
  int key = GetKeyPressed();
  switch (key) {
  case KEY_Q:
    CloseWindow();
    break;
  case KEY_R:
    for (int i = 0; i < COLNUM * ROWNUM; i++) {
      amplitudes[i] = 0;
      dAmplitudesOverDt[i] = 0;
    }
    break;
  case KEY_O:
    oscillatorOn = !oscillatorOn;
    if (oscillatorOn)
      oscillatorPos = GetMousePosition();
    break;
  default:
    break;
  }
  if (key == KEY_Q) {
    CloseWindow();
  }
}

void updateVelocityField() {
  float currentAmplitudes[COLNUM * ROWNUM];
  float force[COLNUM * ROWNUM];
  for (int i = 0; i < COLNUM * ROWNUM; i++) {
    force[i] = 0;
  }
  float constantA = -1;

  memcpy(currentAmplitudes, amplitudes, sizeof(currentAmplitudes));
  for (int row = 0; row < ROWNUM; row++) {
    for (int col = 0; col < COLNUM; col++) {
      float currentCellAmplitude = currentAmplitudes[getFlatIndex(col, row)];
      // float n = row <= 0 ? 0 : currentAmplitudes[getFlatIndex(col, row - 1)];
      float n = row <= 0 ? currentCellAmplitude
                         : currentAmplitudes[getFlatIndex(col, row - 1)];
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - n);

      // float ne = 0;
      float ne = currentCellAmplitude;
      if (col < COLNUM - 1 && row > 0) {
        ne = currentAmplitudes[getFlatIndex(col + 1, row - 1)];
      }
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - ne);

      // float e =
      //     col >= COLNUM - 1 ? 0 : currentAmplitudes[getFlatIndex(col + 1,
      //     row)];
      float e = col >= COLNUM - 1
                    ? currentCellAmplitude
                    : currentAmplitudes[getFlatIndex(col + 1, row)];
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - e);

      // float se = 0;
      float se = currentCellAmplitude;
      if (col < COLNUM - 1 && row < ROWNUM - 1) {
        se = currentAmplitudes[getFlatIndex(col + 1, row + 1)];
      }
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - se);

      // float s =
      //     row >= ROWNUM - 1 ? 0 : currentAmplitudes[getFlatIndex(col, row +
      //     1)];
      float s = row >= ROWNUM - 1
                    ? currentCellAmplitude
                    : currentAmplitudes[getFlatIndex(col, row + 1)];
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - s);

      // float sw = 0;
      float sw = currentCellAmplitude;
      if (col > 0 && row < ROWNUM - 1) {
        sw = currentAmplitudes[getFlatIndex(col - 1, row + 1)];
      }
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - sw);

      // float w = col <= 0 ? 0 : currentAmplitudes[getFlatIndex(col - 1, row)];
      float w = col <= 0 ? currentCellAmplitude
                         : currentAmplitudes[getFlatIndex(col - 1, row)];
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - w);

      // float nw = 0;
      float nw = currentCellAmplitude;
      if (col > 0 && row > 0) {
        nw = currentAmplitudes[getFlatIndex(col - 1, row - 1)];
      }
      force[getFlatIndex(col, row)] += constantA * (currentCellAmplitude - nw);
    }
  }

  float damping = 0.995;
  for (int i = 0; i < COLNUM * ROWNUM; i++) {
    dAmplitudesOverDt[i] += force[i] * GetFrameTime();
    dAmplitudesOverDt[i] *= damping;
  }
}

Color getColor(int col, int row) {
  float val = amplitudes[getFlatIndex(col, row)];
  val = val > MAX_VAL ? MAX_VAL : val;
  val = val < MIN_VAL ? MIN_VAL : val;

  float normalizedVal = val / MAX_VAL;

  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;

  if (normalizedVal >= 0) {
    r = (unsigned char)(255 * normalizedVal);
  } else {
    g = (unsigned char)(255 * -normalizedVal);
  }

  return (Color){r, g, b, 255};
}

void updateAmplitudes() {
  float constantB = 200;
  for (int i = 0; i < COLNUM * ROWNUM; i++) {
    amplitudes[i] += constantB * dAmplitudesOverDt[i] * GetFrameTime();
  }
}
int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "test");
  SetTargetFPS(60);

  amplitudes = malloc(SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(float));
  dAmplitudesOverDt = malloc(SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(float));
  for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
    amplitudes[i] = 0;
    dAmplitudesOverDt[i] = 0;
  }

  while (!WindowShouldClose()) {
    inputHandler();
    Vector2 cursor = GetMousePosition();
    int cursorCol = (int)cursor.x / FIELD_RESOLUTION;
    int cursorRow = (int)cursor.y / FIELD_RESOLUTION;
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) ||
        IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
      amplitudes[getFlatIndex(cursorCol, cursorRow)] = MAX_VAL;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) ||
        IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      amplitudes[getFlatIndex(cursorCol, cursorRow)] = MIN_VAL;
    }
    if (oscillatorOn) {
      oscillatorTimer += GetFrameTime();
      if (oscillatorTimer >= oscillatorPeriod) {
        if (oscillatorAmplitude == MAX_VAL)
          oscillatorAmplitude = MIN_VAL;
        else
          oscillatorAmplitude = MAX_VAL;
        amplitudes[getFlatIndex(oscillatorPos.x / FIELD_RESOLUTION,
                                oscillatorPos.y / FIELD_RESOLUTION)] =
            oscillatorAmplitude;

        oscillatorTimer = 0;
      } else {
        amplitudes[getFlatIndex(oscillatorPos.x / FIELD_RESOLUTION,
                                oscillatorPos.y / FIELD_RESOLUTION)] =
            oscillatorAmplitude;
      }
    }
    updateVelocityField();
    updateAmplitudes();

    BeginDrawing();
    ClearBackground(WHITE);

    for (int x = 0; x < SCREEN_WIDTH; x += FIELD_RESOLUTION) {
      for (int y = 0; y < SCREEN_HEIGHT; y += FIELD_RESOLUTION) {
        int col = x / FIELD_RESOLUTION;
        int row = y / FIELD_RESOLUTION;
        Color color = getColor(col, row);
        DrawRectangle(x, y, FIELD_RESOLUTION, FIELD_RESOLUTION, color);
      }
    }

    EndDrawing();
  }
  free(amplitudes);
  free(dAmplitudesOverDt);

  CloseWindow();
  return EXIT_SUCCESS;
}
