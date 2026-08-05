# editor.py
import tkinter as tk
from config import *
from palette import PalettePanel
from properties import PropertiesPanel
from map_canvas import MapCanvas
from file_ops import FileOps

class TilemapEditor:
    def __init__(self, root):
        self.root = root
        self.tiles      = []
        self.tile_names = []
        self.map_data   = [[EMPTY_TILE] * MAP_W for _ in range(MAP_H)]
        self.meta_data  = [[default_meta() for _ in range(MAP_W)] 
                            for _ in range(MAP_H)]

        self._build_toolbar()

        main = tk.Frame(root, bg="#1e1e1e")
        main.pack(fill=tk.BOTH, expand=True)

        self.palette    = PalettePanel(main, self)
        self.map_canvas = MapCanvas(main, self)
        self.properties = PropertiesPanel(main, self)

        self.file_ops = FileOps(self)
        self._load_tiles()