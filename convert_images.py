#!/usr/bin/env python3
"""
Convert images to TI-Nspire compatible format (320x240 RGB565).

Usage:
    python convert_images.py input.jpg output.tns
    python convert_images.py input.png output.tns
    
Requirements:
    pip install Pillow
"""

import sys
import os
from PIL import Image

def convert_to_rgb565(image_path, output_path):
    """
    Convert an image to RGB565 16-bit format for TI-Nspire.
    
    Args:
        image_path: Path to input image (JPG, PNG, BMP, etc)
        output_path: Path to output .tns file
    """
    try:
        # Open and resize image
        img = Image.open(image_path)
        print(f"Original size: {img.size}")
        
        # Resize to exactly 320x240
        img = img.resize((320, 240), Image.Resampling.LANCZOS)
        print(f"Resized to: {img.size}")
        
        # Convert to RGB
        img = img.convert("RGB")
        
        # Get pixel data
        pixels = img.tobytes()
        
        # Convert RGB to RGB565 (16-bit)
        rgb565 = b''
        for i in range(0, len(pixels), 3):
            r = pixels[i] >> 3           # 5 bits
            g = pixels[i + 1] >> 2       # 6 bits
            b = pixels[i + 2] >> 3       # 5 bits
            
            # Pack into 16-bit value (RGB565)
            color = (r << 11) | (g << 5) | b
            
            # Write as little-endian 16-bit
            rgb565 += bytes([(color & 0xFF), (color >> 8) & 0xFF])
        
        # Write to file
        with open(output_path, "wb") as f:
            f.write(rgb565)
        
        file_size = os.path.getsize(output_path)
        expected_size = 320 * 240 * 2
        
        print(f"Converted to RGB565: {output_path}")
        print(f"File size: {file_size} bytes (expected: {expected_size})")
        
        if file_size == expected_size:
            print("✓ File size is correct!")
            return True
        else:
            print("✗ WARNING: File size mismatch!")
            return False
            
    except FileNotFoundError:
        print(f"Error: Input file '{image_path}' not found")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print("Usage: python convert_images.py <input_image> <output.tns>")
        print("\nExamples:")
        print("  python convert_images.py photo.jpg photo.tns")
        print("  python convert_images.py screenshot.png screenshot.tns")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if convert_to_rgb565(input_file, output_file):
        print("\n✓ Conversion successful! Send this file to your TI-Nspire:")
        print(f"  {output_file}")
        sys.exit(0)
    else:
        print("\n✗ Conversion failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
