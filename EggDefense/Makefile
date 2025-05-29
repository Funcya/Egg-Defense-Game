# Makefile for Unified Tower Defense (Windows/macOS) - Menu Version

# --- General Configuration ---
CC = gcc
SRCDIR = src
INCDIR = include
OBJDIR = obj
# --- ÄNDRING: Definiera det nya gemensamma målet ---
TARGET_BASE = mittspel
# Target kommer att få .exe på Windows senare

# --- Source Files ---
# Ny huvudfil för menyn
MAIN_MENU_SRC = $(SRCDIR)/main.c

# Befintliga källfiler (behåller gamla variabelnamn för enkelhet)
ENGINE_SRCS = $(SRCDIR)/engine.c $(SRCDIR)/paths.c $(SRCDIR)/render.c $(SRCDIR)/input.c
LOGIC_SRCS  = $(SRCDIR)/gameState.c $(SRCDIR)/enemy.c $(SRCDIR)/birds.c $(SRCDIR)/projectiles.c $(SRCDIR)/money_adt.c
CLIENT_SRC = $(SRCDIR)/client.c # Innehåller run_client
SERVER_SRC = $(SRCDIR)/server.c # Innehåller run_server
MAIN_SP_SRC = $(SRCDIR)/main_sp.c # Innehåller run_singleplayer
# main_server.c och main_client.c behövs inte längre som källfiler om de är tomma

# --- Object Files ---
MAIN_MENU_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_MENU_SRC))
ENGINE_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(ENGINE_SRCS))
LOGIC_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LOGIC_SRCS))
CLIENT_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_SRC))
SERVER_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_SRC))
MAIN_SP_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_SP_SRC))
# main_server.o och main_client.o behövs inte längre

# --- ÄNDRING: Samla ALLA objektfiler som behövs för det slutliga målet ---
ALL_OBJS = $(MAIN_MENU_OBJ) $(ENGINE_OBJS) $(LOGIC_OBJS) $(SERVER_OBJ) $(CLIENT_OBJ) $(MAIN_SP_OBJ)

# --- Platform Specific Settings ---
INCLUDE_PATHS = /usr/local/include/SDL2 # Default för macOS/Linux
LIB_PATHS = /usr/local/lib           # Default för macOS/Linux
LINK_FLAGS = -lSDL2 -lSDL2_net -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm # Default
TARGET = $(TARGET_BASE) # Default målfilnamn
RM = rm -f # Unix remove command
MKDIR_CMD = mkdir -p # Unix command

ifeq ($(OS),Windows_NT)
    CC = gcc
    SDL_BASE_PATH = C:/msys64/mingw64 # Anpassa vid behov
    INCLUDE_PATHS = $(SDL_BASE_PATH)/include/SDL2
    LIB_PATHS = $(SDL_BASE_PATH)/lib
    LINK_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_net -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm
    TARGET = $(TARGET_BASE).exe # Lägg till .exe för Windows
    # Using git bash mkdir -p works on Windows too if available, annars anpassa
    # MKDIR_CMD = if not exist $(subst /,\,$(OBJDIR)) mkdir $(subst /,\,$(OBJDIR))
endif

# --- Compilation and Linking Flags ---
INCLUDE_FLAGS = -I$(INCDIR) -I"$(INCLUDE_PATHS)"
# --- ÄNDRING: Tog bort _USE_MATH_DEFINES som inte alltid behövs, men behåller resten ---
CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -std=c11 -g $(INCLUDE_FLAGS) -D_REENTRANT
LDFLAGS = -L"$(LIB_PATHS)" $(LINK_FLAGS)

# --- Build Rules ---

# --- ÄNDRING: Standardmål är nu det gemensamma målet ---
all: $(TARGET)

# Skapa objektkatalogen om den inte finns
$(OBJDIR):
	@$(MKDIR_CMD) $(OBJDIR)

# --- ÄNDRING: Länkningsregel för det gemensamma målet ---
$(TARGET): $(ALL_OBJS) | $(OBJDIR)
	@echo Linking $@...
	$(CC) $(ALL_OBJS) -o $@ $(LDFLAGS)
	@echo Build complete: $(TARGET)

# Gemensamma headerfiler som kan orsaka omkompilering
COMMON_HEADERS = $(INCDIR)/engine.h $(INCDIR)/defs.h $(INCDIR)/paths.h $(INCDIR)/network.h $(INCDIR)/money_adt.h

# --- ÄNDRING: Kompileringsregler ---

# Regel för den nya main.c
$(OBJDIR)/main.o: $(SRCDIR)/main.c $(COMMON_HEADERS) | $(OBJDIR)
	@echo Compiling $< \(Main Menu\)...
	$(CC) $(CFLAGS) -c $< -o $@

# Regel för main_sp.c (utan -DSINGLEPLAYER)
$(OBJDIR)/main_sp.o: $(SRCDIR)/main_sp.c $(COMMON_HEADERS) | $(OBJDIR)
	@echo Compiling $< \(Singleplayer Logic Entry\)...
	$(CC) $(CFLAGS) -c $< -o $@

# Befintliga regler för server.c och client.c (behöver inte ändras)
$(OBJDIR)/server.o: $(SRCDIR)/server.c $(COMMON_HEADERS) | $(OBJDIR)
	@echo Compiling $< \(Server Logic\)...
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/client.o: $(SRCDIR)/client.c $(COMMON_HEADERS) | $(OBJDIR)
	@echo Compiling $< \(Client Logic\)...
	$(CC) $(CFLAGS) -c $< -o $@

# Generisk regel för alla andra .c-filer i src/
# (Observera att specifika regler ovan har företräde)
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(COMMON_HEADERS) | $(OBJDIR)
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c $< -o $@

# --- Clean Rule ---
clean:
	@echo Cleaning...
ifeq ($(OS),Windows_NT)
	-del /Q /F $(subst /,\,$(OBJDIR)\*.o) 2>nul || (exit 0)
	# --- ÄNDRING: Ta bort endast det nya målet ---
	-del /Q /F $(subst /,\,$(TARGET)) 2>nul || (exit 0)
else
	-$(RM) $(OBJDIR)/*.o
	# --- ÄNDRING: Ta bort endast det nya målet ---
	-$(RM) $(TARGET)
endif
	@echo Clean complete.

.PHONY: all clean $(OBJDIR)