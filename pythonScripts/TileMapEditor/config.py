# config.py
MAP_W        = 20
MAP_H        = 15
TILE_SIZE    = 8
DISPLAY_SIZE = 32
TILES_DIR    = "converted"
EMPTY_TILE   = 0
TILE_TYPES   = ["none", "platform", "wall", "hazard", 
                "collectible", "decoration", "spawn"]

def default_meta():
    return {
        "flip_x":      False,
        "flip_y":      False,
        "solid":       False,
        "passthrough": False,
        "type":        "none"
    }