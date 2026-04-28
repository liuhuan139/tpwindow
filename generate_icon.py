#!/usr/bin/env python3
from PIL import Image, ImageDraw, ImageFont

# Create a 256x256 image with transparent background
img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

# Draw a rounded rectangle background
bg_color = (74, 144, 226, 255)  # Nice blue color
draw.rounded_rectangle([16, 16, 240, 240], radius=40, fill=bg_color)

# Try to load a bold font, fall back to default if not available
try:
    # Try common system fonts
    font = ImageFont.truetype("DejaVuSans-Bold.ttf", 120)
except:
    try:
        font = ImageFont.truetype("Arial Bold.ttf", 120)
    except:
        # Use default font if no bold font found
        font = ImageFont.load_default()

# Draw "TW" text centered
text = "TW"
text_color = (255, 255, 255, 255)  # White

# Get text bounding box
bbox = draw.textbbox((0, 0), text, font=font)
text_width = bbox[2] - bbox[0]
text_height = bbox[3] - bbox[1]

# Calculate position to center the text
x = (256 - text_width) / 2
y = (256 - text_height) / 2 - bbox[1]  # Adjust for font baseline

draw.text((x, y), text, fill=text_color, font=font)

# Save as PNG
img.save("data/icons/hicolor/256x256/apps/com.example.TopWindow.png")
img.save("data/icons/com.example.TopWindow.png")

print("Icon generated successfully!")
