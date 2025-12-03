# Makefile for Cinnamoroll Desk Pet

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -luser32 -lgdi32

SRCDIR = src
RESDIR = res
BINDIR = bin

TARGET = $(BINDIR)/cinnamoroll_pet.exe
BMP_CREATOR = $(BINDIR)/create_bmp.exe

SOURCES = $(SRCDIR)/main.c
BMP_SOURCES = $(SRCDIR)/create_bmp.c

all: directories $(BMP_CREATOR) $(TARGET)

$(BMP_CREATOR): $(BMP_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 创建bin目录
.PHONY: directories

directories:
	@mkdir -p $(BINDIR)
	@mkdir -p $(RESDIR)

# 生成BMP图片资源
.PHONY: resources

resources: $(BMP_CREATOR)
	@cd $(BINDIR) && ./create_bmp.exe

# 清理编译产物
.PHONY: clean

clean:
	@rm -f $(BINDIR)/*
	@rm -f $(RESDIR)/*.bmp
