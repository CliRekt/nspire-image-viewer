#include <libndls.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#define SCREEN_W 320
#define SCREEN_H 240

class ImageViewer {
private:
    std::vector<std::string> file_list;
    int current_index = 0;

    uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }

    bool loadBMP(const char* filepath) {
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

        uint16_t* screen = (uint16_t*)SCREEN_BASE_ADDRESS;
        clearScreen();

        bool flipY = height > 0;
        if (height < 0) height = -height;

        int bytesPerPixel = bpp / 8;
        int rowSize = (width * bytesPerPixel + 3) & (~3);
        uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);

        for (int y = 0; y < height && y < SCREEN_H; y++) {
            if (fread(rowBuffer, 1, rowSize, f) != (size_t)rowSize) break;

            int targetY = flipY ? (height - 1 - y) : y;
            if (targetY >= SCREEN_H) continue;

            for (int x = 0; x < width && x < SCREEN_W; x++) {
                uint8_t b = rowBuffer[x * bytesPerPixel];
                uint8_t g = rowBuffer[x * bytesPerPixel + 1];
                uint8_t r = rowBuffer[x * bytesPerPixel + 2];

                screen[targetY * SCREEN_W + x] = rgbTo565(r, g, b);
            }
        }

        free(rowBuffer);
        fclose(f);
        return true;
    }

public:
    void scanDirectory(const char* dir_path) {
        DIR* dir = opendir(dir_path);
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name.length() > 4) {
                std::string ext = name.substr(name.length() - 4);
                if (ext == ".bmp" || ext == ".BMP") {
                    file_list.push_back(std::string(dir_path) + "/" + name);
                }
            }
        }
        closedir(dir);
    }

    void run() {
        assert_ndless_compatibility();
        lcd_ingame_setup();

        scanDirectory("/documents/images");
        scanDirectory("/documents");

        if (file_list.empty()) {
            clearScreen();
            drawString(10, 10, "No .bmp images found!", 0xF808, 0x0000);
            drawString(10, 35, "Copy .bmp files into /documents or", 0xFFFF, 0x0000);
            drawString(10, 55, "/documents/images on your calculator.", 0xFFFF, 0x0000);
            drawString(10, 90, "Press ESC to exit.", 0x07E0, 0x0000);
            repaint();

            while (!isKeyPressed(KEY_NSPIRE_ESC)) {
                msleep(50);
            }
            return;
        }

        bool renderNeeded = true;

        while (!isKeyPressed(KEY_NSPIRE_ESC)) {
            if (renderNeeded) {
                if (!loadBMP(file_list[current_index].c_str())) {
                    clearScreen();
                    drawString(10, 10, "Failed to load BMP:", 0xF808, 0x0000);
                    drawString(10, 30, file_list[current_index].c_str(), 0xFFFF, 0x0000);
                    drawString(10, 55, "(Must be uncompressed 24/32-bit BMP)", 0xC67A, 0x0000);
                }

                char overlay[128];
                snprintf(overlay, sizeof(overlay), "< [%d/%d] %s >", 
                         current_index + 1, (int)file_list.size(), 
                         file_list[current_index].c_str());
                drawString(5, 225, overlay, 0xFFFF, 0x0000);
                repaint();

                renderNeeded = false;
            }

            if (isKeyPressed(KEY_NSPIRE_RIGHT)) {
                current_index = (current_index + 1) % file_list.size();
                renderNeeded = true;
                msleep(200);
            } else if (isKeyPressed(KEY_NSPIRE_LEFT)) {
                current_index = (current_index - 1 + file_list.size()) % file_list.size();
                renderNeeded = true;
                msleep(200);
            }

            msleep(20);
        }
    }
};

int main() {
    ImageViewer app;
    app.run();
    return 0;
}
