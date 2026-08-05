"""
tilemap_editor.py — Tile map editor for embedded game development
Usage: python tilemap_editor.py

Features:
- Load tiles from converted/ folder
- Paint tiles onto a 20x15 grid (matches 320x240 landscape display at 2x scale)
- Per-tile metadata: flip X, flip Y, solid, type tag, passthrough
- Save/load maps as JSON
- Export map + metadata as C arrays
- Export PNG preview
"""

import os
import json
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk, ImageOps

# --- Config ---
MAP_W        = 20
MAP_H        = 15
TILE_SIZE    = 8
DISPLAY_SIZE = 32
TILES_DIR    = "converted"
EMPTY_TILE   = 0

TILE_TYPES = ["none", "platform", "wall", "hazard", "collectible", "decoration", "spawn"]

# Default metadata for a tile cell
def default_meta():
    return {
        "flip_x":      False,
        "flip_y":      False,
        "solid":       False,
        "passthrough": False,
        "type":        "none"
    }


class TilemapEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Tilemap Editor")
        self.root.configure(bg="#1e1e1e")

        self.tiles        = []
        self.tile_names   = []
        self.tile_images  = []
        self.selected_tile = 1
        self.drawing      = False
        self.erasing      = False
        self.current_file = None
        self.selected_cell = None  # (col, row) of currently inspected tile

        # map_data[row][col] = tile index
        self.map_data = [[EMPTY_TILE] * MAP_W for _ in range(MAP_H)]
        # meta_data[row][col] = metadata dict
        self.meta_data = [[default_meta() for _ in range(MAP_W)] for _ in range(MAP_H)]

        self._build_ui()
        self._load_tiles()
        self._draw_map()

    # -------------------------------------------------------------------------
    # UI
    # -------------------------------------------------------------------------
    def _build_ui(self):
        toolbar = tk.Frame(self.root, bg="#2d2d2d", pady=4)
        toolbar.pack(side=tk.TOP, fill=tk.X)

        btn = {"bg": "#3c3c3c", "fg": "#ffffff", "relief": tk.FLAT,
               "padx": 10, "pady": 4, "cursor": "hand2",
               "activebackground": "#505050", "activeforeground": "#ffffff"}

        tk.Button(toolbar, text="New",         **btn, command=self._new_map).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Open",        **btn, command=self._open_map).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Save",        **btn, command=self._save_map).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Save As",     **btn, command=self._save_map_as).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Export C",    **btn, command=self._export_c).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Export PNG",  **btn, command=self._export_png).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="Reload Tiles",**btn, command=self._load_tiles).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="+ Col", **btn, command=self._add_col).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="- Col", **btn, command=self._remove_col).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="+ Row", **btn, command=self._add_row).pack(side=tk.LEFT, padx=4)
        tk.Button(toolbar, text="- Row", **btn, command=self._remove_row).pack(side=tk.LEFT, padx=4)


        self.status_var = tk.StringVar(value="Ready")
        tk.Label(toolbar, textvariable=self.status_var, bg="#2d2d2d",
                 fg="#aaaaaa", font=("Consolas", 10)).pack(side=tk.RIGHT, padx=10)

        # Main 3-pane layout
        main = tk.Frame(self.root, bg="#1e1e1e")
        main.pack(fill=tk.BOTH, expand=True)

        # LEFT — tile palette
        palette_frame = tk.Frame(main, bg="#252525", width=130)
        palette_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(8,0), pady=8)
        palette_frame.pack_propagate(False)

        tk.Label(palette_frame, text="Tiles", bg="#252525", fg="#cccccc",
                 font=("Consolas", 11, "bold")).pack(pady=(8,4))

        palette_scroll_frame = tk.Frame(palette_frame, bg="#252525")
        palette_scroll_frame.pack(fill=tk.BOTH, expand=True)

        self.palette_canvas = tk.Canvas(palette_scroll_frame, bg="#252525",
                                         highlightthickness=0, width=114)
        pal_sb = ttk.Scrollbar(palette_scroll_frame, orient=tk.VERTICAL,
                                command=self.palette_canvas.yview)
        self.palette_canvas.configure(yscrollcommand=pal_sb.set)
        pal_sb.pack(side=tk.RIGHT, fill=tk.Y)
        self.palette_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.palette_inner = tk.Frame(self.palette_canvas, bg="#252525")
        self.palette_canvas.create_window((0,0), window=self.palette_inner, anchor="nw")
        self.palette_inner.bind("<Configure>", lambda e: self.palette_canvas.configure(
            scrollregion=self.palette_canvas.bbox("all")))

        # CENTER — map canvas
        canvas_frame = tk.Frame(main, bg="#1e1e1e")
        canvas_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=8, pady=8)

        self.h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL)
        self.v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL)
        self.h_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        self.v_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.canvas = tk.Canvas(canvas_frame, bg="#111111", highlightthickness=0,
                                 xscrollcommand=self.h_scroll.set,
                                 yscrollcommand=self.v_scroll.set)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.h_scroll.config(command=self.canvas.xview)
        self.v_scroll.config(command=self.canvas.yview)
        self.canvas.config(scrollregion=(0, 0, MAP_W*DISPLAY_SIZE, MAP_H*DISPLAY_SIZE))

        self.canvas.bind("<ButtonPress-1>",   self._on_press_left)
        self.canvas.bind("<ButtonPress-3>",   self._on_press_right)
        self.canvas.bind("<B1-Motion>",       self._on_drag_left)
        self.canvas.bind("<B3-Motion>",       self._on_drag_right)
        self.canvas.bind("<ButtonRelease-1>", self._on_release)
        self.canvas.bind("<ButtonRelease-3>", self._on_release)
        self.canvas.bind("<Motion>",          self._on_hover)

        # RIGHT — properties panel
        self.props_frame = tk.Frame(main, bg="#252525", width=200)
        self.props_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(0,8), pady=8)
        self.props_frame.pack_propagate(False)

        tk.Label(self.props_frame, text="Tile Properties", bg="#252525", fg="#cccccc",
                 font=("Consolas", 11, "bold")).pack(pady=(10,6), padx=8, anchor="w")

        tk.Frame(self.props_frame, bg="#3a3a3a", height=1).pack(fill=tk.X, padx=8)

        self.props_tile_label = tk.Label(self.props_frame, text="No tile selected",
                                          bg="#252525", fg="#888888",
                                          font=("Consolas", 9), wraplength=180)
        self.props_tile_label.pack(pady=(8,4), padx=8, anchor="w")

        self.props_preview = tk.Label(self.props_frame, bg="#252525")
        self.props_preview.pack(pady=4)

        tk.Frame(self.props_frame, bg="#3a3a3a", height=1).pack(fill=tk.X, padx=8, pady=4)

        lbl = {"bg": "#252525", "fg": "#cccccc", "font": ("Consolas", 10), "anchor": "w"}
        chk = {"bg": "#252525", "fg": "#cccccc", "font": ("Consolas", 10),
               "activebackground": "#252525", "activeforeground": "#cccccc",
               "selectcolor": "#3a3a5c", "cursor": "hand2"}

        # Flip X
        self.flip_x_var = tk.BooleanVar()
        tk.Label(self.props_frame, text="Flip X", **lbl).pack(padx=8, anchor="w")
        tk.Checkbutton(self.props_frame, text="Mirror horizontal",
                       variable=self.flip_x_var, **chk,
                       command=self._apply_meta).pack(padx=16, anchor="w")

        tk.Frame(self.props_frame, bg="#2a2a2a", height=1).pack(fill=tk.X, padx=8, pady=4)

        # Flip Y
        self.flip_y_var = tk.BooleanVar()
        tk.Label(self.props_frame, text="Flip Y", **lbl).pack(padx=8, anchor="w")
        tk.Checkbutton(self.props_frame, text="Mirror vertical",
                       variable=self.flip_y_var, **chk,
                       command=self._apply_meta).pack(padx=16, anchor="w")

        tk.Frame(self.props_frame, bg="#2a2a2a", height=1).pack(fill=tk.X, padx=8, pady=4)

        # Solid
        self.solid_var = tk.BooleanVar()
        tk.Label(self.props_frame, text="Collision", **lbl).pack(padx=8, anchor="w")
        tk.Checkbutton(self.props_frame, text="Solid (blocks movement)",
                       variable=self.solid_var, **chk,
                       command=self._apply_meta).pack(padx=16, anchor="w")

        tk.Frame(self.props_frame, bg="#2a2a2a", height=1).pack(fill=tk.X, padx=8, pady=4)

        # Passthrough
        self.passthrough_var = tk.BooleanVar()
        tk.Label(self.props_frame, text="Passthrough", **lbl).pack(padx=8, anchor="w")
        tk.Checkbutton(self.props_frame, text="Jump through, land on top",
                       variable=self.passthrough_var, **chk,
                       command=self._apply_meta).pack(padx=16, anchor="w")

        tk.Frame(self.props_frame, bg="#2a2a2a", height=1).pack(fill=tk.X, padx=8, pady=4)

        # Tile type
        tk.Label(self.props_frame, text="Tile Type", **lbl).pack(padx=8, anchor="w")
        self.type_var = tk.StringVar(value="none")
        type_menu = ttk.Combobox(self.props_frame, textvariable=self.type_var,
                                  values=TILE_TYPES, state="readonly", width=18)
        type_menu.pack(padx=16, pady=4, anchor="w")
        type_menu.bind("<<ComboboxSelected>>", lambda e: self._apply_meta())

        tk.Frame(self.props_frame, bg="#3a3a3a", height=1).pack(fill=tk.X, padx=8, pady=8)

        # Apply to all same tiles button
        btn2 = {"bg": "#3c3c3c", "fg": "#ffffff", "relief": tk.FLAT,
                "padx": 8, "pady": 4, "cursor": "hand2",
                "activebackground": "#505050", "activeforeground": "#ffffff",
                "font": ("Consolas", 9)}
        tk.Button(self.props_frame, text="Apply to all same tiles",
                  **btn2, command=self._apply_to_all_same).pack(padx=8, fill=tk.X)

        # Bottom coord bar
        self.coord_var = tk.StringVar(value="")
        tk.Label(self.root, textvariable=self.coord_var, bg="#1e1e1e",
                 fg="#555555", font=("Consolas", 9), anchor="w").pack(
                 side=tk.BOTTOM, fill=tk.X, padx=8)

    # -------------------------------------------------------------------------
    # Tile loading + palette
    # -------------------------------------------------------------------------
    def _load_tiles(self):
        self.tiles      = []
        self.tile_names = []
        self.tile_images = []

        empty = Image.new("L", (TILE_SIZE, TILE_SIZE), 0)
        self.tiles.append(empty)
        self.tile_names.append("empty")

        if os.path.isdir(TILES_DIR):
            for f in sorted(os.listdir(TILES_DIR)):
                if f.endswith(".png"):
                    img = Image.open(os.path.join(TILES_DIR, f)).convert("L")
                    img = img.resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
                    self.tiles.append(img)
                    self.tile_names.append(os.path.splitext(f)[0])

        self._build_palette()
        self._draw_map()
        self.status_var.set(f"Loaded {len(self.tiles)-1} tiles from {TILES_DIR}/")

    def _build_palette(self):
        for w in self.palette_inner.winfo_children():
            w.destroy()
        self.tile_images = []

        for i, tile in enumerate(self.tiles):
            scaled = tile.resize((40, 40), Image.NEAREST)
            photo  = ImageTk.PhotoImage(scaled)
            self.tile_images.append(photo)

            frame = tk.Frame(self.palette_inner, bg="#252525", cursor="hand2")
            frame.pack(fill=tk.X, padx=4, pady=2)

            lbl_img = tk.Label(frame, image=photo, bg="#252525", relief=tk.FLAT, bd=0)
            lbl_img.pack(side=tk.LEFT, padx=4)

            lbl_txt = tk.Label(frame, text=self.tile_names[i][:10],
                                bg="#252525", fg="#aaaaaa", font=("Consolas", 8))
            lbl_txt.pack(side=tk.LEFT)

            idx = i
            for w in (frame, lbl_img, lbl_txt):
                w.bind("<Button-1>", lambda e, n=idx: self._select_tile(n))

        self._highlight_selected()

    def _select_tile(self, idx):
        self.selected_tile = idx
        self._highlight_selected()

    def _highlight_selected(self):
        for i, child in enumerate(self.palette_inner.winfo_children()):
            col = "#3a3a5c" if i == self.selected_tile else "#252525"
            child.config(bg=col)
            for w in child.winfo_children():
                w.config(bg=col)

    # -------------------------------------------------------------------------
    # Map drawing
    # -------------------------------------------------------------------------
    def _draw_map(self):
        self.canvas.delete("all")
        d = DISPLAY_SIZE

        self._canvas_tiles = []
        for tile in self.tiles:
            scaled = tile.resize((d, d), Image.NEAREST)
            photo  = ImageTk.PhotoImage(scaled)
            self._canvas_tiles.append(photo)

        for row in range(MAP_H):
            for col in range(MAP_W):
                self._redraw_tile(col, row)

        for col in range(0, MAP_W+1, 5):
            self.canvas.create_line(col*d, 0, col*d, MAP_H*d, fill="#2a2a2a")
        for row in range(0, MAP_H+1, 5):
            self.canvas.create_line(0, row*d, MAP_W*d, row*d, fill="#2a2a2a")

    def _redraw_tile(self, col, row):
        d   = DISPLAY_SIZE
        x   = col * d
        y   = row * d
        tag = f"tile_{col}_{row}"
        self.canvas.delete(tag)

        tile_idx = self.map_data[row][col]
        meta     = self.meta_data[row][col]

        if tile_idx == EMPTY_TILE:
            self.canvas.create_rectangle(x, y, x+d, y+d,
                                          fill="#111111", outline="#1a1a1a",
                                          width=0.5, tags=tag)
        else:
            # Apply flips to preview
            img = self.tiles[tile_idx] if tile_idx < len(self.tiles) else self.tiles[0]
            if meta["flip_x"]:
                img = ImageOps.mirror(img)
            if meta["flip_y"]:
                img = ImageOps.flip(img)
            scaled = img.resize((d, d), Image.NEAREST)
            photo  = ImageTk.PhotoImage(scaled)
            # Store reference to prevent GC
            if not hasattr(self, "_live_photos"):
                self._live_photos = {}
            self._live_photos[tag] = photo

            self.canvas.create_image(x, y, anchor=tk.NW, image=photo, tags=tag)
            self.canvas.create_rectangle(x, y, x+d, y+d,
                                          fill="", outline="#1a1a1a",
                                          width=0.5, tags=tag)

            # Overlay indicators for metadata
            if meta["solid"]:
                self.canvas.create_rectangle(x+1, y+1, x+6, y+6,
                                              fill="#4444cc", outline="", tags=tag)
            if meta["passthrough"]:
                self.canvas.create_rectangle(x+1, y+d-7, x+6, y+d-2,
                                              fill="#44cc44", outline="", tags=tag)
            if meta["type"] != "none":
                colors = {"platform":"#cc8844","wall":"#888888","hazard":"#cc4444",
                          "collectible":"#cccc44","decoration":"#44cccc","spawn":"#cc44cc"}
                c = colors.get(meta["type"], "#ffffff")
                self.canvas.create_rectangle(x+d-7, y+1, x+d-2, y+6,
                                              fill=c, outline="", tags=tag)

        # Highlight selected cell
        if self.selected_cell == (col, row):
            self.canvas.create_rectangle(x+1, y+1, x+d-1, y+d-1,
                                          fill="", outline="#ffffff",
                                          width=2, tags=tag)

    def _paint_tile(self, event, tile_idx):
        cx  = self.canvas.canvasx(event.x)
        cy  = self.canvas.canvasy(event.y)
        col = int(cx // DISPLAY_SIZE)
        row = int(cy // DISPLAY_SIZE)
        if 0 <= col < MAP_W and 0 <= row < MAP_H:
            if self.map_data[row][col] != tile_idx:
                self.map_data[row][col] = tile_idx
                if tile_idx == EMPTY_TILE:
                    self.meta_data[row][col] = default_meta()
                self._redraw_tile(col, row)

    def _select_cell(self, event):
        cx  = self.canvas.canvasx(event.x)
        cy  = self.canvas.canvasy(event.y)
        col = int(cx // DISPLAY_SIZE)
        row = int(cy // DISPLAY_SIZE)
        if 0 <= col < MAP_W and 0 <= row < MAP_H:
            old = self.selected_cell
            self.selected_cell = (col, row)
            if old:
                self._redraw_tile(old[0], old[1])
            self._redraw_tile(col, row)
            self._show_props(col, row)

    def _show_props(self, col, row):
        tile_idx = self.map_data[row][col]
        meta     = self.meta_data[row][col]
        name     = self.tile_names[tile_idx] if tile_idx < len(self.tile_names) else "?"

        self.props_tile_label.config(
            text=f"Cell ({col}, {row})\nTile: {name} [#{tile_idx}]")

        # Show tile preview with flips applied
        img = self.tiles[tile_idx] if tile_idx < len(self.tiles) else self.tiles[0]
        if meta["flip_x"]: img = ImageOps.mirror(img)
        if meta["flip_y"]: img = ImageOps.flip(img)
        scaled = img.resize((64, 64), Image.NEAREST)
        photo  = ImageTk.PhotoImage(scaled)
        self.props_preview.config(image=photo)
        self.props_preview._photo = photo

        # Load meta into controls
        self.flip_x_var.set(meta["flip_x"])
        self.flip_y_var.set(meta["flip_y"])
        self.solid_var.set(meta["solid"])
        self.passthrough_var.set(meta["passthrough"])
        self.type_var.set(meta["type"])

    def _apply_meta(self):
        if not self.selected_cell:
            return
        col, row = self.selected_cell
        self.meta_data[row][col] = {
            "flip_x":      self.flip_x_var.get(),
            "flip_y":      self.flip_y_var.get(),
            "solid":       self.solid_var.get(),
            "passthrough": self.passthrough_var.get(),
            "type":        self.type_var.get()
        }
        self._redraw_tile(col, row)
        # Refresh preview
        self._show_props(col, row)

    def _apply_to_all_same(self):
        if not self.selected_cell:
            return
        col, row = self.selected_cell
        tile_idx = self.map_data[row][col]
        meta     = {
            "flip_x":      self.flip_x_var.get(),
            "flip_y":      self.flip_y_var.get(),
            "solid":       self.solid_var.get(),
            "passthrough": self.passthrough_var.get(),
            "type":        self.type_var.get()
        }
        count = 0
        for r in range(MAP_H):
            for c in range(MAP_W):
                if self.map_data[r][c] == tile_idx:
                    self.meta_data[r][c] = dict(meta)
                    self._redraw_tile(c, r)
                    count += 1
        self.status_var.set(f"Applied metadata to {count} tiles of type '{self.tile_names[tile_idx]}'")


    # -------------------------------------------------------------------------
    # Expanding/Shrinking Map
    # -------------------------------------------------------------------------

    def _add_col(self):
        global MAP_W
        MAP_W += 1
        for row in range(MAP_H):
            self.map_data[row].append(EMPTY_TILE)
            self.meta_data[row].append(default_meta())
        self.canvas.config(scrollregion=(0, 0, MAP_W*DISPLAY_SIZE, MAP_H*DISPLAY_SIZE))
        self._draw_map()
        self.status_var.set(f"Map is now {MAP_W}x{MAP_H}")

    def _remove_col(self):
        global MAP_W
        if MAP_W <= 1:
            return
        MAP_W -= 1
        for row in range(MAP_H):
            self.map_data[row].pop()
            self.meta_data[row].pop()
        self.canvas.config(scrollregion=(0, 0, MAP_W*DISPLAY_SIZE, MAP_H*DISPLAY_SIZE))
        self._draw_map()
        self.status_var.set(f"Map is now {MAP_W}x{MAP_H}")

    def _add_row(self):
        global MAP_H
        MAP_H += 1
        self.map_data.append([EMPTY_TILE] * MAP_W)
        self.meta_data.append([default_meta() for _ in range(MAP_W)])
        self.canvas.config(scrollregion=(0, 0, MAP_W*DISPLAY_SIZE, MAP_H*DISPLAY_SIZE))
        self._draw_map()
        self.status_var.set(f"Map is now {MAP_W}x{MAP_H}")

    def _remove_row(self):
        global MAP_H
        if MAP_H <= 1:
            return
        MAP_H -= 1
        self.map_data.pop()
        self.meta_data.pop()
        self.canvas.config(scrollregion=(0, 0, MAP_W*DISPLAY_SIZE, MAP_H*DISPLAY_SIZE))
        self._draw_map()
        self.status_var.set(f"Map is now {MAP_W}x{MAP_H}")

    # -------------------------------------------------------------------------
    # Canvas events
    # -------------------------------------------------------------------------
    def _on_press_left(self, event):
        self.drawing = True
        self._paint_tile(event, self.selected_tile)
        self._select_cell(event)

    def _on_press_right(self, event):
        self.erasing = True
        self._paint_tile(event, EMPTY_TILE)

    def _on_drag_left(self, event):
        if self.drawing:
            self._paint_tile(event, self.selected_tile)

    def _on_drag_right(self, event):
        if self.erasing:
            self._paint_tile(event, EMPTY_TILE)

    def _on_release(self, event):
        self.drawing = False
        self.erasing = False

    def _on_hover(self, event):
        cx  = self.canvas.canvasx(event.x)
        cy  = self.canvas.canvasy(event.y)
        col = int(cx // DISPLAY_SIZE)
        row = int(cy // DISPLAY_SIZE)
        if 0 <= col < MAP_W and 0 <= row < MAP_H:
            tile_idx = self.map_data[row][col]
            name = self.tile_names[tile_idx] if tile_idx < len(self.tile_names) else "?"
            self.coord_var.set(
                f"  col {col}, row {row}  |  tile: {tile_idx} ({name})  "
                f"|  selected: {self.tile_names[self.selected_tile]}")

    # -------------------------------------------------------------------------
    # File operations
    # -------------------------------------------------------------------------
    def _new_map(self):
        if messagebox.askyesno("New Map", "Clear the current map?"):
            self.map_data  = [[EMPTY_TILE]    * MAP_W for _ in range(MAP_H)]
            self.meta_data = [[default_meta() for _ in range(MAP_W)] for _ in range(MAP_H)]
            self.current_file  = None
            self.selected_cell = None
            self._draw_map()
            self.status_var.set("New map created")

    def _open_map(self):
        path = filedialog.askopenfilename(
            title="Open Map", filetypes=[("Map files", "*.map"), ("All files", "*.*")])
        if not path:
            return
        with open(path, "r") as f:
            data = json.load(f)
        self.map_data  = data["map"]
        self.meta_data = data.get("meta", [[default_meta() for _ in range(MAP_W)]
                                            for _ in range(MAP_H)])
        self.current_file = path
        self._draw_map()
        self.status_var.set(f"Opened {os.path.basename(path)}")

    def _save_map(self):
        if self.current_file:
            self._write_map(self.current_file)
        else:
            self._save_map_as()

    def _save_map_as(self):
        path = filedialog.asksaveasfilename(
            title="Save Map", defaultextension=".map",
            filetypes=[("Map files", "*.map"), ("All files", "*.*")])
        if path:
            self._write_map(path)
            self.current_file = path

    def _write_map(self, path):
        data = {
            "width":  MAP_W,
            "height": MAP_H,
            "tiles":  self.tile_names,
            "map":    self.map_data,
            "meta":   self.meta_data
        }
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
        self.status_var.set(f"Saved {os.path.basename(path)}")

    # -------------------------------------------------------------------------
    # Export
    # -------------------------------------------------------------------------
    def _export_c(self):
        path = filedialog.asksaveasfilename(
            title="Export C Array", defaultextension=".c",
            filetypes=[("C files", "*.c"), ("All files", "*.*")])
        if not path:
            return

        name = os.path.splitext(os.path.basename(path))[0]
        h_path = path.replace(".c", ".h")

        # Build flags byte per cell:
        # bit 0 = flip_x
        # bit 1 = flip_y
        # bit 2 = solid
        # bit 3 = passthrough
        # bits 7:4 = type index (0-7)

        type_index = {t: i for i, t in enumerate(TILE_TYPES)}

        c = []
        c.append("/* Auto-generated by tilemap_editor.py — do not edit manually */")
        c.append(f'#include "{name}.h"')
        c.append("")
        c.append(f"/* Tile indices: 0=empty, 1..N = tile from tiles.h */")
        c.append(f"const uint8_t {name}_tiles[{MAP_H}][{MAP_W}] = {{")
        for row in self.map_data:
            c.append("    { " + ", ".join(f"0x{v:02X}" for v in row) + " },")
        c.append("};")
        c.append("")
        c.append(f"/* Flags per tile: bit0=flipX bit1=flipY bit2=solid bit3=passthrough bits7:4=type */")
        c.append(f"const uint8_t {name}_flags[{MAP_H}][{MAP_W}] = {{")
        for row_idx, row in enumerate(self.meta_data):
            flags = []
            for meta in row:
                f = 0
                if meta["flip_x"]:      f |= (1 << 0)
                if meta["flip_y"]:      f |= (1 << 1)
                if meta["solid"]:       f |= (1 << 2)
                if meta["passthrough"]: f |= (1 << 3)
                f |= (type_index.get(meta["type"], 0) << 4)
                flags.append(f)
            c.append("    { " + ", ".join(f"0x{v:02X}" for v in flags) + " },")
        c.append("};")

        h = []
        h.append("/* Auto-generated by tilemap_editor.py — do not edit manually */")
        h.append(f"#ifndef {name.upper()}_H")
        h.append(f"#define {name.upper()}_H")
        h.append("")
        h.append("#include <stdint.h>")
        h.append("")
        h.append(f"#define MAP_W {MAP_W}")
        h.append(f"#define MAP_H {MAP_H}")
        h.append("")
        h.append("/* Flag bit masks */")
        h.append("#define TILE_FLAG_FLIP_X      0x01")
        h.append("#define TILE_FLAG_FLIP_Y      0x02")
        h.append("#define TILE_FLAG_SOLID       0x04")
        h.append("#define TILE_FLAG_PASSTHROUGH 0x08")
        h.append("#define TILE_TYPE_MASK        0xF0")
        h.append("#define TILE_TYPE_SHIFT       4")
        h.append("")
        h.append("/* Tile type values */")
        for i, t in enumerate(TILE_TYPES):
            h.append(f"#define TILE_TYPE_{t.upper():12s} {i}")
        h.append("")
        h.append(f"extern const uint8_t {name}_tiles[{MAP_H}][{MAP_W}];")
        h.append(f"extern const uint8_t {name}_flags[{MAP_H}][{MAP_W}];")
        h.append("")
        h.append(f"#endif /* {name.upper()}_H */")

        with open(path,   "w") as f: f.write("\n".join(c) + "\n")
        with open(h_path, "w") as f: f.write("\n".join(h) + "\n")
        self.status_var.set(f"Exported {os.path.basename(path)} + .h")

    def _export_png(self):
        path = filedialog.asksaveasfilename(
            title="Export PNG", defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("All files", "*.*")])
        if not path:
            return

        img = Image.new("L", (MAP_W * TILE_SIZE, MAP_H * TILE_SIZE), 0)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tile_idx = self.map_data[row][col]
                if tile_idx != EMPTY_TILE and tile_idx < len(self.tiles):
                    meta = self.meta_data[row][col]
                    t = self.tiles[tile_idx].resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
                    if meta["flip_x"]: t = ImageOps.mirror(t)
                    if meta["flip_y"]: t = ImageOps.flip(t)
                    img.paste(t, (col * TILE_SIZE, row * TILE_SIZE))

        preview = img.resize((MAP_W * TILE_SIZE * 3, MAP_H * TILE_SIZE * 3), Image.NEAREST)
        preview.save(path)
        self.status_var.set(f"Exported PNG to {os.path.basename(path)}")


if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("1200x750")
    app = TilemapEditor(root)
    root.mainloop()