#include <libndls.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>
#include <vector>
#include <string>
#include <sys/stat.h>

extern "C" void _fini(void) {}

#define SCREEN_W 320
#define SCREEN_H 240

static uint16_t framebuffer[SCREEN_W * SCREEN_H];

// Minimal 8x8 ASCII font table (characters 32 to 90)
static const uint8_t font8x8[59][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // '!'
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // '"'
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // '#'
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // '$'
    {0x00,0x66,0x6C,0x18,0x30,0x66,0x46,0x00}, // '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // '&'
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // '\''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // '.'
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // '1'
    {0x3C,0x66,0x0C,0x18,0x30,0x60,0x7E,0x00}, // '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // '3'
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // '5'
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // '6'
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // '9'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // ':'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // '>'
    {0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00}, // '?'
    {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00}, // '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 'I'
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 'P'
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 'Q'
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}  // 'Z'
};

class ImageViewer {
private:
    std::vector<std::string> file_list;
    int current_index = 0;
    int rotation = 0;

    uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }

    void drawChar(int x, int y, char c, uint16_t color) {
        c = std::toupper(c);
        if (c < 32 || c > 90) c = '?';
        int idx = c - 32;

        for (int row = 0; row < 8; row++) {
            uint8_t bits = font8x8[idx][row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) {
                    int px = x + col;
                    int py = y + row;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                        framebuffer[py * SCREEN_W + px] = color;
                    }
                }
            }
        }
    }

    void drawText(int x, int y, const char* str, uint16_t color) {
        while (*str) {
            drawChar(x, y, *str, color);
            x += 8;
            str++;
        }
    }

    long findValidBMPHeaderOffset(FILE* f) {
        if (!f) return -1;

        fseek(f, 0, SEEK_SET);
        unsigned char buf[1024];
        size_t bytesRead = fread(buf, 1, sizeof(buf), f);

        for (size_t i = 0; i + 54 <= bytesRead; i++) {
            if (buf[i] == 'B' && buf[i + 1] == 'M') {
                uint16_t bpp  = *(uint16_t*)&buf[i + 28];
                uint32_t comp = *(uint32_t*)&buf[i + 30];
                int32_t  w    = *(int32_t*)&buf[i + 18];
                int32_t  h    = *(int32_t*)&buf[i + 22];

                if ((bpp == 24 || bpp == 32) && (comp == 0 || comp == 3) && w > 0 && w <= 2048 && abs(h) <= 2048) {
                    return (long)i;
                }
            }
        }
        return -1;
    }

    bool loadBMP(const char* filepath, int rot) {
        FILE* f = fopen(filepath, "rb");
        if (!f) return false;

        long bmpOffset = findValidBMPHeaderOffset(f);
        if (bmpOffset < 0) {
            fclose(f);
            return false;
        }

        fseek(f, bmpOffset, SEEK_SET);

        unsigned char header[54];
        if (fread(header, 1, 54, f) != 54) {
            fclose(f);
            return false;
        }

        uint32_t dataOffset  = *(uint32_t*)&header[10] + bmpOffset;
        int32_t  width       = *(int32_t*)&header[18];
        int32_t  height      = *(int32_t*)&header[22];
        uint16_t bpp        = *(uint16_t*)&header[28];
        uint32_t compression = *(uint32_t*)&header[30];

        if ((bpp != 24 && bpp != 32) || (compression != 0 && compression != 3)) {
            fclose(f);
            return false;
        }

        fseek(f, dataOffset, SEEK_SET);
        memset(framebuffer, 0, sizeof(framebuffer));

        bool flipY = height > 0;
        if (height < 0) height = -height;

        int bytesPerPixel = bpp / 8;
        int rowSize = (width * bytesPerPixel + 3) & (~3);
        uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);

        for (int y = 0; y < height && y < SCREEN_H; y++) {
            if (fread(rowBuffer, 1, rowSize, f) != (size_t)rowSize) break;

            int srcY = flipY ? (height - 1 - y) : y;

            for (int x = 0; x < width && x < SCREEN_W; x++) {
                uint8_t b = rowBuffer[x * bytesPerPixel];
                uint8_t g = rowBuffer[x * bytesPerPixel + 1];
                uint8_t r = rowBuffer[x * bytesPerPixel + 2];
                uint16_t color = rgbTo565(r, g, b);

                int destX = x;
                int destY = srcY;

                if (rot == 1) { destX = 279 - srcY; destY = x - 40; }
                else if (rot == 2) { destX = SCREEN_W - 1 - x; destY = SCREEN_H - 1 - srcY; }
                else if (rot == 3) { destX = srcY + 40; destY = 279 - x; }

                if (destX >= 0 && destX < SCREEN_W && destY >= 0 && destY < SCREEN_H) {
                    framebuffer[destY * SCREEN_W + destX] = color;
                }
            }
        }

        free(rowBuffer);
        fclose(f);
        return true;
    }

    bool isDirectory(const char* path) {
        struct stat st;
        if (stat(path, &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    bool isImageCandidate(const std::string& name) {
        std::string lower = name;
        for (char &c : lower) c = std::tolower((unsigned char)c);
        return lower.find(".bmp") != std::string::npos || lower.find(".tns") != std::string::npos;
    }

    void scanDirectory(const char* dir_path) {
        if (!dir_path || !isDirectory(dir_path)) return;

        DIR* dir = opendir(dir_path);
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            if (!isImageCandidate(name)) continue;

            std::string full_path = std::string(dir_path) + "/" + name;
            FILE* f = fopen(full_path.c_str(), "rb");
            if (f) {
                if (findValidBMPHeaderOffset(f) >= 0) {
                    bool alreadyAdded = false;
                    for (const auto& item : file_list) {
                        if (item == full_path) { alreadyAdded = true; break; }
                    }
                    if (!alreadyAdded) file_list.push_back(full_path);
                }
                fclose(f);
            }
        }
        closedir(dir);
    }

public:
    void run() {
        scanDirectory(".");
        scanDirectory("/documents");
        scanDirectory("/MyLib");
        scanDirectory("/mnt/files");
        scanDirectory("/home/user/Documents");

        bool renderNeeded = true;
        bool exitHeld = false;

        while (true) {
            bool escapePressed = isKeyPressed(KEY_NSPIRE_ESC);
            bool enterPressed = isKeyPressed(KEY_NSPIRE_ENTER);
            bool docPressed = isKeyPressed(KEY_NSPIRE_DOC);

            if ((escapePressed || enterPressed || docPressed) && !exitHeld) break;
            exitHeld = escapePressed || enterPressed || docPressed;

            if (renderNeeded) {
                memset(framebuffer, 0, sizeof(framebuffer));

                if (file_list.empty()) {
                    drawText(10, 20, "IMAGEVIEWER V1.0", 0xF800);
                    drawText(10, 50, "NO IMAGES FOUND!", 0xFFFF);
                    drawText(10, 80, "PLACE 320X240 BMP FILES", 0x07E0);
                    drawText(10, 100, "IN /DOCUMENTS OR /MYLIB", 0x07E0);
                    drawText(10, 150, "PRESS ESC OR ENTER TO EXIT", 0xFFE0);
                } else {
                    if (!loadBMP(file_list[current_index].c_str(), rotation)) {
                        drawText(10, 20, "FAILED TO LOAD IMAGE:", 0xF800);
                        drawText(10, 50, file_list[current_index].c_str(), 0xFFFF);
                    } else {
                        char hud[64];
                        snprintf(hud, sizeof(hud), "[%d/%d] %s", current_index + 1, (int)file_list.size(), file_list[current_index].c_str());
                        drawText(5, 225, hud, 0x07E0);
                    }
                }

                lcd_blit(framebuffer, SCR_320x240_565);
                renderNeeded = false;
            }

            if (!file_list.empty()) {
                if (isKeyPressed(KEY_NSPIRE_RIGHT)) {
                    current_index = (current_index + 1) % file_list.size();
                    rotation = 0;
                    renderNeeded = true;
                    msleep(200);
                } else if (isKeyPressed(KEY_NSPIRE_LEFT)) {
                    current_index = (current_index - 1 + file_list.size()) % file_list.size();
                    rotation = 0;
                    renderNeeded = true;
                    msleep(200);
                } else if (isKeyPressed(KEY_NSPIRE_UP)) {
                    rotation = (rotation + 1) % 4;
                    renderNeeded = true;
                    msleep(200);
                } else if (isKeyPressed(KEY_NSPIRE_DOWN)) {
                    rotation = (rotation + 3) % 4;
                    renderNeeded = true;
                    msleep(200);
                }
            }

            msleep(20);
        }

        msleep(100);
    }
};

int main() {
    ImageViewer app;
    app.run();
    return 0;
}
