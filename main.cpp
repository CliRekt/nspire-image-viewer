#include <libndls.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

// Bare-metal ARM stub for C++ destructor cleanup
extern "C" void _fini(void) {}

#define SCREEN_W 320
#define SCREEN_H 240

// Off-screen 16-bit (RGB565) frame buffer
static uint16_t framebuffer[SCREEN_W * SCREEN_H];

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

        memset(framebuffer, 0, sizeof(framebuffer));

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

                framebuffer[targetY * SCREEN_W + x] = rgbTo565(r, g, b);
            }
        }

        free(rowBuffer);
        fclose(f);

        // Render buffer directly to LCD screen
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
        scanDirectory("/documents/images");
        scanDirectory("/documents");

        if (file_list.empty()) {
            // Display a red screen if no BMPs are found
            for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
                framebuffer[i] = 0xF800;
            }
            lcd_blit(framebuffer, SCR_320x240_565);

            while (!isKeyPressed(KEY_NSPIRE_ESC)) {
                msleep(50);
            }
            return;
        }

        bool renderNeeded = true;

        while (!isKeyPressed(KEY_NSPIRE_ESC)) {
            if (renderNeeded) {
                if (!loadBMP(file_list[current_index].c_str())) {
                    // Display a dark blue screen if image fails to load
                    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
                        framebuffer[i] = 0x1D13;
                    }
                    lcd_blit(framebuffer, SCR_320x240_565);
                }

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
