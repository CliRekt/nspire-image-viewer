#include <libndls.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>
#include <vector>
#include <string>

extern "C" void _fini(void) {}

#define SCREEN_W 320
#define SCREEN_H 240

static uint16_t framebuffer[SCREEN_W * SCREEN_H];

class ImageViewer {
private:
    std::vector<std::string> file_list;
    int current_index = 0;
    int rotation = 0; // 0 = 0°, 1 = 90° CW, 2 = 180° CW, 3 = 270° CW

    uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }

    // Fast check to see if a file has the BMP magic header "BM"
    bool isBMPFile(const char* filepath) {
        FILE* f = fopen(filepath, "rb");
        if (!f) return false;

        unsigned char magic[2];
        size_t read_bytes = fread(magic, 1, 2, f);
        fclose(f);

        return (read_bytes == 2 && magic[0] == 'B' && magic[1] == 'M');
    }

    bool loadBMP(const char* filepath, int rot) {
        FILE* f = fopen(filepath, "rb");
        if (!f) return false;

        unsigned char header[54];
        if (fread(header, 1, 54, f) != 54) {
            fclose(f);
            return false;
        }

        if (header[0] != 'B' || header[1] != 'M') {
            fclose(f);
            return false;
        }

        uint32_t dataOffset  = *(uint32_t*)&header[10];
        int32_t  width       = *(int32_t*)&header[18];
        int32_t  height      = *(int32_t*)&header[22];
        uint16_t bpp        = *(uint16_t*)&header[28];
        uint32_t compression = *(uint32_t*)&header[30];

        if ((bpp != 24 && bpp != 32) || compression != 0) {
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

                if (rot == 1) { // 90° CW
                    destX = 279 - srcY;
                    destY = x - 40;
                } else if (rot == 2) { // 180° CW
                    destX = SCREEN_W - 1 - x;
                    destY = SCREEN_H - 1 - srcY;
                } else if (rot == 3) { // 270° CW
                    destX = srcY + 40;
                    destY = 279 - x;
                }

                if (destX >= 0 && destX < SCREEN_W && destY >= 0 && destY < SCREEN_H) {
                    framebuffer[destY * SCREEN_W + destX] = color;
                }
            }
        }

        free(rowBuffer);
        fclose(f);

        lcd_blit(framebuffer, SCR_320x240_565);
        return true;
    }

public:
    void scanDirectory(const char* dir_path) {
        DIR* dir = opendir(dir_path);
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            std::string lower = name;
            for (char &c : lower) c = std::tolower(c);

            // Check files ending with .bmp or .tns
            if (lower.find(".bmp") != std::string::npos || lower.find(".tns") != std::string::npos) {
                std::string full_path = std::string(dir_path) + "/" + name;

                // Verify the file is actually a BMP image (and skip duplicate entries)
                if (isBMPFile(full_path.c_str())) {
                    bool exists = false;
                    for (const auto& existing : file_list) {
                        if (existing == full_path) { exists = true; break; }
                    }
                    if (!exists) {
                        file_list.push_back(full_path);
                    }
                }
            }
        }
        closedir(dir);
    }

    void run() {
        scanDirectory(".");
        scanDirectory("/documents");
        scanDirectory("/documents/images");
        scanDirectory("/documents/My Images");

        if (file_list.empty()) {
            for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
                framebuffer[i] = 0xF800; // Red screen: No valid BMP headers found
            }
            lcd_blit(framebuffer, SCR_320x240_565);

            while (!isKeyPressed(KEY_NSPIRE_ESC) && !isKeyPressed(KEY_NSPIRE_ENTER)) {
                msleep(50);
            }
            while (isKeyPressed(KEY_NSPIRE_ESC) || isKeyPressed(KEY_NSPIRE_ENTER)) {
                msleep(20);
            }
            return;
        }

        bool renderNeeded = true;

        while (!isKeyPressed(KEY_NSPIRE_ESC)) {
            if (renderNeeded) {
                if (!loadBMP(file_list[current_index].c_str(), rotation)) {
                    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
                        framebuffer[i] = 0x1D13; // Blue screen: Failed to parse
                    }
                    lcd_blit(framebuffer, SCR_320x240_565);
                }
                renderNeeded = false;
            }

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
                rotation = (rotation + 1) % 4; // 90° CW
                renderNeeded = true;
                msleep(200);
            } else if (isKeyPressed(KEY_NSPIRE_DOWN)) {
                rotation = (rotation + 3) % 4; // 90° CCW
                renderNeeded = true;
                msleep(200);
            }

            msleep(20);
        }

        while (isKeyPressed(KEY_NSPIRE_ESC)) {
            msleep(20);
        }
    }
};

int main() {
    ImageViewer app;
    app.run();
    return 0;
}
