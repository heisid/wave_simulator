# =====================
# Toolchain
# =====================
CC      := gcc
AR      := ar
RM      := rm -rf
MKDIR   := mkdir -p

# =====================
# Project paths
# =====================
SRC_DIR := src
OBJ_DIR := build
TARGET  := wave

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# =====================
# Third-party deps
# =====================
TPDIR        := third_party
RAYLIB_DIR   := $(TPDIR)/raylib
RAYGUI_DIR   := $(TPDIR)/raygui
RAYLIB_TAG   := 5.5
RAYGUI_TAG   := 4.0

# =====================
# Compiler & linker
# =====================
INCLUDES := -I$(RAYLIB_DIR)/src -I$(RAYGUI_DIR)/src
LDFLAGS  := -L$(RAYLIB_DIR)/src
LIBS     := -lraylib -lm -lpthread -ldl -lGL -lX11 -lrt

CFLAGS := -std=c11 -O2 -Wall $(INCLUDES)

# =====================
# Targets
# =====================
.PHONY: all
all: deps $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)


# =====================
# Dependency setup
# =====================
.PHONY: deps
deps: $(RAYLIB_DIR)/src/libraylib.a $(RAYGUI_DIR)/src/raygui.h

# --- raylib ---
$(RAYLIB_DIR)/src/libraylib.a:
	@echo "==> Building raylib..."
	@$(MKDIR) $(TPDIR)
	test -d $(RAYLIB_DIR) || git submodule add https://github.com/raysan5/raylib.git $(RAYLIB_DIR)
	git -C $(RAYLIB_DIR) fetch --tags --quiet || true
	git -C $(RAYLIB_DIR) checkout $(RAYLIB_TAG)
	$(MAKE) -C $(RAYLIB_DIR)/src PLATFORM=PLATFORM_DESKTOP

# --- raygui ---
$(RAYGUI_DIR)/src/raygui.h:
	@echo "==> Cloning raygui.."
	@$(MKDIR) $(TPDIR)
	test -d $(RAYGUI_DIR) || git submodule add https://github.com/raysan5/raygui.git $(RAYGUI_DIR)
	git -C $(RAYGUI_DIR) || fetch --tags --quiet || true
	git -C $(RAYGUI_DIR) checkout $(RAYGUI_TAG)


# =====================
# Housekeeping
# =====================
.PHONY: run clean distclean

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(OBJ_DIR) $(TARGET)

distclean: clean
	$(RM) $(TPDIR)
