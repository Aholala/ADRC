#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Config File Generator - 配置文件生成工具
通过GUI界面管理CMake模块配置，读写rule.json文件。
"""

import pygame
import json
import os
import sys
import tkinter as tk
from tkinter import filedialog

# ============================================================
# 初始化
# ============================================================
pygame.init()

# 窗口设置
WIN_WIDTH, WIN_HEIGHT = 960, 660
screen = pygame.display.set_mode((WIN_WIDTH, WIN_HEIGHT))
pygame.display.set_caption("Config File Generator - ACE_ECF_26")

# 字体设置
try:
    FONT_PATH = "C:/Windows/Fonts/msyh.ttc"
    font_normal = pygame.font.Font(FONT_PATH, 16)
    font_small = pygame.font.Font(FONT_PATH, 13)
    font_title = pygame.font.Font(FONT_PATH, 22)
    font_bold = pygame.font.Font(FONT_PATH, 18)
except Exception:
    font_normal = pygame.font.SysFont("simhei", 16)
    font_small = pygame.font.SysFont("simhei", 13)
    font_title = pygame.font.SysFont("simhei", 22)
    font_bold = pygame.font.SysFont("simhei", 18)

# ============================================================
# 颜色常量
# ============================================================
BG_COLOR = (45, 45, 50)
PANEL_COLOR = (55, 55, 60)
TAB_ACTIVE = (70, 130, 200)
TAB_INACTIVE = (65, 65, 70)
TAB_HOVER = (80, 80, 85)
BUTTON_COLOR = (70, 130, 200)
BUTTON_HOVER = (90, 150, 220)
BUTTON_DELETE = (200, 60, 60)
BUTTON_DELETE_HOVER = (230, 70, 70)
BUTTON_GREEN = (60, 160, 80)
BUTTON_GREEN_HOVER = (70, 180, 90)
TEXT_COLOR = (220, 220, 220)
TEXT_DIM = (150, 150, 150)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
INPUT_BG = (40, 40, 45)
INPUT_BORDER = (100, 100, 105)
INPUT_BORDER_ACTIVE = (70, 130, 200)
ITEM_BG = (60, 60, 65)
ITEM_HOVER = (75, 75, 80)
ITEM_SELECTED = (70, 100, 160)
SEPARATOR = (80, 80, 85)
SCROLLBAR_BG = (50, 50, 55)
SCROLLBAR_THUMB = (100, 100, 110)

# ============================================================
# 路径配置
# ============================================================
# 兼容 PyInstaller 打包后的资源路径
if getattr(sys, 'frozen', False):
    _RESOURCE_DIR = sys._MEIPASS
    _USER_DIR = os.path.dirname(sys.executable)
else:
    _script_dir = os.path.dirname(os.path.abspath(__file__))
    if os.path.basename(_script_dir) == "ConfigFileGenerate":
        _USER_DIR = os.path.dirname(_script_dir)
    else:
        _USER_DIR = _script_dir
    _RESOURCE_DIR = _USER_DIR

CFG_DIR = os.path.join(_USER_DIR, "ConfigFileGenerate")
RES_CFG_DIR = os.path.join(_RESOURCE_DIR, "ConfigFileGenerate")
MAINDATA_USER = os.path.join(CFG_DIR, "MainDate")
MAINDATA_RES = os.path.join(RES_CFG_DIR, "MainDate")
USERCONFIG_DIR = os.path.join(CFG_DIR, "UserConfig")

# rule.json 始终读写于 MainDate
RULE_JSON_PATH = os.path.join(MAINDATA_USER, "rule.json")

# 模版文件路径（可在运行时通过按钮更改）
CMAKE_EXAMPLE_USER = os.path.join(MAINDATA_USER, "CMakeLists.txt.example")
CMAKE_EXAMPLE_RES = os.path.join(MAINDATA_RES, "CMakeLists.txt.example")
CONFIG_EXAMPLE_USER = os.path.join(MAINDATA_USER, "ecf_configList.h.example")
CONFIG_EXAMPLE_RES = os.path.join(MAINDATA_RES, "ecf_configList.h.example")

# ============================================================
# 数据结构
# ============================================================
def load_rules():
    """加载 rule.json，若文件为空或不存在则初始化空结构"""
    if os.path.exists(RULE_JSON_PATH):
        try:
            with open(RULE_JSON_PATH, 'r', encoding='utf-8') as f:
                content = f.read().strip()
                if content:
                    data = json.loads(content)
                    if "modules" not in data:
                        data["modules"] = []
                    return data
        except (json.JSONDecodeError, IOError):
            pass
    return {"modules": []}


def save_rules(data):
    """保存 rule.json"""
    os.makedirs(MAINDATA_USER, exist_ok=True)
    with open(RULE_JSON_PATH, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def read_example_file(user_path, fallback_path=None):
    """读取示例文件内容，优先读取用户副本，不存在则回退到内嵌资源"""
    # 优先用户副本
    if os.path.exists(user_path):
        with open(user_path, 'r', encoding='utf-8') as f:
            return f.read()
    # 回退内嵌资源
    if fallback_path and os.path.exists(fallback_path):
        with open(fallback_path, 'r', encoding='utf-8') as f:
            return f.read()
    return ""


# ============================================================
# 工具函数
# ============================================================
def select_folder():
    """使用 tkinter 打开文件夹选择对话框，返回相对路径"""
    root = tk.Tk()
    root.withdraw()
    root.attributes('-topmost', True)
    folder = filedialog.askdirectory(
        title="选择要包含的文件夹",
        initialdir=os.getcwd()
    )
    root.destroy()
    if folder:
        try:
            return os.path.relpath(folder, os.getcwd()).replace("\\", "/")
        except ValueError:
            return folder.replace("\\", "/")
    return None


def draw_text(surface, text, x, y, font, color=TEXT_COLOR, align_left=True):
    """绘制文字"""
    if not text:
        return
    rendered = font.render(str(text), True, color)
    if align_left:
        surface.blit(rendered, (x, y))
    else:
        rect = rendered.get_rect()
        surface.blit(rendered, (x - rect.width, y))


def draw_rect(surface, rect, color, border_radius=4):
    """绘制圆角矩形"""
    x, y, w, h = rect
    if w <= 0 or h <= 0:
        return
    pygame.draw.rect(surface, color, rect, border_radius=border_radius)


def draw_rect_outline(surface, rect, color, border_radius=4, width=2):
    """绘制圆角矩形边框"""
    x, y, w, h = rect
    if w <= 0 or h <= 0:
        return
    pygame.draw.rect(surface, color, rect, border_radius=border_radius, width=width)


def clip_text(text, font, max_width):
    """裁剪文本以适应宽度，超出部分显示..."""
    if not text:
        return ""
    if font.render(text, True, (0, 0, 0)).get_width() <= max_width:
        return text
    for i in range(len(text) - 1, 0, -1):
        test = text[:i] + "..."
        if font.render(test, True, (0, 0, 0)).get_width() <= max_width:
            return test
    return "..."


def wrap_text(text, font, max_width):
    """将文本按字体宽度逐字折行，返回字符串列表"""
    if not text:
        return []
    lines = []
    current = ""
    for ch in text:
        test = current + ch
        if font.render(test, True, (0, 0, 0)).get_width() <= max_width:
            current = test
        else:
            if current:
                lines.append(current)
            current = ch
    if current:
        lines.append(current)
    return lines


def cursor_in_wrapped(text, font, max_width, cursor_pos):
    """返回 cursor_pos 在折行后的 (行索引, 列索引)"""
    lines = wrap_text(text, font, max_width)
    remaining = cursor_pos
    for li, line in enumerate(lines):
        if remaining <= len(line):
            return li, remaining
        remaining -= len(line)
    last = len(lines) - 1 if lines else 0
    last_col = len(lines[-1]) if lines else 0
    return last, last_col


def collect_text_tooltip(full_text, font, clip_width, item_rect, container_rect):
    """如果文本被裁切且鼠标悬停，返回 tooltip 信息 (x,y,w,h,wrapped_lines)，否则返回 None"""
    clipped = clip_text(full_text, font, clip_width)
    if full_text == clipped or not full_text:
        return None
    if not item_rect.collidepoint(pygame.mouse.get_pos()):
        return None
    mx, my = pygame.mouse.get_pos()
    # 折行以便 tooltip 显示完整文本
    max_tip_w = min(420, font.render(full_text, True, (0, 0, 0)).get_width() + 24)
    lines = wrap_text(full_text, font, max_tip_w - 20)
    tip_w = max((font.render(l, True, (0, 0, 0)).get_width() for l in lines), default=80) + 20
    tip_h = len(lines) * 20 + 16
    tip_x = mx + 16
    if tip_x + tip_w > container_rect.right:
        tip_x = mx - tip_w - 8
    tip_y = min(my, container_rect.bottom - tip_h - 4)
    tip_y = max(tip_y, container_rect.y + 4)
    return (tip_x, tip_y, tip_w, tip_h, lines)


# ============================================================
# UI 元素类
# ============================================================
class TextInput:
    """文本输入框，支持 IME 中文输入和字符验证器"""

    def __init__(self, x, y, w, h, placeholder="", font_obj=None, validator=None,
                 multi_line=False, enter_newline=False):
        """
        validator: 可选函数，接受单个字符，返回 bool。
                   返回 False 时拒绝该字符输入。
                   例如: validator=lambda c: c.isascii() and (c.isalpha() or c == '_')
        multi_line: 启用多行模式，文本自动折行显示，支持上下箭头导航
        enter_newline: Enter 键插入换行符而非失焦（用于文本编辑器）
        """
        self.rect = pygame.Rect(x, y, w, h)
        self.text = ""
        self.placeholder = placeholder
        self.font = font_obj or font_normal
        self.active = False
        self.cursor_pos = 0
        self.cursor_timer = 0
        self.cursor_visible = True
        self.validator = validator
        self.composition_text = ""  # IME 组合中的文本（拼音预览）
        self.multi_line = multi_line
        self.enter_newline = enter_newline
        # 文本选择
        self.sel_start = -1
        self.sel_end = -1
        self._dragging = False
        self._scroll_line = 0      # 多行模式滚动偏移（视觉行）
        self._total_vis_lines = 0  # 总视觉行数
        self._sb_thumb_rect = None # 滚动条滑块矩形
        self._sb_track_rect = None # 滚动条轨道矩形
        self._sb_dragging = False  # 滚动条拖拽中

    def _insert_text(self, text_str):
        """插入文本，经过 validator(ch, cursor_pos, current_text) 过滤"""
        for ch in text_str:
            if self.validator is not None:
                try:
                    ok = self.validator(ch, self.cursor_pos, self.text)
                except TypeError:
                    ok = self.validator(ch)  # 兼容旧版单参数 validator
                if not ok:
                    continue
            self.text = self.text[:self.cursor_pos] + ch + self.text[self.cursor_pos:]
            self.cursor_pos += 1

    def handle_event(self, event):
        if not self.active:
            return False

        # ---- 滚动 ----
        if event.type == pygame.MOUSEWHEEL and self.multi_line:
            max_scroll = max(0, self._total_vis_lines - self._visible_lines())
            self._scroll_line = max(0, min(max_scroll,
                                           self._scroll_line - event.y * 3))
            self._clear_sel()
            return True

        # ---- 滚动条交互 ----
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            sb_rect = getattr(self, '_sb_track_rect', None)
            if sb_rect and sb_rect.collidepoint(event.pos):
                # 点击在滚动条轨道上
                if self._sb_thumb_rect and self._sb_thumb_rect.collidepoint(event.pos):
                    # 拖拽滑块
                    self._sb_dragging = True
                    self._sb_drag_start_y = event.pos[1]
                    self._sb_drag_start_scroll = self._scroll_line
                else:
                    # 点击轨道空白处 → 跳到对应位置
                    max_scroll = max(0, self._total_vis_lines - self._visible_lines())
                    if max_scroll > 0:
                        sb_y = sb_rect.y
                        sb_h = sb_rect.h
                        thumb_h = self._sb_thumb_rect.h if self._sb_thumb_rect else 12
                        track_h = sb_h - thumb_h
                        rel_y = event.pos[1] - sb_y - thumb_h / 2
                        frac = max(0, min(1, rel_y / track_h)) if track_h > 0 else 0
                        self._scroll_line = int(frac * max_scroll)
                self._clear_sel()
                return True
            # 点击在文本区域 → 定位光标 / 开始选择
            if self.contains(event.pos):
                idx = self._pos_to_index(event.pos[0], event.pos[1])
                self.cursor_pos = idx
                self.sel_start = idx
                self.sel_end = idx
                self._dragging = True
                return True
        if event.type == pygame.MOUSEMOTION and self._sb_dragging:
            max_scroll = max(0, self._total_vis_lines - self._visible_lines())
            if max_scroll > 0 and self._sb_thumb_rect:
                track_rect = self._sb_track_rect
                sb_h = track_rect.h if track_rect else self.rect.height - 6
                thumb_h = self._sb_thumb_rect.h
                track_h = sb_h - thumb_h
                if track_h > 0:
                    delta = event.pos[1] - self._sb_drag_start_y
                    new_scroll = self._sb_drag_start_scroll + delta * max_scroll / track_h
                    self._scroll_line = int(max(0, min(max_scroll, new_scroll)))
            return True

        # ---- 鼠标拖选 ----
        if event.type == pygame.MOUSEMOTION and self._dragging:
            if event.buttons[0]:  # 左键仍按下
                idx = self._pos_to_index(event.pos[0], event.pos[1])
                self.sel_end = idx
                self.cursor_pos = idx
                return True
            else:
                self._dragging = False
        if event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            if self._sb_dragging:
                self._sb_dragging = False
                return True
            if self._dragging:
                self._dragging = False
                if self.sel_start == self.sel_end:
                    self._clear_sel()
                return True

        # IME 组合输入（中文等）
        if event.type == pygame.TEXTEDITING:
            self.composition_text = event.text
            return True

        # IME 组合完成输入
        if event.type == pygame.TEXTINPUT:
            self.composition_text = ""
            self._delete_sel()  # 有选择时先删除再插入
            self._insert_text(event.text)
            return True

        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_BACKSPACE:
                if not self._delete_sel() and self.cursor_pos > 0:
                    self.text = self.text[:self.cursor_pos - 1] + self.text[self.cursor_pos:]
                    self.cursor_pos -= 1
            elif event.key == pygame.K_DELETE:
                if not self._delete_sel() and self.cursor_pos < len(self.text):
                    self.text = self.text[:self.cursor_pos] + self.text[self.cursor_pos + 1:]
            elif event.key == pygame.K_LEFT:
                self.cursor_pos = max(0, self.cursor_pos - 1)
            elif event.key == pygame.K_RIGHT:
                self.cursor_pos = min(len(self.text), self.cursor_pos + 1)
            elif event.key == pygame.K_HOME:
                self.cursor_pos = 0
            elif event.key == pygame.K_END:
                self.cursor_pos = len(self.text)
            elif event.key == pygame.K_RETURN:
                if self.enter_newline:
                    self._insert_text('\n')
                else:
                    self.active = False
                    self.composition_text = ""
                return True
            elif event.key == pygame.K_TAB:
                if self.enter_newline:
                    self._insert_text('\t')
                else:
                    self.active = False
                    self.composition_text = ""
                return True
            elif self.multi_line and event.key == pygame.K_UP:
                visual_lines = self._build_visual_lines()
                cur_vline = 0
                cur_col = 0
                for vi, (start_idx, lt, end_idx) in enumerate(visual_lines):
                    if start_idx <= self.cursor_pos < end_idx:
                        cur_vline = vi
                        cur_col = self.cursor_pos - start_idx
                        break
                if cur_vline > 0:
                    prev_start, prev_text, _ = visual_lines[cur_vline - 1]
                    new_col = min(cur_col, len(prev_text))
                    self.cursor_pos = prev_start + new_col
                    self._clear_sel()
            elif self.multi_line and event.key == pygame.K_DOWN:
                visual_lines = self._build_visual_lines()
                cur_vline = 0
                cur_col = 0
                for vi, (start_idx, lt, end_idx) in enumerate(visual_lines):
                    if start_idx <= self.cursor_pos < end_idx:
                        cur_vline = vi
                        cur_col = self.cursor_pos - start_idx
                        break
                if cur_vline < len(visual_lines) - 1:
                    next_start, next_text, _ = visual_lines[cur_vline + 1]
                    new_col = min(cur_col, len(next_text))
                    self.cursor_pos = next_start + new_col
                    self._clear_sel()
            elif event.key == pygame.K_v and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                # Ctrl+V 粘贴
                try:
                    import tkinter as tk
                    root = tk.Tk()
                    root.withdraw()
                    clipboard = root.clipboard_get()
                    root.destroy()
                    self._insert_text(clipboard)
                except Exception:
                    pass
                return True
            elif event.key == pygame.K_c and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                # Ctrl+C 复制
                try:
                    import tkinter as tk
                    root = tk.Tk()
                    root.withdraw()
                    root.clipboard_clear()
                    root.clipboard_append(self.text)
                    root.destroy()
                except Exception:
                    pass
                return True
            elif event.key == pygame.K_a and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                # Ctrl+A 全选
                self.cursor_pos = 0
                return True
            # 字符输入统一由 TEXTINPUT 事件处理，避免 KEYDOWN+TEXTINPUT 双重插入
            return True
        return False

    def set_text(self, text):
        self.text = text
        self.cursor_pos = len(text)

    def clear(self):
        self.text = ""
        self.cursor_pos = 0

    def update(self, dt):
        self.cursor_timer += dt
        if self.cursor_timer >= 500:
            self.cursor_timer = 0
            self.cursor_visible = not self.cursor_visible

    def draw(self, surface, label=""):
        # 标签
        if label:
            draw_text(surface, label, self.rect.x, self.rect.y - 20, font_small, TEXT_DIM)

        # 背景
        border_color = INPUT_BORDER_ACTIVE if self.active else INPUT_BORDER
        draw_rect(surface, self.rect, INPUT_BG)
        draw_rect_outline(surface, self.rect, border_color, width=1)

        pad_x = 8
        line_h = self.font.get_height() + 4
        max_text_w = self.rect.width - pad_x * 2

        if self.text or self.composition_text:
            # 构建显示文本（含 IME 组合）
            display_text = self.text
            if self.composition_text:
                display_text = (self.text[:self.cursor_pos] + self.composition_text
                                + self.text[self.cursor_pos:])

            if self.multi_line:
                visual_lines = self._build_visual_lines(display_text)
                self._total_vis_lines = len(visual_lines)
                vis_count = self._visible_lines()
                # 自动滚动：光标所在行保持在可见范围内
                if self.active:
                    cur_vline = 0
                    for vi, (start_idx, _lt, end_idx) in enumerate(visual_lines):
                        if start_idx <= self.cursor_pos < end_idx:
                            cur_vline = vi
                            break
                    else:
                        cur_vline = len(visual_lines) - 1 if visual_lines else 0
                    if cur_vline < self._scroll_line:
                        self._scroll_line = cur_vline
                    elif cur_vline >= self._scroll_line + vis_count:
                        self._scroll_line = cur_vline - vis_count + 1
                # 限制滚动范围
                max_sl = max(0, len(visual_lines) - vis_count)
                self._scroll_line = max(0, min(max_sl, self._scroll_line))

                # 裁剪可见区域
                old_clip = surface.get_clip()
                surface.set_clip(self.rect.clip(old_clip))

                # 绘制可见行
                for vi in range(self._scroll_line,
                                min(self._scroll_line + vis_count, len(visual_lines))):
                    _si, line_text, _ei = visual_lines[vi]
                    ly = self.rect.y + 4 + (vi - self._scroll_line) * line_h
                    draw_text(surface, line_text, self.rect.x + pad_x, ly, self.font)

                surface.set_clip(old_clip)

                # 滚动条
                if len(visual_lines) > vis_count:
                    sb_w = 6
                    sb_x = self.rect.right - sb_w - 3
                    sb_h = self.rect.height - 6
                    sb_y = self.rect.y + 3
                    thumb_h = max(12, sb_h * vis_count / len(visual_lines))
                    thumb_y = sb_y + (sb_h - thumb_h) * self._scroll_line / max_sl
                    # 轨道背景（加宽点击区域）
                    track_rect = pygame.Rect(sb_x - 4, sb_y, sb_w + 8, sb_h)
                    draw_rect(surface, track_rect, SCROLLBAR_BG)
                    draw_rect(surface, (sb_x, thumb_y, sb_w, thumb_h), SCROLLBAR_THUMB)
                    self._sb_track_rect = track_rect
                    self._sb_thumb_rect = pygame.Rect(sb_x, thumb_y, sb_w, thumb_h)
                else:
                    self._sb_track_rect = None
                    self._sb_thumb_rect = None
            else:
                # 单行模式
                clipped = clip_text(display_text, self.font, max_text_w)
                draw_text(surface, clipped, self.rect.x + pad_x, self.rect.y + 6, self.font)
                # IME 组合下划线
                if self.composition_text:
                    before = self.font.render(self.text[:self.cursor_pos], True, (0, 0, 0)).get_width()
                    comp_w = self.font.render(self.composition_text, True, (0, 0, 0)).get_width()
                    underline_y = self.rect.y + self.rect.height - 5
                    pygame.draw.line(surface, (200, 200, 200),
                                     (self.rect.x + pad_x + before, underline_y),
                                     (self.rect.x + pad_x + before + comp_w, underline_y), 2)
        elif self.placeholder:
            if self.multi_line:
                draw_text(surface, self.placeholder, self.rect.x + pad_x, self.rect.y + 6,
                          self.font, TEXT_DIM)
            else:
                draw_text(surface, self.placeholder, self.rect.x + pad_x, self.rect.y + 6,
                          self.font, TEXT_DIM)

        # 选择高亮（在文本之后、光标之前绘制）
        sel = self._get_sel_range()
        if sel and self.multi_line and self.text:
            visual_lines = self._build_visual_lines()
            vis_count = self._visible_lines()
            for vi, (base_idx, line_text, end_idx) in enumerate(visual_lines):
                if vi < self._scroll_line or vi >= self._scroll_line + vis_count:
                    continue
                s_start = max(sel[0], base_idx)
                s_end = min(sel[1], end_idx)
                # 避免选中 \n 字符
                s_end = min(s_end, base_idx + len(line_text))
                if s_start < s_end:
                    pre_w = self.font.render(line_text[:s_start - base_idx], True, (0, 0, 0)).get_width()
                    sel_w = self.font.render(line_text[s_start - base_idx:s_end - base_idx],
                                              True, (0, 0, 0)).get_width()
                    ly = self.rect.y + 4 + (vi - self._scroll_line) * line_h
                    hl_rect = pygame.Rect(self.rect.x + pad_x + pre_w, ly, sel_w, line_h - 2)
                    draw_rect(surface, hl_rect, (60, 100, 180))
        elif sel and not self.multi_line:
            # 单行模式选择高亮
            pre_w = self.font.render(self.text[:sel[0]], True, (0, 0, 0)).get_width()
            sel_w = self.font.render(self.text[sel[0]:sel[1]], True, (0, 0, 0)).get_width()
            hl_rect = pygame.Rect(self.rect.x + pad_x + pre_w, self.rect.y + 4,
                                   sel_w, self.rect.height - 8)
            draw_rect(surface, hl_rect, (60, 100, 180))

        # 光标
        if self.active and self.cursor_visible and not self.composition_text:
            if self.multi_line:
                visual_lines = self._build_visual_lines()
                cur_vline = len(visual_lines) - 1 if visual_lines else 0
                cur_col = 0
                line_text = ""
                for vi, (start_idx, lt, end_idx) in enumerate(visual_lines):
                    if start_idx <= self.cursor_pos < end_idx:
                        cur_vline = vi
                        cur_col = self.cursor_pos - start_idx
                        line_text = lt
                        break
                vis_count = self._visible_lines()
                if self._scroll_line <= cur_vline < self._scroll_line + vis_count:
                    prefix = line_text[:cur_col]
                    cursor_x = self.rect.x + pad_x + self.font.render(prefix, True, (0, 0, 0)).get_width()
                    cursor_y = self.rect.y + 4 + (cur_vline - self._scroll_line) * line_h
                    pygame.draw.line(surface, WHITE,
                                     (cursor_x, cursor_y),
                                     (cursor_x, cursor_y + line_h - 4), 1)
            else:
                cursor_x = self.rect.x + pad_x
                if self.text[:self.cursor_pos]:
                    cursor_x += self.font.render(self.text[:self.cursor_pos], True, (0, 0, 0)).get_width()
                cursor_y = self.rect.y + 4
                pygame.draw.line(surface, WHITE,
                                 (cursor_x, cursor_y),
                                 (cursor_x, cursor_y + self.rect.height - 8), 1)

    def contains(self, pos):
        return self.rect.collidepoint(pos)

    def _get_sel_range(self):
        """返回 (min, max) 选择范围，无选择则 None"""
        if self.sel_start < 0 or self.sel_end < 0 or self.sel_start == self.sel_end:
            return None
        return (min(self.sel_start, self.sel_end), max(self.sel_start, self.sel_end))

    def _visible_lines(self):
        """返回当前输入框可显示的视觉行数"""
        return max(1, (self.rect.height - 4) // (self.font.get_height() + 4))

    def _build_visual_lines(self, text=None):
        """统一构建视觉行列表，返回 [(start_idx, line_text, end_idx), ...]
        end_idx 包含该行末尾的 \\n 字符位置。"""
        if text is None:
            text = self.text
        max_w = self.rect.width - 16
        paragraphs = text.split('\n')
        result = []
        char_offset = 0
        for pi, para in enumerate(paragraphs):
            wrapped = wrap_text(para, self.font, max_w) if para else [""]
            for wl in wrapped:
                end_idx = char_offset + len(wl)
                if pi < len(paragraphs) - 1:
                    end_idx += 1  # 末尾的 \n
                result.append((char_offset, wl, end_idx))
                char_offset = end_idx
        return result

    def _clear_sel(self):
        """清除选择"""
        self.sel_start = -1
        self.sel_end = -1

    def _delete_sel(self):
        """删除选中文本，返回是否执行了删除"""
        r = self._get_sel_range()
        if r is None:
            return False
        self.text = self.text[:r[0]] + self.text[r[1]:]
        self.cursor_pos = r[0]
        self._clear_sel()
        return True

    def _pos_to_index(self, mx, my):
        """将屏幕坐标映射为文本字符索引"""
        rel_x = mx - self.rect.x - 8
        rel_y = my - self.rect.y - 4
        line_h = self.font.get_height() + 4

        if self.multi_line:
            visual_lines = self._build_visual_lines()
            if not visual_lines:
                return 0
            vis_count = self._visible_lines()
            # 点击位置在可见行范围内
            line_idx = self._scroll_line + int(rel_y // line_h)
            line_idx = max(0, min(line_idx, len(visual_lines) - 1))
            base_idx, line_text, _ = visual_lines[line_idx]
            # 在该行内按 x 找列
            col = len(line_text)
            for i in range(len(line_text)):
                w = self.font.render(line_text[:i + 1], True, (0, 0, 0)).get_width()
                if w > rel_x:
                    col = i
                    break
            return min(base_idx + col, len(self.text))
        else:
            # 单行模式
            col = len(self.text)
            for i in range(len(self.text)):
                w = self.font.render(self.text[:i + 1], True, (0, 0, 0)).get_width()
                if w > rel_x:
                    col = i
                    break
            return col


class Button:
    """按钮"""

    def __init__(self, x, y, w, h, text, color=BUTTON_COLOR, hover_color=None, font_obj=None):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.color = color
        self.hover_color = hover_color or tuple(min(c + 20, 255) for c in color)
        self.font = font_obj or font_small
        self.hovered = False

    def handle_event(self, event):
        if event.type == pygame.MOUSEMOTION:
            self.hovered = self.rect.collidepoint(event.pos)
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos) and event.button == 1:
                return True
        return False

    def draw(self, surface):
        color = self.hover_color if self.hovered else self.color
        draw_rect(surface, self.rect, color)
        text_surf = self.font.render(self.text, True, WHITE)
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    def contains(self, pos):
        return self.rect.collidepoint(pos)


class ScrollableList:
    """可滚动的模块列表"""

    def __init__(self, x, y, w, h, max_visible=12):
        self.rect = pygame.Rect(x, y, w, h)
        self.scroll_offset = 0
        self.max_scroll = 0
        self.max_visible = max_visible
        self.hovered_index = -1
        self.item_height = 72

    def update_scroll(self, total_items):
        visible_height = self.rect.height - 10
        total_height = total_items * self.item_height
        self.max_scroll = max(0, total_height - visible_height)

    def scroll(self, dy):
        self.scroll_offset = max(0, min(self.max_scroll, self.scroll_offset + dy))

    def handle_event(self, event):
        if event.type == pygame.MOUSEWHEEL:
            if self.rect.collidepoint(pygame.mouse.get_pos()):
                self.scroll(-event.y * 30)
                return True
        return False

    def get_item_rect(self, index):
        return pygame.Rect(
            self.rect.x + 5,
            self.rect.y + 5 + index * self.item_height - self.scroll_offset,
            self.rect.width - 20,
            self.item_height - 6
        )

    def draw_scrollbar(self, surface, total_items):
        if self.max_scroll <= 0:
            return
        visible_height = self.rect.height - 10
        thumb_h = max(30, visible_height * visible_height / (total_items * self.item_height))
        thumb_y = self.rect.y + (visible_height - thumb_h) * self.scroll_offset / self.max_scroll
        draw_rect(surface, (self.rect.x + self.rect.width - 8, thumb_y, 6, thumb_h), SCROLLBAR_THUMB)


# ============================================================
# 主应用类
# ============================================================
# ============================================================
# 帮助文本（修改这些字符串即可改变各区域说明）
# ============================================================
HELP_TEXTS = {
    'env_user_config':
        "点击\"选择配置文件\":选择一个你保存的.json配置\n"
        "软件将自动读取你保存的配置内容\n"
        "其他三个选项一般不用修改，保持默认即可\n",
    'env_module_enable':
        "勾选需要启用的模块。\n"
        "勾选父模块时会自动启用其所有子模块；\n"
        "取消勾选子模块时会同步取消其父模块。\n"
        "鼠标悬停在模块行上可查看备注和子模块清单。",
    'env_custom_paths':
        "输入项目名称，将替换CMakeLists.txt中的项目名。\n"
        "添加自定义文件夹路径（相对于项目根目录）\n"
        "这些路径会被写入生成的 CMakeLists.txt 中\n"
        "同样也会保存在生成的用户配置文件",
    'env_config_edit':
        "按模块分组展示所有已勾选模块的宏定义。\n"
        "点击定义值可进入编辑模式修改覆盖值（黄色高亮表示已覆盖）；\n"
        "点击右侧 X 按钮可删除该宏定义使其不写入输出文件。\n"
        "按回车确认编辑，ESC 取消。",
    'env_generate':
        "选择要生成的文件类型，点击 [生成] 按钮选择输出目录。\n"
        "CMakeLists.txt 包含所有模块文件夹路径；\n"
        "ecf_configList.h 包含模块名宏和定义值宏；\n"
        "用户配置文件保存当前的配置，\n"
        "可在\"修改配置文件名\"处自定义文件名。",
    'add_module':
        "新建或编辑 CMake 模块。模块名需符合C语言规范\n"
        "可添加该模块包含的文件夹路径及绑定子模块，\n"
        "点击[确认添加] 保存模块。",
    'edit_module':
        "展示所有已创建的模块列表。\n"
        "点击模块名可将其加载到左侧编辑区域进行修改。\n"
        "使用 ▲ 按钮上移模块调整顺序，使用[删除] 按钮移除模块（操作不可撤销，请谨慎）",
    'config_module_list':
        "左侧展示所有模块，每个模块拥有独立的宏定义列表。\n"
        "点击模块名选中后，即可在右侧区域编辑该模块的宏定义。\n"
        "括号内显示该模块当前拥有的宏定义数量。",
    'config_define_edit':
        "为选中模块管理宏定义。\n"
        "点击已有宏定义可加载到下方表单修改；\n"
        "点击 [删除] 可移除该宏定义。",
}
# ============================================================

class App:
    def __init__(self):
        self.clock = pygame.time.Clock()
        self.running = True
        self.dt = 0

        # 标签页
        self.tabs = ["环境文件生成", "修改CMake模块", "修改config全局宏定义"]
        self.current_tab = 0  # 默认显示"环境文件生成"

        # 加载数据
        self.rules = load_rules()
        self.cmake_example = read_example_file(CMAKE_EXAMPLE_USER, CMAKE_EXAMPLE_RES)
        self.config_example = read_example_file(CONFIG_EXAMPLE_USER, CONFIG_EXAMPLE_RES)

        # ---- 环境文件生成 页面的 UI 元素 ----

        # 页面滚动
        self.env_scroll_y = 0

        # 区域1：用户配置（4 按钮 2×2 网格）
        self.env_user_config = None
        self.env_config_path = ""        # 用户配置文件路径
        self.env_cmake_tpl_path = CMAKE_EXAMPLE_USER
        self.env_config_tpl_path = CONFIG_EXAMPLE_USER
        # Row1
        self.btn_env_load_config = Button(35, 88, 150, 28, "选择配置文件")
        self.btn_env_load_cmake_tpl = Button(430, 88, 170, 28, "载入CMakeList模版",
                                              color=(90, 110, 160))
        # Row2
        self.btn_env_load_user_cfg = Button(35, 120, 130, 28, "载入配置模版",
                                             color=(90, 110, 160))
        self.btn_env_load_config_tpl = Button(430, 120, 180, 28, "载入ecf_configList模版",
                                               color=(90, 110, 160))
        # 按钮 4 的模版路径（默认 rule.json），按钮 1 用 env_config_path
        self.env_rule_tpl_path = RULE_JSON_PATH
        # 各按钮关联的路径（用于 tooltip）
        self._btn_path_labels = {}

        # 区域2：模块启用勾选（初始化所有模块为未勾选）
        self.env_module_checks = {mod["name"]: False for mod in self.rules["modules"]}

        # 区域3：CMakeList自定义
        self.env_project_name = "Project"  # 项目名称
        self.env_project_input = TextInput(35, 0, 200, 28, "Project",
            validator=lambda c, pos, text: c not in '\\/:*?"<>|')
        self.env_custom_paths = []       # ["rel/path1", ...]
        self.btn_env_add_path = Button(35, 0, 120, 32, "选择文件夹")  # y动态计算

        # 区域4：宏定义覆盖值
        self.env_define_overrides = {}   # {"DEFINE_NAME": "value", ...}
        self.env_excluded_defines = set()  # 临时排除的定义名集合
        self.env_define_editing_index = -1  # 正在编辑的定义行索引（模块内）
        self.env_define_editing_module_name = ""  # 正在编辑的定义所属模块名
        self.env_define_edit_input = TextInput(0, 0, 200, 28, "输入新值...")  # 位置动态
        self.env_define_item_map = []  # 绘制时填充: [(module_name, def_idx, define_dict, item_rect), ...]
        self._env_def_list_rect = pygame.Rect(0, 0, 0, 0)  # 区域4的列表容器矩形
        self._env_sb_info = None  # 滚动条参数 (sb_x, sb_y, sb_h, thumb_h, content_h, visible_h)
        self._env_content_bottom = WIN_HEIGHT  # 页面总高度

        # 区域5：生成文件
        self.env_gen_cmake = True       # 勾选 CMakeLists.txt
        self.env_gen_config = True      # 勾选 ecf_configList.h
        self.env_gen_userjson = True    # 勾选 CFG_User.json
        self.env_cmake_output_dir = ""  # CMakeLists 输出目录
        self.btn_env_cmake_outdir = Button(35, 0, 130, 26, "选择生成位置",
                                            color=(90, 110, 160))  # y动态
        self.env_userjson_filename = "CFG_User"  # 自定义文件名（无后缀）
        self.env_userjson_input = TextInput(0, 0, 140, 26, "CFG_User",
            validator=lambda c, pos, text: c not in '\\/:*?"<>|')  # y动态
        self.btn_env_generate = Button(35, 0, 100, 34, "生成", color=BUTTON_GREEN)  # y动态

        # ---- 自动加载 UserConfig 中的第一个 .json ----
        self._auto_load_user_config()

        # ---- 修改CMake模块 页面的 UI 元素 ----

        # 输入框（模块名仅允许英文数字下划线，数字不能开头）
        self.input_name = TextInput(35, 105, 380, 32, "输入模块名...",
                                     validator=lambda c, pos, text: c.isascii()
                                     and (c.isalnum() or c == '_')
                                     and not (pos == 0 and c.isdigit()))
        self.input_note = TextInput(35, 175, 380, 60, "输入备注...", multi_line=True)

        # 启用 IME 文本输入（中文输入法支持）
        pygame.key.start_text_input()
        pygame.key.set_text_input_rect(pygame.Rect(35, 105, 380, 32))

        # 添加模块区域按钮
        btn_y = 560
        self.btn_add_folder = Button(35, btn_y, 110, 34, "包含文件夹")
        self.btn_reset = Button(160, btn_y, 80, 34, "重置", color=(120, 120, 125))
        self.btn_confirm_add = Button(255, btn_y, 110, 34, "确认添加", color=BUTTON_GREEN)
        self.btn_bind_submodule = Button(35, 447, 120, 30, "绑定子模块",
                                          color=(90, 110, 160))

        # 路径列表
        self.selected_paths = []

        # 子模块列表（存储已绑定的模块名）
        self.selected_submodules = []

        # 子模块选择对话框状态
        self.submodule_dialog_active = False
        self.submodule_dialog_scroll = 0
        self.submodule_dialog_temp = set()  # 对话框中暂选的模块名

        # 编辑模块区域
        self.module_list = ScrollableList(485, 85, 440, 500)

        # 编辑模式标记
        self.editing_module_index = -1  # -1 表示新建模式

        # 状态消息
        self.status_message = ""
        self.status_timer = 0

        # 确认对话框（CMake模块页用）
        self.confirm_dialog_active = False
        self.confirm_dialog_index = -1
        self.confirm_dialog_module_name = ""

        # 生成结果弹窗
        self.gen_result_active = False
        self.gen_result_message = ""  # 多行文本
        self.gen_result_is_error = False
        self._gen_ok_btn_rect = pygame.Rect(0, 0, 0, 0)

        # 帮助弹窗
        self.help_active = False
        self.help_title = ""
        self.help_text = ""
        self._help_ok_btn_rect = pygame.Rect(0, 0, 0, 0)
        # 帮助按钮点击区域列表: [(rect, section_key), ...]
        self._help_btn_rects = []

        # ---- 修改config全局宏定义 页面的 UI 元素 ----

        # 左侧模块列表
        self.config_module_list_rect = pygame.Rect(20, 80, 270, 510)
        self.config_module_scroll = 0
        self.config_selected_module = None  # 当前选中的模块名

        # 右侧宏定义列表区域
        self.config_defines_rect = pygame.Rect(310, 80, 630, 410)
        self.config_defines_scroll = 0

        # 添加宏定义状态
        self.config_adding_define = False
        self.config_new_define_name = TextInput(310, 528, 180, 30, "定义名...",
            validator=lambda c, pos, text: c.isascii()
            and (c.isalnum() or c == '_')
            and not (pos == 0 and c.isdigit()))
        self.config_new_define_value = TextInput(500, 528, 230, 30, "定义内容...")
        self.config_new_define_comment = TextInput(310, 568, 630, 56, "注释(可选)...",
                                                    multi_line=True)

        # 添加/取消按钮
        self.btn_config_confirm_define = Button(740, 527, 100, 30, "确认添加", color=BUTTON_GREEN)
        self.btn_config_cancel_define = Button(845, 527, 80, 30, "取消", color=(120, 120, 125))
        self.btn_config_add_define = Button(310, 522, 100, 30, "添加定义", color=(90, 110, 160))
        self.btn_config_batch_define = Button(420, 522, 100, 30, "批量添加", color=(90, 110, 160))

        # 批量添加弹窗
        self.batch_dialog_active = False
        self.batch_text_input = TextInput(0, 0, 600, 320, "在此粘贴 C 语言宏定义 ...",
                                           multi_line=True, enter_newline=True)
        self.btn_batch_confirm = Button(0, 0, 100, 34, "确认添加", color=BUTTON_GREEN)
        self.btn_batch_cancel = Button(0, 0, 80, 34, "取消", color=(120, 120, 125))

        # 删除宏定义确认对话框
        self.config_delete_dialog_active = False
        self.config_delete_define_index = -1
        self.config_delete_define_name = ""

        # 编辑定义索引（-1=新增模式, >=0=编辑已有定义）
        self.config_editing_define_index = -1

        # 滚动条拖拽状态: (area_id, start_mouse_y, start_scroll, total_h, visible_h) 或 None
        self._sb_drag = None

    # ============================================================
    # 数据操作
    # ============================================================
    def add_module(self, name, note, paths, submodules):
        """添加模块到 rules"""
        if not name.strip():
            self.set_status("错误：模块名不能为空", is_error=True)
            return False

        # 检查是否重名（编辑模式排除自身）
        for i, mod in enumerate(self.rules["modules"]):
            if mod["name"] == name.strip() and i != self.editing_module_index:
                self.set_status("错误：模块名已存在", is_error=True)
                return False

        module_entry = {
            "name": name.strip(),
            "note": note.strip(),
            "paths": list(paths),
            "submodules": list(submodules)
        }

        if self.editing_module_index >= 0:
            # 编辑模式：更新已有模块，保留原有 defines
            old_defines = self.rules["modules"][self.editing_module_index].get("defines", [])
            module_entry["defines"] = old_defines
            self.rules["modules"][self.editing_module_index] = module_entry
            self.set_status(f"模块 '{name.strip()}' 已更新")
            self.editing_module_index = -1
        else:
            # 新建模式
            self.rules["modules"].append(module_entry)
            self.set_status(f"模块 '{name.strip()}' 添加成功")

        save_rules(self.rules)
        return True

    def delete_module(self, index):
        """删除模块"""
        if 0 <= index < len(self.rules["modules"]):
            name = self.rules["modules"][index]["name"]
            del self.rules["modules"][index]
            save_rules(self.rules)
            self.set_status(f"模块 '{name}' 已删除")
            # 如果正在编辑此模块，清除编辑状态
            if self.editing_module_index == index:
                self.reset_add_area()
            elif self.editing_module_index > index:
                self.editing_module_index -= 1

    def move_module_up(self, index):
        """将模块向上移动一位"""
        if 1 <= index < len(self.rules["modules"]):
            name = self.rules["modules"][index]["name"]
            self.rules["modules"][index], self.rules["modules"][index - 1] = \
                self.rules["modules"][index - 1], self.rules["modules"][index]
            save_rules(self.rules)
            self.set_status(f"模块 '{name}' 已上移")
            # 同步编辑索引
            if self.editing_module_index == index:
                self.editing_module_index -= 1
            elif self.editing_module_index == index - 1:
                self.editing_module_index += 1

    def load_module_to_edit(self, index):
        """将模块数据加载到编辑区域"""
        if 0 <= index < len(self.rules["modules"]):
            mod = self.rules["modules"][index]
            self.input_name.set_text(mod["name"])
            self.input_note.set_text(mod["note"])
            self.selected_paths = list(mod.get("paths", []))
            self.selected_submodules = list(mod.get("submodules", []))
            self.editing_module_index = index
            self.set_status(f"正在编辑模块 '{mod['name']}'")

    def reset_add_area(self):
        """重置添加区域"""
        self.input_name.clear()
        self.input_note.clear()
        self.selected_paths.clear()
        self.selected_submodules.clear()
        self.editing_module_index = -1
        self.set_status("已重置")

    def _open_submodule_dialog(self):
        """打开子模块选择对话框"""
        self.submodule_dialog_active = True
        self.submodule_dialog_scroll = 0
        # 用当前已选的子模块初始化临时选择集
        self.submodule_dialog_temp = set(self.selected_submodules)

    def _handle_submodule_dialog_events(self, event):
        """处理子模块选择对话框的事件"""
        dialog_rect = pygame.Rect(
            WIN_WIDTH // 2 - 220, WIN_HEIGHT // 2 - 180, 440, 360
        )

        if event.type == pygame.MOUSEWHEEL:
            if dialog_rect.collidepoint(pygame.mouse.get_pos()):
                self.submodule_dialog_scroll = max(
                    0, self.submodule_dialog_scroll - event.y * 30
                )

        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = event.pos

            # 确认 / 取消按钮
            confirm_btn = pygame.Rect(WIN_WIDTH // 2 - 120, dialog_rect.bottom - 50, 100, 34)
            cancel_btn = pygame.Rect(WIN_WIDTH // 2 + 20, dialog_rect.bottom - 50, 100, 34)

            if confirm_btn.collidepoint(mx, my):
                # 确认：将临时选择写入 selected_submodules
                self.selected_submodules = list(self.submodule_dialog_temp)
                self.submodule_dialog_active = False
                self.set_status(f"已绑定 {len(self.selected_submodules)} 个子模块")
                return
            if cancel_btn.collidepoint(mx, my):
                self.submodule_dialog_active = False
                return

            # 点击对话框外部取消
            if not dialog_rect.collidepoint(mx, my):
                self.submodule_dialog_active = False
                return

            # 列表区域点击 - 切换选中
            list_area = pygame.Rect(
                dialog_rect.x + 15, dialog_rect.y + 55,
                dialog_rect.width - 30, dialog_rect.height - 120
            )
            if list_area.collidepoint(mx, my):
                item_h = 36
                # 构建与 draw 一致的过滤列表，确保索引对齐
                available = [
                    (j, m) for j, m in enumerate(self.rules["modules"])
                    if not (self.editing_module_index >= 0 and j == self.editing_module_index)
                ]
                for idx, (_j, mod) in enumerate(available):
                    item_y = list_area.y + 4 + idx * item_h - self.submodule_dialog_scroll
                    item_rect = pygame.Rect(list_area.x, item_y, list_area.width, item_h - 4)
                    if item_rect.collidepoint(mx, my):
                        mod_name = mod["name"]
                        if mod_name in self.submodule_dialog_temp:
                            self.submodule_dialog_temp.discard(mod_name)
                        else:
                            self.submodule_dialog_temp.add(mod_name)
                        return

    def _draw_submodule_dialog(self):
        """绘制子模块选择对话框"""
        if not self.submodule_dialog_active:
            return

        # 半透明遮罩
        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        # 对话框
        dialog_w, dialog_h = 440, 380
        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect, BUTTON_COLOR, width=2)

        # 标题
        title = font_bold.render("选择要绑定的子模块", True, WHITE)
        title_rect = title.get_rect(center=(WIN_WIDTH // 2, dialog_y + 25))
        screen.blit(title, title_rect)

        hint = font_small.render("（点击模块名切换选中/取消）", True, TEXT_DIM)
        hint_rect = hint.get_rect(center=(WIN_WIDTH // 2, dialog_y + 48))
        screen.blit(hint, hint_rect)

        # 列表区域
        list_area = pygame.Rect(
            dialog_x + 15, dialog_y + 60,
            dialog_w - 30, dialog_h - 130
        )
        screen.set_clip(list_area)

        # 过滤可选模块（排除正在编辑的自身）
        available_modules = [
            (i, mod) for i, mod in enumerate(self.rules["modules"])
            if not (self.editing_module_index >= 0 and i == self.editing_module_index)
        ]

        _dlg_tooltip = None  # 对话框内 tooltip

        if not available_modules:
            no_mod = font_small.render("暂无可用模块", True, TEXT_DIM)
            no_rect = no_mod.get_rect(center=list_area.center)
            screen.blit(no_mod, no_rect)
        else:
            item_h = 36
            for idx, (_orig_i, mod) in enumerate(available_modules):
                item_y = list_area.y + 4 + idx * item_h - self.submodule_dialog_scroll
                if item_y + item_h < list_area.y or item_y > list_area.bottom:
                    continue

                item_rect = pygame.Rect(list_area.x, item_y, list_area.width, item_h - 4)
                is_selected = mod["name"] in self.submodule_dialog_temp
                is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())

                if is_selected:
                    bg = ITEM_SELECTED
                elif is_hovered:
                    bg = ITEM_HOVER
                else:
                    bg = ITEM_BG
                draw_rect(screen, item_rect, bg)

                # 勾选标记（用图形绘制，避免特殊字符字体缺失）
                checkbox_rect = pygame.Rect(item_rect.x + 10, item_rect.y + 8, 18, 18)
                if is_selected:
                    draw_rect(screen, checkbox_rect, (80, 180, 80))
                    # 白色对勾（用两条短线模拟）
                    cx, cy = checkbox_rect.center
                    pygame.draw.line(screen, WHITE, (cx - 4, cy), (cx - 1, cy + 3), 2)
                    pygame.draw.line(screen, WHITE, (cx - 1, cy + 3), (cx + 4, cy - 4), 2)
                else:
                    draw_rect_outline(screen, checkbox_rect, TEXT_DIM, border_radius=2, width=1)

                # 模块名
                name_surf = font_normal.render(mod["name"], True,
                                               WHITE if is_selected else TEXT_COLOR)
                screen.blit(name_surf, (item_rect.x + 35, item_rect.y + 7))

                # 备注
                note = mod.get("note", "")
                clipped_n = clip_text(note, font_small, 200)
                note_surf = font_small.render(clipped_n, True, TEXT_DIM)
                screen.blit(note_surf, (item_rect.x + 200, item_rect.y + 9))
                # 备注 tooltip
                if is_hovered and note and clipped_n != note and _dlg_tooltip is None:
                    _dlg_tooltip = collect_text_tooltip(note, font_small, 200,
                                                         item_rect, list_area)

        screen.set_clip(None)

        # 绘制备注 tooltip（在裁剪区域外）
        if _dlg_tooltip is not None:
            tip_x, tip_y, tip_w, tip_h, lines = _dlg_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, line in enumerate(lines):
                s_surf = font_small.render(line, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))

        # 已选数量
        count_text = f"已选 {len(self.submodule_dialog_temp)} 个"
        draw_text(screen, count_text, dialog_x + 15, dialog_y + dialog_h - 75, font_small,
                  (100, 220, 100))

        # 按钮
        mx, my = pygame.mouse.get_pos()
        confirm_btn = Button(WIN_WIDTH // 2 - 120, dialog_y + dialog_h - 50, 100, 34,
                             "确认", color=BUTTON_GREEN, font_obj=font_small)
        cancel_btn = Button(WIN_WIDTH // 2 + 20, dialog_y + dialog_h - 50, 100, 34,
                            "取消", color=(100, 100, 105), font_obj=font_small)
        confirm_btn.hovered = confirm_btn.rect.collidepoint(mx, my)
        cancel_btn.hovered = cancel_btn.rect.collidepoint(mx, my)
        confirm_btn.draw(screen)
        cancel_btn.draw(screen)

    # ============================================================
    # Config 宏定义数据操作
    # ============================================================
    def _get_selected_module_data(self):
        """获取当前选中模块的数据"""
        if self.config_selected_module is None:
            return None
        for mod in self.rules["modules"]:
            if mod["name"] == self.config_selected_module:
                return mod
        return None

    def _get_selected_module_defines(self):
        """获取当前选中模块的宏定义列表"""
        mod = self._get_selected_module_data()
        if mod is None:
            return []
        return mod.get("defines", [])

    def add_define_to_module(self, name, value, comment):
        """向当前选中模块添加宏定义"""
        if not name.strip():
            self.set_status("错误：定义名不能为空", is_error=True)
            return False
        mod = self._get_selected_module_data()
        if mod is None:
            self.set_status("错误：未选中模块", is_error=True)
            return False
        if "defines" not in mod:
            mod["defines"] = []
        mod["defines"].append({
            "name": name.strip(),
            "value": value.strip(),
            "comment": comment.strip()
        })
        save_rules(self.rules)
        self.set_status(f"宏定义 '{name.strip()}' 已添加")
        return True

    def delete_define_from_module(self, index):
        """从当前选中模块删除宏定义"""
        mod = self._get_selected_module_data()
        if mod is None or "defines" not in mod:
            return
        if 0 <= index < len(mod["defines"]):
            name = mod["defines"][index]["name"]
            del mod["defines"][index]
            save_rules(self.rules)
            self.set_status(f"宏定义 '{name}' 已删除")

    def parse_and_batch_add_defines(self, raw_text):
        """解析 C 语言宏定义文本，批量添加到当前选中模块"""
        mod = self._get_selected_module_data()
        if mod is None:
            self.set_status("错误：未选中模块", is_error=True)
            return 0
        if "defines" not in mod:
            mod["defines"] = []

        # 按行扫描，提取 #define NAME VALUE
        import re
        # 匹配: 可选空白 + #define + 空白 + 名称 + 空白 + 值(到行尾)
        pattern = re.compile(r'^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.*?)\s*$')
        parsed = {}  # 用 dict 去重，后面的覆盖前面的
        for line in raw_text.split('\n'):
            m = pattern.match(line)
            if m:
                name = m.group(1)
                value = m.group(2).strip()
                # 去除行尾注释 // ...
                comment = ""
                cmt_pos = value.find('//')
                if cmt_pos >= 0:
                    comment = value[cmt_pos + 2:].strip()
                    value = value[:cmt_pos].strip()
                # 也处理 /* */ 注释
                cmt_pos = value.find('/*')
                if cmt_pos >= 0:
                    end_pos = value.find('*/', cmt_pos)
                    if end_pos >= 0:
                        comment = value[cmt_pos + 2:end_pos].strip()
                    else:
                        comment = value[cmt_pos + 2:].strip()
                    value = value[:cmt_pos].strip()
                parsed[name] = (value, comment)

        added = 0
        for name, (value, comment) in parsed.items():
            # 检查当前模块是否已有同名定义，有则更新值
            existing = next((d for d in mod["defines"] if d["name"] == name), None)
            if existing:
                existing["value"] = value
                existing["comment"] = comment or existing.get("comment", "")
            else:
                mod["defines"].append({
                    "name": name,
                    "value": value,
                    "comment": comment
                })
            added += 1

        if added > 0:
            save_rules(self.rules)
        return added

    def _config_reset_form(self):
        """重置添加/编辑表单"""
        self.config_new_define_name.clear()
        self.config_new_define_value.clear()
        self.config_new_define_comment.clear()
        self.config_adding_define = False
        self.config_editing_define_index = -1

    def update_define_in_module(self, index, name, value, comment):
        """更新当前选中模块的指定宏定义"""
        mod = self._get_selected_module_data()
        if mod is None or "defines" not in mod:
            return False
        if 0 <= index < len(mod["defines"]):
            if not name.strip():
                self.set_status("错误：定义名不能为空", is_error=True)
                return False
            mod["defines"][index] = {
                "name": name.strip(),
                "value": value.strip(),
                "comment": comment.strip()
            }
            save_rules(self.rules)
            self.set_status(f"宏定义 '{name.strip()}' 已更新")
            return True
        return False

    # ============================================================
    # 环境文件生成 - 数据操作
    # ============================================================
    def _auto_load_user_config(self):
        """启动时自动加载 UserConfig 中第一个 .json 文件"""
        ucd = USERCONFIG_DIR
        os.makedirs(ucd, exist_ok=True)
        try:
            json_files = sorted(
                [f for f in os.listdir(ucd) if f.endswith('.json')])
        except OSError:
            return
        if not json_files:
            return
        filepath = os.path.join(ucd, json_files[0])
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                self.env_user_config = json.load(f)
            self.env_config_path = filepath.replace("\\", "/")
            self._env_sync_from_config()
        except (json.JSONDecodeError, IOError):
            pass

    def _env_select_tpl_file(self, tpl_type):
        """选择模版文件（cmake 或 config）"""
        root = tk.Tk()
        root.withdraw()
        root.attributes('-topmost', True)
        if tpl_type == "cmake":
            title = "选择 CMakeLists 模版文件"
            default = CMAKE_EXAMPLE_USER
        else:
            title = "选择 ecf_configList 模版文件"
            default = CONFIG_EXAMPLE_USER
        filepath = filedialog.askopenfilename(
            title=title,
            initialdir=os.path.dirname(default) if os.path.exists(os.path.dirname(default)) else os.getcwd(),
            filetypes=[("All files", "*.*"), ("C/H files", "*.c;*.h"), ("Text files", "*.txt")]
        )
        root.destroy()
        if not filepath:
            return
        filepath = filepath.replace("\\", "/")
        if tpl_type == "cmake":
            self.env_cmake_tpl_path = filepath
            self.set_status(f"CMake模版: {os.path.basename(filepath)}")
        else:
            self.env_config_tpl_path = filepath
            self.set_status(f"config模版: {os.path.basename(filepath)}")

    def _do_load_rule_tpl(self):
        """（由 update 延迟调用）选择 rule.json 配置模版文件并重新加载"""
        root = tk.Tk()
        root.withdraw()
        root.attributes('-topmost', True)
        default = self.env_rule_tpl_path
        filepath = filedialog.askopenfilename(
            title="选择 rule.json 配置模版",
            initialdir=os.path.dirname(default) if os.path.exists(os.path.dirname(default)) else os.getcwd(),
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        root.destroy()
        if not filepath:
            return
        self.env_rule_tpl_path = filepath.replace("\\", "/")
        # 重新加载模块数据
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                data = json.load(f)
            if "modules" not in data:
                data["modules"] = []
            self.rules = data
            # 彻底刷新所有相关状态
            self.env_module_checks = {mod["name"]: False for mod in self.rules["modules"]}
            self.config_selected_module = None
            self.config_defines_scroll = 0
            self.editing_module_index = -1
            self.selected_paths.clear()
            self.selected_submodules.clear()
            self.input_name.clear()
            self.input_note.clear()
            self._config_reset_form()
            # 若已加载用户配置，重新同步勾选
            if self.env_user_config:
                self._env_sync_from_config()
            n_mods = len(data["modules"])
            self.set_status(f"已加载 {n_mods} 个模块: {os.path.basename(filepath)}")
        except (json.JSONDecodeError, IOError) as e:
            self.set_status(f"加载模版失败: {e}", is_error=True)

    def env_load_user_config(self, filepath=None):
        """选择并加载用户配置文件（可传路径跳过对话框）"""
        if filepath is None:
            root = tk.Tk()
            root.withdraw()
            root.attributes('-topmost', True)
            filepath = filedialog.askopenfilename(
                title="选择配置文件",
                initialdir=os.getcwd(),
                filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
            )
            root.destroy()
            if not filepath:
                return False

        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                self.env_user_config = json.load(f)
        except (json.JSONDecodeError, IOError) as e:
            self.set_status(f"错误：无法读取配置文件 - {e}", is_error=True)
            return False

        try:
            self.env_config_path = os.path.relpath(filepath, os.getcwd()).replace("\\", "/")
        except ValueError:
            self.env_config_path = filepath.replace("\\", "/")
        # 从配置中同步勾选状态
        self._env_sync_from_config()
        self.set_status(f"已加载配置: {self.env_config_path}")
        return True

    def _env_sync_from_config(self):
        """根据 env_user_config 同步模块勾选、路径、宏定义覆盖"""
        cfg = self.env_user_config
        if cfg is None:
            return

        # 同步模块勾选：配置中存在的模块名即勾选
        cfg_modules = set(cfg.get("enabled_modules", []))
        self.env_module_checks.clear()
        for mod in self.rules["modules"]:
            self.env_module_checks[mod["name"]] = mod["name"] in cfg_modules

        # 自动勾选子模块
        changed = True
        while changed:
            changed = False
            for mod in self.rules["modules"]:
                if self.env_module_checks.get(mod["name"], False):
                    for sub_name in mod.get("submodules", []):
                        if not self.env_module_checks.get(sub_name, False):
                            self.env_module_checks[sub_name] = True
                            changed = True

        # 同步自定义路径
        self.env_custom_paths = list(cfg.get("custom_paths", []))

        # 同步项目名称
        saved_name = cfg.get("project_name", "")
        if saved_name:
            self.env_project_name = saved_name
            self.env_project_input.set_text(saved_name)

        # 同步宏定义覆盖值
        self.env_define_overrides = dict(cfg.get("define_overrides", {}))
        # 同步临时排除的定义
        self.env_excluded_defines = set(cfg.get("excluded_defines", []))

    def env_toggle_module(self, module_name):
        """切换模块勾选，自动同步子模块"""
        # 若模块不在字典中则初始化为 False
        if module_name not in self.env_module_checks:
            self.env_module_checks[module_name] = False

        new_state = not self.env_module_checks[module_name]
        self.env_module_checks[module_name] = new_state

        if new_state:
            # 勾选模块时，同时勾选其所有子模块
            self._env_check_submodules(module_name)
        else:
            # 取消勾选时，同时取消所有父模块
            self._env_uncheck_parents(module_name)

    def _env_check_submodules(self, module_name):
        """递归勾选子模块"""
        for mod in self.rules["modules"]:
            if mod["name"] == module_name:
                for sub_name in mod.get("submodules", []):
                    self.env_module_checks[sub_name] = True
                    self._env_check_submodules(sub_name)
                break

    def _env_uncheck_parents(self, module_name):
        """递归取消包含此子模块的所有父模块"""
        for mod in self.rules["modules"]:
            if module_name in mod.get("submodules", []):
                if self.env_module_checks.get(mod["name"], False):
                    self.env_module_checks[mod["name"]] = False
                    self._env_uncheck_parents(mod["name"])

    def env_get_checked_modules(self):
        """获取所有已勾选的模块名列表"""
        return [name for name, checked in self.env_module_checks.items() if checked]

    def env_get_all_defines_for_checked(self):
        """获取所有已勾选模块的宏定义，附带覆盖值和来源模块"""
        result = []
        seen = set()
        checked = set(self.env_get_checked_modules())
        for mod in self.rules["modules"]:
            if mod["name"] not in checked:
                continue
            for d in mod.get("defines", []):
                key = d["name"]
                if key in self.env_excluded_defines:
                    continue
                if key not in seen:
                    seen.add(key)
                    override = self.env_define_overrides.get(key, d["value"])
                    result.append({
                        "name": d["name"],
                        "default_value": d["value"],
                        "current_value": override,
                        "comment": d.get("comment", ""),
                        "source_module": mod["name"]
                    })
        return result

    # ============================================================
    # 环境文件生成 - 文件生成逻辑
    # ============================================================
    def env_generate_files(self):
        """直接生成勾选的文件到预设位置"""
        cmake_dir = self.env_cmake_output_dir if self.env_gen_cmake else ""
        if self.env_gen_cmake and not cmake_dir:
            self.gen_result_message = "请先选择 CMakeLists.txt 的生成位置"
            self.gen_result_is_error = True
            self.gen_result_active = True
            return

        generated = []
        out_dirs = []
        try:
            if self.env_gen_cmake:
                self._gen_cmake_file(cmake_dir)
                generated.append("CMakeLists.txt")
                out_dirs.append(cmake_dir)
            if self.env_gen_config:
                # 生成到程序所在目录
                self._gen_config_file(_USER_DIR)
                generated.append("ecf_configList.h")
                out_dirs.append(_USER_DIR)
            if self.env_gen_userjson:
                os.makedirs(USERCONFIG_DIR, exist_ok=True)
                fname = self._gen_user_json(USERCONFIG_DIR)
                generated.append(fname)
                out_dirs.append(USERCONFIG_DIR)
        except Exception as e:
            self.gen_result_message = f"生成失败:\n{e}"
            self.gen_result_is_error = True
            self.gen_result_active = True
            return

        if generated:
            dir_list = "\n".join(f"  → {d}" for d in sorted(set(out_dirs)))
            self.gen_result_message = (f"已生成 {len(generated)} 个文件:\n"
                                       + "\n".join(f"  • {f}" for f in generated)
                                       + f"\n\n输出位置:\n{dir_list}")
            self.gen_result_is_error = False
        else:
            self.gen_result_message = "未选择任何文件生成"
            self.gen_result_is_error = True
        self.gen_result_active = True

    def _gen_cmake_file(self, out_dir):
        """生成 CMakeLists.txt"""
        src = self.env_cmake_tpl_path
        if not os.path.exists(src):
            src = CMAKE_EXAMPLE_RES
        if not os.path.exists(src):
            raise FileNotFoundError(f"模板文件不存在: {self.env_cmake_tpl_path}")
        if not os.path.exists(src):
            raise FileNotFoundError(f"模板文件不存在: {src}")

        with open(src, 'r', encoding='utf-8') as f:
            content = f.read()

        # 替换项目名称（使用正则匹配模板中可能已被修改的值）
        import re
        project_name = self.env_project_name.strip() or "Project"
        content = re.sub(
            r'(set\(CMAKE_PROJECT_NAME\s+)\S+(\s*\))',
            r'\1' + project_name + r'\2',
            content
        )

        # 收集所有勾选模块的路径 + 自定义路径
        all_paths = list(self.env_custom_paths)
        checked = set(self.env_get_checked_modules())
        for mod in self.rules["modules"]:
            if mod["name"] in checked:
                for p in mod.get("paths", []):
                    if p not in all_paths:
                        all_paths.append(p)

        # 生成 USER_ADD_DIR 行
        ace_prefix = "ACE_ECF_26/"
        lines = []
        for p in all_paths:
            p = p.replace("\\", "/")
            if p.startswith(".."):
                # 以 .. 开头：去掉 ../ 和 ACE_ECF_26/ 前缀
                while p.startswith("../"):
                    p = p[3:]
                if p.startswith(ace_prefix):
                    p = p[len(ace_prefix):]
                lines.append(f'USER_ADD_DIR(${{CMAKE_SOURCE_DIR}}/{p})')
            else:
                # 不以 .. 开头：保留 ACE_ECF_26/ 前缀
                if p.startswith(ace_prefix):
                    lines.append(f'USER_ADD_DIR(${{CMAKE_SOURCE_DIR}}/{p})')
                else:
                    lines.append(f'USER_ADD_DIR(${{CMAKE_SOURCE_DIR}}/{ace_prefix}{p})')
        insert_block = "\n".join(lines)

        # 在 # auto generate start / end 之间插入
        start_marker = "# auto generate start"
        end_marker = "# auto generate end"
        if start_marker in content and end_marker in content:
            before = content.split(start_marker)[0]
            after = content.split(end_marker)[1]
            content = before + start_marker + "\n" + insert_block + "\n" + end_marker + after
        else:
            # 如果没有标记，追加到末尾
            content += f"\n{start_marker}\n{insert_block}\n{end_marker}\n"

        out_path = os.path.join(out_dir, "CMakeLists.txt")
        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(content)

    def _gen_config_file(self, out_dir):
        """生成 ecf_configList.h"""
        src = self.env_config_tpl_path
        if not os.path.exists(src):
            src = CONFIG_EXAMPLE_RES
        if not os.path.exists(src):
            raise FileNotFoundError(f"模板文件不存在: {self.env_config_tpl_path}")

        with open(src, 'r', encoding='utf-8') as f:
            content = f.read()

        # 收集勾选的模块名和宏定义
        checked = set(self.env_get_checked_modules())
        defines = self.env_get_all_defines_for_checked()

        insert_lines = []
        # 模块名宏
        for mod_name in sorted(checked):
            insert_lines.append(f'#ifndef {mod_name}')
            insert_lines.append(f'#define {mod_name}')
            insert_lines.append(f'#endif // {mod_name}')
            insert_lines.append('')

        # 定义宏
        for d in defines:
            val = self.env_define_overrides.get(d["name"], d["current_value"])
            cmt = f' // {d["comment"]}' if d.get("comment") else ''
            insert_lines.append(f'#ifndef {d["name"]}')
            insert_lines.append(f'#define {d["name"]} {val}{cmt}')
            insert_lines.append(f'#endif // {d["name"]}')
            insert_lines.append('')

        insert_block = "\n".join(insert_lines).rstrip()

        # 插入到 #define __ECF_CONFIGLIST_H 和 #endif // __ECF_CONFIGLIST_H 之间
        header_start = "#define __ECF_CONFIGLIST_H"
        header_end = "#endif // __ECF_CONFIGLIST_H"
        if header_start in content and header_end in content:
            before = content.split(header_start)[0] + header_start
            after = content.split(header_end)[-1]
            # 取最后一个 #endif 之后的内容
            idx = content.rfind(header_end)
            before = content[:idx]
            after = content[idx:]
            content = before + "\n" + insert_block + "\n" + after
        else:
            content += f"\n{insert_block}\n"

        out_path = os.path.join(out_dir, "ecf_configList.h")
        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(content)

    def _gen_user_json(self, out_dir):
        """生成用户配置文件（文件名可由用户自定义）"""
        data = {
            "enabled_modules": sorted(self.env_get_checked_modules()),
            "custom_paths": list(self.env_custom_paths),
            "define_overrides": dict(self.env_define_overrides),
            "excluded_defines": sorted(self.env_excluded_defines),
            "project_name": self.env_project_name,
        }
        fname = self.env_userjson_filename.strip() or "CFG_User"
        # 去除用户可能输入的后缀，统一加 .json
        if fname.endswith(".json"):
            fname = fname[:-5]
        fname += ".json"
        out_path = os.path.join(out_dir, fname)
        with open(out_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        return fname

    def set_status(self, message, is_error=False):
        self.status_message = message
        self.status_timer = 3000  # 3秒后消失

    # ============================================================
    # 滚动条拖拽
    # ============================================================
    def _sb_begin_drag(self, area_id, mouse_y, current_scroll, total_h, visible_h):
        """开始滚动条拖拽"""
        self._sb_drag = (area_id, mouse_y, current_scroll, total_h, visible_h)

    def _sb_update_drag(self, mouse_y):
        """拖拽中：返回新的 scroll 值；若未拖拽返回 None"""
        if self._sb_drag is None:
            return None
        _, start_y, start_scroll, total_h, visible_h = self._sb_drag
        max_sc = max(0, total_h - visible_h)
        if max_sc == 0:
            return None
        thumb_h = max(30, visible_h * visible_h / total_h)
        track_h = visible_h - thumb_h
        if track_h <= 0:
            return None
        delta = mouse_y - start_y
        new_scroll = start_scroll + delta * max_sc / track_h
        return max(0, min(max_sc, new_scroll))

    def _sb_end_drag(self):
        """结束滚动条拖拽"""
        self._sb_drag = None

    def _sb_check_thumb(self, mouse_pos, track_x, track_y, track_h,
                          total_h, visible_h, current_scroll):
        """检查鼠标是否在滚动条滑块上，返回 True 如果点击了滑块"""
        if total_h <= visible_h:
            return False
        mx, my = mouse_pos
        thumb_w = 6
        # 检查 x 坐标在滚动条范围内
        if not (track_x <= mx <= track_x + thumb_w):
            return False
        thumb_h = max(30, visible_h * visible_h / total_h)
        max_sc = total_h - visible_h
        thumb_y = track_y + (track_h - thumb_h) * current_scroll / max_sc
        # 检查 y 坐标在滑块范围内
        return thumb_y <= my <= thumb_y + thumb_h

    # ============================================================
    # 事件处理
    # ============================================================
    def handle_tab_events(self, event):
        """处理标签页区域的事件"""
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = event.pos
            tab_w = WIN_WIDTH // 3
            for i in range(3):
                tab_rect = pygame.Rect(i * tab_w, 0, tab_w, 42)
                if tab_rect.collidepoint(mx, my):
                    self.current_tab = i
                    # 切换标签时取消所有编辑状态
                    self.config_editing_define_index = -1
                    self.env_define_editing_index = -1
                    self.env_define_editing_module_name = ""
                    return True
        return False

    def handle_add_module_events(self, event):
        """处理添加模块区域的事件"""
        # 输入框事件
        if event.type == pygame.MOUSEBUTTONDOWN:
            if self.input_name.contains(event.pos):
                self.input_name.active = True
                self.input_note.active = False
                pygame.key.set_text_input_rect(self.input_name.rect)
                return True
            elif self.input_note.contains(event.pos):
                self.input_note.active = True
                self.input_name.active = False
                pygame.key.set_text_input_rect(self.input_note.rect)
                return True
            else:
                # 点击其他区域取消焦点
                if self.input_name.active or self.input_note.active:
                    self.input_name.active = False
                    self.input_note.active = False

            # 路径列表 - 点击删除
            path_base_y = 276  # path_list_rect.y + 4
            for i, path in enumerate(self.selected_paths):
                item_y = path_base_y + i * 32
                delete_rect = pygame.Rect(35 + 380 - 30, item_y + 4, 22, 22)
                if delete_rect.collidepoint(event.pos):
                    del self.selected_paths[i]
                    return True

            # 子模块标签 - 点击删除
            if self.selected_submodules:
                tag_x = 165
                tag_y = self.btn_bind_submodule.rect.y + 2
                for i, sm_name in enumerate(self.selected_submodules):
                    tag_text = clip_text(sm_name, font_small, 100)
                    tag_w = font_small.render(tag_text, True, (0, 0, 0)).get_width() + 20
                    tag_w = min(tag_w, 120)
                    tag_del = pygame.Rect(tag_x + tag_w - 18, tag_y + 4, 14, 18)
                    if tag_del.collidepoint(event.pos):
                        del self.selected_submodules[i]
                        return True
                    tag_x += tag_w + 6
                    if tag_x > 410:
                        break

        # IME 组合输入 / 完成输入（中文等）传递给激活的输入框
        if event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING):
            if self.input_name.active:
                self.input_name.handle_event(event)
                self.status_message = ""
                return True
            elif self.input_note.active:
                self.input_note.handle_event(event)
                self.status_message = ""
                return True

        # 键盘输入传递给激活的输入框
        if event.type == pygame.KEYDOWN:
            if self.input_name.active:
                self.input_name.handle_event(event)
                self.status_message = ""
                return True
            elif self.input_note.active:
                self.input_note.handle_event(event)
                self.status_message = ""
                return True

        # 按钮事件
        if self.btn_add_folder.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                folder = select_folder()
                if folder:
                    if folder not in self.selected_paths:
                        self.selected_paths.append(folder)
                        self.set_status(f"已添加路径: {folder}")
                    else:
                        self.set_status("该路径已存在", is_error=True)
            return True

        if self.btn_bind_submodule.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self._open_submodule_dialog()
            return True

        if self.btn_reset.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self.reset_add_area()
            return True

        if self.btn_confirm_add.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                if self.add_module(self.input_name.text, self.input_note.text,
                                   self.selected_paths, self.selected_submodules):
                    self.input_name.clear()
                    self.input_note.clear()
                    self.selected_paths.clear()
                    self.selected_submodules.clear()
                    self.editing_module_index = -1
            return True

        return False

    def handle_edit_module_events(self, event):
        """处理编辑模块区域的事件"""
        # 滚动事件
        self.module_list.handle_event(event)

        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = event.pos

            # 滚动条拖拽
            ml = self.module_list
            total_h = len(self.rules["modules"]) * ml.item_height
            visible_h = ml.rect.height - 10
            if total_h > visible_h:
                sb_x = ml.rect.right - 8
                if self._sb_check_thumb((mx, my), sb_x, ml.rect.y, visible_h,
                                         total_h, visible_h, ml.scroll_offset):
                    self._sb_begin_drag('modlist', my, ml.scroll_offset, total_h, visible_h)
                    return True

            # 检查确认对话框按钮
            if self.confirm_dialog_active:
                # 确认删除按钮
                confirm_btn_rect = pygame.Rect(WIN_WIDTH // 2 - 120, WIN_HEIGHT // 2 + 20, 100, 34)
                cancel_btn_rect = pygame.Rect(WIN_WIDTH // 2 + 20, WIN_HEIGHT // 2 + 20, 100, 34)
                if confirm_btn_rect.collidepoint(mx, my):
                    self.delete_module(self.confirm_dialog_index)
                    self.confirm_dialog_active = False
                    return True
                if cancel_btn_rect.collidepoint(mx, my):
                    self.confirm_dialog_active = False
                    return True
                # 点击对话框外部取消
                self.confirm_dialog_active = False
                return True

            # 检查模块列表项
            for i, mod in enumerate(self.rules["modules"]):
                item_rect = self.module_list.get_item_rect(i)
                if item_rect.collidepoint(mx, my):
                    # 上移按钮区域（第一个模块没有）
                    if i > 0:
                        up_btn_rect = pygame.Rect(
                            item_rect.x + item_rect.width - 112,
                            item_rect.y + (item_rect.height - 28) // 2,
                            28, 28
                        )
                        if up_btn_rect.collidepoint(mx, my):
                            self.move_module_up(i)
                            return True

                    # 删除按钮区域 (右侧)
                    delete_btn_rect = pygame.Rect(
                        item_rect.x + item_rect.width - 55,
                        item_rect.y + (item_rect.height - 28) // 2,
                        50, 28
                    )
                    if delete_btn_rect.collidepoint(mx, my):
                        self.confirm_dialog_active = True
                        self.confirm_dialog_index = i
                        self.confirm_dialog_module_name = mod["name"]
                        return True

                    # 点击模块名 -> 加载到编辑区
                    self.load_module_to_edit(i)
                    return True

        return False

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
                return

            # ---- 滚动条拖拽全局处理 ----
            if self._sb_drag is not None:
                if event.type == pygame.MOUSEMOTION:
                    new_val = self._sb_update_drag(event.pos[1])
                    if new_val is not None:
                        area_id = self._sb_drag[0]
                        if area_id == 'env':
                            self.env_scroll_y = int(new_val)
                        elif area_id == 'modlist':
                            self.module_list.scroll_offset = int(new_val)
                        elif area_id == 'cfgmod':
                            self.config_module_scroll = int(new_val)
                        elif area_id == 'cfgdef':
                            self.config_defines_scroll = int(new_val)
                    continue
                if event.type == pygame.MOUSEBUTTONUP:
                    self._sb_end_drag()
                    continue

            # 批量添加弹窗处理
            if self.batch_dialog_active:
                if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    mx, my = event.pos
                    # 点击文本编辑区激活 + 开始选择
                    if self.batch_text_input.contains((mx, my)):
                        self.batch_text_input.active = True
                        self.batch_text_input.handle_event(event)
                        pygame.key.set_text_input_rect(self.batch_text_input.rect)
                    else:
                        self.batch_text_input.active = False
                    # 确认按钮
                    if self.btn_batch_confirm.rect.collidepoint(mx, my):
                        text = self.batch_text_input.text.strip()
                        if text:
                            n = self.parse_and_batch_add_defines(text)
                            self.set_status(f"批量添加完成: 共 {n} 个宏定义")
                        else:
                            self.set_status("未输入任何内容", is_error=True)
                        self.batch_dialog_active = False
                        self.batch_text_input.clear()
                    # 取消按钮
                    if self.btn_batch_cancel.rect.collidepoint(mx, my):
                        self.batch_dialog_active = False
                        self.batch_text_input.clear()
                elif event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING, pygame.KEYDOWN,
                                    pygame.MOUSEMOTION, pygame.MOUSEBUTTONUP,
                                    pygame.MOUSEWHEEL):
                    if self.batch_text_input.active:
                        if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                            self.batch_dialog_active = False
                            self.batch_text_input.clear()
                        else:
                            self.batch_text_input.handle_event(event)
                continue

            # 帮助弹窗处理
            if self.help_active:
                if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    if self._help_ok_btn_rect.collidepoint(event.pos):
                        self.help_active = False
                elif event.type == pygame.KEYDOWN and event.key in (pygame.K_RETURN, pygame.K_ESCAPE):
                    self.help_active = False
                continue

            # 生成结果弹窗优先处理
            if self.gen_result_active:
                if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    if self._gen_ok_btn_rect.collidepoint(event.pos):
                        self.gen_result_active = False
                elif event.type == pygame.KEYDOWN and event.key == pygame.K_RETURN:
                    self.gen_result_active = False
                continue

            # 子模块对话框优先处理
            if self.submodule_dialog_active:
                self._handle_submodule_dialog_events(event)
                continue

            # config宏定义删除确认对话框
            if self.config_delete_dialog_active:
                self.handle_config_tab_events(event)
                continue

            # CMake模块确认对话框
            if self.confirm_dialog_active:
                self.handle_edit_module_events(event)
                continue

            # 环境文件生成 - 定义编辑模式拦截
            if self.env_define_editing_index >= 0 and self.current_tab == 0:
                self.handle_env_tab_events(event)
                continue

            # 帮助按钮点击检测（所有页面通用）
            if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                for btn_rect, section_key in self._help_btn_rects:
                    if btn_rect.collidepoint(event.pos):
                        title = self._get_help_title(section_key)
                        if title:
                            self.help_title = title
                            self.help_text = HELP_TEXTS.get(section_key, "")
                            self.help_active = True
                        return

            # 标签页切换
            if self.handle_tab_events(event):
                continue

            # 根据当前标签页分发事件
            if self.current_tab == 0:  # 环境文件生成
                if self.handle_env_tab_events(event):
                    continue
            elif self.current_tab == 1:  # 修改CMake模块
                if self.handle_add_module_events(event):
                    continue
                if self.handle_edit_module_events(event):
                    continue
            elif self.current_tab == 2:  # 修改config全局宏定义
                if self.handle_config_tab_events(event):
                    continue

    # ============================================================
    # 更新
    # ============================================================
    def update(self):
        self.dt = self.clock.tick(60)

        # 更新光标
        self.input_name.update(self.dt)
        self.input_note.update(self.dt)
        self.config_new_define_name.update(self.dt)
        self.config_new_define_value.update(self.dt)
        self.config_new_define_comment.update(self.dt)
        self.env_define_edit_input.update(self.dt)
        self.env_userjson_input.update(self.dt)
        self.batch_text_input.update(self.dt)

        # 更新状态计时器
        if self.status_timer > 0:
            self.status_timer -= self.dt
            if self.status_timer <= 0:
                self.status_message = ""

        # 更新模块列表滚动范围
        self.module_list.update_scroll(len(self.rules["modules"]))

        # 延迟处理：文件对话框（避免 tkinter 阻塞 pygame 事件循环）
        if getattr(self, '_pending_user_config_load', False):
            self._pending_user_config_load = False
            self.env_load_user_config()
        if getattr(self, '_pending_rule_load', False):
            self._pending_rule_load = False
            self._do_load_rule_tpl()
        if getattr(self, '_pending_tpl_load', ""):
            t = self._pending_tpl_load
            self._pending_tpl_load = ""
            self._env_select_tpl_file(t)

    # ============================================================
    # 绘制
    # ============================================================
    def draw_tab_bar(self):
        """绘制标签栏"""
        tab_w = WIN_WIDTH // 3
        for i, tab_name in enumerate(self.tabs):
            tab_rect = pygame.Rect(i * tab_w, 0, tab_w, 42)
            # 判断hover/active
            if i == self.current_tab:
                color = TAB_ACTIVE
            elif tab_rect.collidepoint(pygame.mouse.get_pos()):
                color = TAB_HOVER
            else:
                color = TAB_INACTIVE

            draw_rect(screen, tab_rect, color, border_radius=0)
            # 底部高亮线
            if i == self.current_tab:
                pygame.draw.line(screen, BUTTON_COLOR,
                                 (i * tab_w + 20, 41), (i * tab_w + tab_w - 20, 41), 3)

            text_surf = font_normal.render(tab_name, True, WHITE)
            text_rect = text_surf.get_rect(center=tab_rect.center)
            screen.blit(text_surf, text_rect)

        # 标签栏底部线条
        pygame.draw.line(screen, SEPARATOR, (0, 42), (WIN_WIDTH, 42), 1)

    def draw_empty_tab(self, title):
        """绘制空白标签页"""
        text_surf = font_title.render(title, True, TEXT_DIM)
        text_rect = text_surf.get_rect(center=(WIN_WIDTH // 2, WIN_HEIGHT // 2 - 20))
        screen.blit(text_surf, text_rect)

        sub_text = font_small.render("（此功能暂未实现）", True, TEXT_DIM)
        sub_rect = sub_text.get_rect(center=(WIN_WIDTH // 2, WIN_HEIGHT // 2 + 20))
        screen.blit(sub_text, sub_rect)

    def draw_add_module_area(self):
        """绘制"添加模块"区域"""
        # 区域标题
        draw_text(screen, "添加模块", 35, 57, font_bold, WHITE)
        self._draw_help_button(400, 57, 'add_module')
        if self.editing_module_index >= 0:
            edit_hint = font_small.render("（编辑模式）", True, (255, 200, 80))
            screen.blit(edit_hint, (130, 60))

        # 模块名输入
        draw_text(screen, "模块名:", 35, 85, font_small, TEXT_DIM)
        self.input_name.draw(screen)

        # 备注输入
        draw_text(screen, "备注:", 35, 155, font_small, TEXT_DIM)
        self.input_note.draw(screen)

        # 已选路径区域
        path_area_y = 250
        draw_text(screen, "包含文件夹路径:", 35, path_area_y, font_small, TEXT_DIM)

        # 路径列表背景（高度缩小，为子模块区域让位）
        path_list_rect = pygame.Rect(35, path_area_y + 22, 385, 175)
        draw_rect(screen, path_list_rect, PANEL_COLOR)
        draw_rect_outline(screen, path_list_rect, SEPARATOR, width=1)

        if not self.selected_paths:
            hint = font_small.render("点击下方\"包含文件夹\"按钮添加路径", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=path_list_rect.center)
            screen.blit(hint, hint_rect)
        else:
            for i, path in enumerate(self.selected_paths):
                item_y = path_list_rect.y + 4 + i * 32
                # 超出区域则不绘制
                if item_y + 28 > path_list_rect.bottom - 4:
                    break

                item_rect = pygame.Rect(path_list_rect.x + 4, item_y, path_list_rect.width - 8, 28)
                draw_rect(screen, item_rect, ITEM_BG)

                path_text = clip_text(path, font_small, item_rect.width - 40)
                draw_text(screen, path_text, item_rect.x + 8, item_rect.y + 5, font_small)

                # 删除按钮
                del_btn = pygame.Rect(item_rect.x + item_rect.width - 28, item_rect.y + 3, 22, 22)
                is_hover = del_btn.collidepoint(pygame.mouse.get_pos())
                del_color = BUTTON_DELETE_HOVER if is_hover else BUTTON_DELETE
                draw_rect(screen, del_btn, del_color)
                x_mark = font_small.render("X", True, WHITE)
                x_rect = x_mark.get_rect(center=del_btn.center)
                screen.blit(x_mark, x_rect)

        # 子模块绑定区域
        sub_area_y = 455
        draw_text(screen, "绑定子模块:", 35, sub_area_y, font_small, TEXT_DIM)
        self.btn_bind_submodule.draw(screen)

        # 已绑定的子模块标签（与按钮同一行）
        if self.selected_submodules:
            tag_x = 165
            tag_y = self.btn_bind_submodule.rect.y + 2
            for sm_name in self.selected_submodules:
                tag_text = clip_text(sm_name, font_small, 100)
                tag_w = font_small.render(tag_text, True, (0, 0, 0)).get_width() + 20
                tag_rect = pygame.Rect(tag_x, tag_y, min(tag_w, 120), 26)
                draw_rect(screen, tag_rect, (60, 90, 140))
                tag_surf = font_small.render(tag_text, True, WHITE)
                screen.blit(tag_surf, (tag_rect.x + 10, tag_rect.y + 4))
                # 删除按钮
                tag_del = pygame.Rect(tag_rect.right - 18, tag_rect.y + 4, 14, 18)
                is_hover = tag_del.collidepoint(pygame.mouse.get_pos())
                del_col = BUTTON_DELETE_HOVER if is_hover else BUTTON_DELETE
                draw_rect(screen, tag_del, del_col)
                x_s = font_small.render("X", True, WHITE)
                screen.blit(x_s, (tag_del.x + 1, tag_del.y - 1))
                tag_x += tag_rect.width + 6
                if tag_x > 410:
                    break

        # 按钮
        self.btn_add_folder.draw(screen)
        self.btn_reset.draw(screen)
        self.btn_confirm_add.draw(screen)

        # 编辑模式提示
        if self.editing_module_index >= 0:
            tip = font_small.render("点击确认添加将更新此模块", True, (255, 200, 80))
            screen.blit(tip, (35, 600))

    def draw_edit_module_area(self):
        """绘制"编辑模块"区域"""
        # 区域标题
        draw_text(screen, "编辑模块", 485, 55, font_bold, WHITE)
        self._draw_help_button(890, 55, 'edit_module')

        # 模块数量
        count_text = f"共 {len(self.rules['modules'])} 个模块"
        draw_text(screen, count_text, 665, 58, font_small, TEXT_DIM)

        # 模块列表背景
        list_rect = self.module_list.rect
        draw_rect(screen, list_rect, PANEL_COLOR)
        draw_rect_outline(screen, list_rect, SEPARATOR, width=1)

        # 裁剪绘制区域
        screen.set_clip(list_rect)

        _edit_tooltip = None  # 备注 tooltip

        if not self.rules["modules"]:
            hint = font_small.render("暂无模块，请在左侧添加", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=list_rect.center)
            screen.blit(hint, hint_rect)
        else:
            for i, mod in enumerate(self.rules["modules"]):
                item_rect = self.module_list.get_item_rect(i)
                # 跳过完全不可见的项
                if item_rect.bottom < list_rect.y or item_rect.top > list_rect.bottom:
                    continue

                # 判断状态
                is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())
                is_editing = (i == self.editing_module_index)

                if is_editing:
                    bg_color = ITEM_SELECTED
                elif is_hovered:
                    bg_color = ITEM_HOVER
                else:
                    bg_color = ITEM_BG

                draw_rect(screen, item_rect, bg_color)

                # 模块名
                name_text = clip_text(mod["name"], font_bold, 200)
                name_surf = font_bold.render(name_text, True, WHITE if is_editing else TEXT_COLOR)
                screen.blit(name_surf, (item_rect.x + 10, item_rect.y + 6))

                # 备注
                full_note = mod.get("note", "")
                note_text = clip_text(full_note, font_small, 250)
                note_color = TEXT_DIM if not note_text else (200, 200, 200) if not is_editing else (220, 220, 240)
                note_surf = font_small.render(note_text or "(无备注)", True, note_color)
                screen.blit(note_surf, (item_rect.x + 10, item_rect.y + 28))
                # 备注 tooltip
                if is_hovered and full_note and note_text != full_note and _edit_tooltip is None:
                    _edit_tooltip = collect_text_tooltip(full_note, font_small, 250,
                                                          item_rect, list_rect)

                # 路径数量 + 子模块数量
                path_count = len(mod.get("paths", []))
                sub_count = len(mod.get("submodules", []))
                if sub_count > 0:
                    info = f"{path_count} 个路径 · {sub_count} 个子模块"
                else:
                    info = f"{path_count} 个路径"
                info_surf = font_small.render(info, True, TEXT_DIM)
                screen.blit(info_surf, (item_rect.x + 10, item_rect.y + 48))

                # 上移按钮（第一个模块不显示）
                if i > 0:
                    up_btn_rect = pygame.Rect(
                        item_rect.x + item_rect.width - 112,
                        item_rect.y + (item_rect.height - 28) // 2,
                        28, 28
                    )
                    up_hovered = up_btn_rect.collidepoint(pygame.mouse.get_pos())
                    up_color = BUTTON_HOVER if up_hovered else BUTTON_COLOR
                    draw_rect(screen, up_btn_rect, up_color)
                    # 用线条画上箭头 ▲
                    cx, cy = up_btn_rect.centerx, up_btn_rect.centery
                    pygame.draw.polygon(screen, WHITE, [
                        (cx, cy - 4), (cx - 5, cy + 3), (cx + 5, cy + 3)
                    ])

                # 删除按钮
                del_btn_rect = pygame.Rect(
                    item_rect.x + item_rect.width - 55,
                    item_rect.y + (item_rect.height - 28) // 2,
                    50, 28
                )
                del_hovered = del_btn_rect.collidepoint(pygame.mouse.get_pos())
                del_color = BUTTON_DELETE_HOVER if del_hovered else BUTTON_DELETE
                draw_rect(screen, del_btn_rect, del_color)
                del_text = font_small.render("删除", True, WHITE)
                del_text_rect = del_text.get_rect(center=del_btn_rect.center)
                screen.blit(del_text, del_text_rect)

        # 恢复裁剪
        screen.set_clip(None)

        # 绘制备注 tooltip
        if _edit_tooltip is not None:
            tip_x, tip_y, tip_w, tip_h, lines = _edit_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, line in enumerate(lines):
                s_surf = font_small.render(line, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))

        # 滚动条
        self.module_list.draw_scrollbar(screen, len(self.rules["modules"]))

    # ============================================================
    # 修改config全局宏定义 - 绘制
    # ============================================================
    def draw_config_tab(self):
        """绘制修改config全局宏定义标签页"""
        # ---- 左侧：模块列表 ----
        draw_text(screen, "模块列表", 20, 55, font_bold, WHITE)
        self._draw_help_button(265, 55, 'config_module_list')
        draw_rect(screen, self.config_module_list_rect, PANEL_COLOR)
        draw_rect_outline(screen, self.config_module_list_rect, SEPARATOR, width=1)

        screen.set_clip(self.config_module_list_rect)
        item_h = 46
        modules = self.rules["modules"]

        if not modules:
            hint = font_small.render("暂无模块", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=self.config_module_list_rect.center)
            screen.blit(hint, hint_rect)
        else:
            for i, mod in enumerate(modules):
                item_y = self.config_module_list_rect.y + 4 + i * item_h - self.config_module_scroll
                if item_y + item_h < self.config_module_list_rect.y or item_y > self.config_module_list_rect.bottom:
                    continue

                item_rect = pygame.Rect(
                    self.config_module_list_rect.x + 4, item_y,
                    self.config_module_list_rect.width - 16, item_h - 4
                )
                is_selected = (mod["name"] == self.config_selected_module)
                is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())

                if is_selected:
                    bg = ITEM_SELECTED
                elif is_hovered:
                    bg = ITEM_HOVER
                else:
                    bg = ITEM_BG
                draw_rect(screen, item_rect, bg)

                name_surf = font_bold.render(mod["name"], True, WHITE if is_selected else TEXT_COLOR)
                screen.blit(name_surf, (item_rect.x + 10, item_rect.y + 6))
                # 宏定义数量
                def_count = len(mod.get("defines", []))
                count_surf = font_small.render(f"{def_count} 个宏", True, TEXT_DIM)
                screen.blit(count_surf, (item_rect.x + 10, item_rect.y + 26))

        screen.set_clip(None)
        # 模块列表滚动条
        total_h = len(modules) * item_h
        visible_h = self.config_module_list_rect.height
        if total_h > visible_h:
            thumb_h = max(30, visible_h * visible_h / total_h)
            thumb_y = self.config_module_list_rect.y + (visible_h - thumb_h) * self.config_module_scroll / (total_h - visible_h)
            draw_rect(screen, (self.config_module_list_rect.right - 8, thumb_y, 6, thumb_h), SCROLLBAR_THUMB)

        # ---- 右侧：编辑宏定义 ----
        draw_text(screen, "编辑宏定义", 310, 55, font_bold, WHITE)
        self._draw_help_button(910, 55, 'config_define_edit')
        if self.config_selected_module:
            sel_surf = font_small.render(f"- {self.config_selected_module}", True, (255, 200, 80))
            screen.blit(sel_surf, (420, 58))

        draw_rect(screen, self.config_defines_rect, PANEL_COLOR)
        draw_rect_outline(screen, self.config_defines_rect, SEPARATOR, width=1)

        defines = self._get_selected_module_defines()
        screen.set_clip(self.config_defines_rect)

        _cfg_tooltip = None  # 注释 tooltip

        def_item_h = 44

        if not self.config_selected_module:
            hint = font_small.render("请在左侧选择一个模块", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=self.config_defines_rect.center)
            screen.blit(hint, hint_rect)
        elif not defines:
            hint = font_small.render("暂无宏定义，点击下方\"添加定义\"按钮新增", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=self.config_defines_rect.center)
            screen.blit(hint, hint_rect)
        else:
            for i, d in enumerate(defines):
                item_y = self.config_defines_rect.y + 4 + i * def_item_h - self.config_defines_scroll
                if item_y + def_item_h < self.config_defines_rect.y or item_y > self.config_defines_rect.bottom:
                    continue

                item_rect = pygame.Rect(
                    self.config_defines_rect.x + 4, item_y,
                    self.config_defines_rect.width - 16, def_item_h - 4
                )
                is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())
                bg = ITEM_HOVER if is_hovered else ITEM_BG
                draw_rect(screen, item_rect, bg)

                # 定义名 (固定列宽，对齐后续内容)
                name_col_w = 180
                name_rect = pygame.Rect(item_rect.x + 8, item_rect.y + 6, name_col_w, 22)
                draw_rect(screen, name_rect, (60, 100, 170))
                name_surf = font_small.render(clip_text(d["name"], font_small, name_col_w - 10), True, WHITE)
                screen.blit(name_surf, (name_rect.x + 6, name_rect.y + 3))

                # 定义内容
                val_x = name_rect.right + 12
                val_surf = font_small.render(clip_text(d["value"], font_small, 170), True, TEXT_COLOR)
                screen.blit(val_surf, (val_x, item_rect.y + 8))

                # 注释
                cmt_x = val_x + 180
                cmt_max_w = item_rect.right - 55 - cmt_x - 10  # 避开删除按钮
                cmt = d.get("comment", "")
                if cmt:
                    clipped_c = clip_text(cmt, font_small, cmt_max_w)
                    cmt_surf = font_small.render(clipped_c, True, TEXT_DIM)
                    screen.blit(cmt_surf, (cmt_x, item_rect.y + 8))
                    # 注释 tooltip
                    if is_hovered and clipped_c != cmt and _cfg_tooltip is None:
                        _cfg_tooltip = collect_text_tooltip(cmt, font_small, cmt_max_w,
                                                             item_rect, self.config_defines_rect)

                # 删除按钮
                del_btn = pygame.Rect(item_rect.right - 55, item_rect.y + 5, 48, 28)
                del_hover = del_btn.collidepoint(pygame.mouse.get_pos())
                del_col = BUTTON_DELETE_HOVER if del_hover else BUTTON_DELETE
                draw_rect(screen, del_btn, del_col)
                del_surf = font_small.render("删除", True, WHITE)
                del_rect2 = del_surf.get_rect(center=del_btn.center)
                screen.blit(del_surf, del_rect2)

        screen.set_clip(None)

        # 绘制注释 tooltip
        if _cfg_tooltip is not None:
            tip_x, tip_y, tip_w, tip_h, lines = _cfg_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, line in enumerate(lines):
                s_surf = font_small.render(line, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))

        # 滚动条
        total_def_h = len(defines) * def_item_h
        visible_def_h = self.config_defines_rect.height
        if total_def_h > visible_def_h:
            thumb_h = max(30, visible_def_h * visible_def_h / total_def_h)
            max_sc = total_def_h - visible_def_h
            thumb_y = self.config_defines_rect.y + (visible_def_h - thumb_h) * self.config_defines_scroll / max_sc
            draw_rect(screen, (self.config_defines_rect.right - 8, thumb_y, 6, thumb_h), SCROLLBAR_THUMB)

        # ---- 底部：添加/编辑定义 ----
        if self.config_selected_module:
            if self.config_adding_define:
                is_editing = (self.config_editing_define_index >= 0)
                mode_label = "编辑定义" if is_editing else "添加定义"
                draw_text(screen, mode_label, 310, 494, font_bold, (255, 200, 80) if is_editing else WHITE)
                draw_text(screen, "定义名:", 310, 510, font_small, TEXT_DIM)
                self.config_new_define_name.draw(screen)
                draw_text(screen, "定义内容:", 500, 510, font_small, TEXT_DIM)
                self.config_new_define_value.draw(screen)
                # 按钮放在定义内容右边
                self.btn_config_confirm_define.text = "确认更新" if is_editing else "确认添加"
                self.btn_config_confirm_define.draw(screen)
                self.btn_config_cancel_define.draw(screen)
                # 注释行
                draw_text(screen, "注释:", 310, 550, font_small, TEXT_DIM)
                self.config_new_define_comment.draw(screen)
            else:
                self.btn_config_add_define.draw(screen)
                self.btn_config_batch_define.draw(screen)

    # ============================================================
    # 修改config全局宏定义 - 事件处理
    # ============================================================
    def handle_config_tab_events(self, event):
        """处理修改config全局宏定义标签页的事件"""

        # ---- 添加/编辑定义模式下的输入框事件 ----
        if self.config_adding_define:
            if event.type == pygame.MOUSEBUTTONDOWN:
                mx, my = event.pos
                if self.config_new_define_name.contains((mx, my)):
                    self.config_new_define_name.active = True
                    self.config_new_define_value.active = False
                    self.config_new_define_comment.active = False
                    pygame.key.set_text_input_rect(self.config_new_define_name.rect)
                    return True
                elif self.config_new_define_value.contains((mx, my)):
                    self.config_new_define_value.active = True
                    self.config_new_define_name.active = False
                    self.config_new_define_comment.active = False
                    pygame.key.set_text_input_rect(self.config_new_define_value.rect)
                    return True
                elif self.config_new_define_comment.contains((mx, my)):
                    self.config_new_define_comment.active = True
                    self.config_new_define_name.active = False
                    self.config_new_define_value.active = False
                    pygame.key.set_text_input_rect(self.config_new_define_comment.rect)
                    return True
                else:
                    self.config_new_define_name.active = False
                    self.config_new_define_value.active = False
                    self.config_new_define_comment.active = False

                # 确认/取消按钮
                if self.btn_config_confirm_define.handle_event(event):
                    if event.type == pygame.MOUSEBUTTONDOWN:
                        name = self.config_new_define_name.text
                        value = self.config_new_define_value.text
                        comment = self.config_new_define_comment.text
                        if self.config_editing_define_index >= 0:
                            # 编辑模式：更新已有定义
                            if self.update_define_in_module(
                                self.config_editing_define_index, name, value, comment
                            ):
                                self._config_reset_form()
                        else:
                            # 新增模式
                            if self.add_define_to_module(name, value, comment):
                                self._config_reset_form()
                    return True
                if self.btn_config_cancel_define.handle_event(event):
                    if event.type == pygame.MOUSEBUTTONDOWN:
                        self._config_reset_form()
                        self.set_status("已取消")
                    return True

            # TEXTINPUT / KEYDOWN 传递给激活的输入框
            if event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING):
                for inp in [self.config_new_define_name, self.config_new_define_value, self.config_new_define_comment]:
                    if inp.active:
                        inp.handle_event(event)
                        self.status_message = ""
                        return True
            if event.type == pygame.KEYDOWN:
                for inp in [self.config_new_define_name, self.config_new_define_value, self.config_new_define_comment]:
                    if inp.active:
                        inp.handle_event(event)
                        self.status_message = ""
                        return True
            return False

        # ---- 删除确认对话框 ----
        if self.config_delete_dialog_active:
            if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                mx, my = event.pos
                confirm_btn = pygame.Rect(WIN_WIDTH // 2 - 120, WIN_HEIGHT // 2 + 20, 100, 34)
                cancel_btn = pygame.Rect(WIN_WIDTH // 2 + 20, WIN_HEIGHT // 2 + 20, 100, 34)
                if confirm_btn.collidepoint(mx, my):
                    self.delete_define_from_module(self.config_delete_define_index)
                    self.config_delete_dialog_active = False
                    return True
                if cancel_btn.collidepoint(mx, my):
                    self.config_delete_dialog_active = False
                    return True
                self.config_delete_dialog_active = False
                return True
            return True  # 对话框打开时拦截所有事件

        # ---- 正常模式事件 ----
        if event.type == pygame.MOUSEWHEEL:
            mx, my = pygame.mouse.get_pos()
            if self.config_module_list_rect.collidepoint(mx, my):
                modules = self.rules["modules"]
                total_h = len(modules) * 46
                visible_h = self.config_module_list_rect.height
                max_sc = max(0, total_h - visible_h)
                self.config_module_scroll = max(0, min(max_sc, self.config_module_scroll - event.y * 30))
                return True
            if self.config_defines_rect.collidepoint(mx, my):
                defines = self._get_selected_module_defines()
                total_h = len(defines) * 44
                visible_h = self.config_defines_rect.height
                max_sc = max(0, total_h - visible_h)
                self.config_defines_scroll = max(0, min(max_sc, self.config_defines_scroll - event.y * 30))
                return True

        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = event.pos

            # ---- 滚动条拖拽 ----
            modules = self.rules["modules"]
            # 左侧模块列表滚动条
            total_h = len(modules) * 46
            visible_h = self.config_module_list_rect.height
            if total_h > visible_h:
                sb_x = self.config_module_list_rect.right - 8
                if self._sb_check_thumb((mx, my), sb_x, self.config_module_list_rect.y,
                                         visible_h, total_h, visible_h, self.config_module_scroll):
                    self._sb_begin_drag('cfgmod', my, self.config_module_scroll,
                                         total_h, visible_h)
                    return True
            # 右侧宏定义列表滚动条
            defines = self._get_selected_module_defines()
            total_h = len(defines) * 44
            visible_h = self.config_defines_rect.height
            if total_h > visible_h:
                sb_x = self.config_defines_rect.right - 8
                if self._sb_check_thumb((mx, my), sb_x, self.config_defines_rect.y,
                                         visible_h, total_h, visible_h, self.config_defines_scroll):
                    self._sb_begin_drag('cfgdef', my, self.config_defines_scroll,
                                         total_h, visible_h)
                    return True

            # 左侧模块列表点击
            if self.config_module_list_rect.collidepoint(mx, my):
                item_h = 46
                for i, mod in enumerate(self.rules["modules"]):
                    item_y = self.config_module_list_rect.y + 4 + i * item_h - self.config_module_scroll
                    item_rect = pygame.Rect(
                        self.config_module_list_rect.x + 4, item_y,
                        self.config_module_list_rect.width - 16, item_h - 4
                    )
                    if item_rect.collidepoint(mx, my):
                        self.config_selected_module = mod["name"]
                        self.config_defines_scroll = 0
                        self.config_adding_define = False
                        self.set_status(f"已选择模块 '{mod['name']}'")
                        return True

            # 右侧宏定义列表点击
            if self.config_defines_rect.collidepoint(mx, my):
                defines = self._get_selected_module_defines()
                def_item_h = 44
                for i, d in enumerate(defines):
                    item_y = self.config_defines_rect.y + 4 + i * def_item_h - self.config_defines_scroll
                    item_rect = pygame.Rect(
                        self.config_defines_rect.x + 4, item_y,
                        self.config_defines_rect.width - 16, def_item_h - 4
                    )
                    if item_rect.collidepoint(mx, my):
                        # 删除按钮区域
                        del_btn = pygame.Rect(item_rect.right - 55, item_rect.y + 5, 48, 28)
                        if del_btn.collidepoint(mx, my):
                            self.config_delete_dialog_active = True
                            self.config_delete_define_index = i
                            self.config_delete_define_name = d["name"]
                            return True
                        # 点击定义 → 加载到编辑表单
                        self.config_editing_define_index = i
                        self.config_adding_define = True
                        self.config_new_define_name.set_text(d["name"])
                        self.config_new_define_value.set_text(d["value"])
                        self.config_new_define_comment.set_text(d.get("comment", ""))
                        self.config_new_define_name.active = True
                        self.config_new_define_value.active = False
                        self.config_new_define_comment.active = False
                        pygame.key.set_text_input_rect(self.config_new_define_name.rect)
                        self.set_status(f"正在编辑 '{d['name']}'")
                        return True

            # 添加定义按钮 → 清空表单进入新增模式
            if self.btn_config_add_define.handle_event(event):
                if event.type == pygame.MOUSEBUTTONDOWN and self.config_selected_module:
                    self.config_editing_define_index = -1
                    self.config_adding_define = True
                    self.config_new_define_name.clear()
                    self.config_new_define_value.clear()
                    self.config_new_define_comment.clear()
                    self.config_new_define_name.active = True
                    self.config_new_define_value.active = False
                    self.config_new_define_comment.active = False
                    pygame.key.set_text_input_rect(self.config_new_define_name.rect)
                    self.set_status("新增宏定义")
                return True

            # 批量添加按钮 → 打开弹窗
            if self.btn_config_batch_define.handle_event(event):
                if event.type == pygame.MOUSEBUTTONDOWN and self.config_selected_module:
                    self.batch_text_input.clear()
                    self.batch_text_input.active = True
                    self.batch_dialog_active = True
                    pygame.key.set_text_input_rect(self.batch_text_input.rect)
                    self.set_status("批量添加模式：粘贴宏定义文本后点击确认")
                return True

        return False

    def draw_config_delete_dialog(self):
        """绘制宏定义删除确认对话框"""
        if not self.config_delete_dialog_active:
            return
        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        dialog_w, dialog_h = 380, 150
        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect, BUTTON_COLOR, width=2)

        title = font_bold.render("确认删除宏定义", True, WHITE)
        title_rect = title.get_rect(center=(WIN_WIDTH // 2, dialog_y + 30))
        screen.blit(title, title_rect)

        msg = font_small.render(f"确定要删除宏定义 \"{self.config_delete_define_name}\" 吗？", True, TEXT_COLOR)
        msg_rect = msg.get_rect(center=(WIN_WIDTH // 2, dialog_y + 65))
        screen.blit(msg, msg_rect)

        mx, my = pygame.mouse.get_pos()
        confirm_btn = Button(WIN_WIDTH // 2 - 120, dialog_y + 95, 100, 34, "确认删除",
                             color=BUTTON_DELETE, font_obj=font_small)
        cancel_btn = Button(WIN_WIDTH // 2 + 20, dialog_y + 95, 100, 34, "取消",
                            color=(100, 100, 105), font_obj=font_small)
        confirm_btn.hovered = confirm_btn.rect.collidepoint(mx, my)
        cancel_btn.hovered = cancel_btn.rect.collidepoint(mx, my)
        confirm_btn.draw(screen)
        cancel_btn.draw(screen)

    # ============================================================
    # 环境文件生成 - 绘制
    # ============================================================
    CONTENT_LEFT = 35
    CONTENT_WIDTH = 890
    TAB0_TOP = 50   # 内容区起始 Y

    def draw_env_tab(self):
        """绘制环境文件生成标签页（整个页面可上下滚动）"""
        # 整体内容裁剪，防止滚动时覆盖标签栏
        env_clip = pygame.Rect(0, self.TAB0_TOP, WIN_WIDTH, WIN_HEIGHT - self.TAB0_TOP - 28)
        screen.set_clip(env_clip)

        sy = self.env_scroll_y  # scroll offset
        top = self.TAB0_TOP
        left = self.CONTENT_LEFT
        cw = self.CONTENT_WIDTH
        visible_bottom = WIN_HEIGHT - 30

        # ---- 区域1：用户配置读取 ----
        area1_y = top - sy
        self._draw_env_section_header("用户配置读取", left, area1_y, 'env_user_config')
        area1_h = 100
        self._btn_path_labels = {}  # 每帧重建 tooltip 映射

        # Row 1: [选择配置文件] [载入CMakeList模版]
        row1_y = area1_y + 28
        self.btn_env_load_config.rect.y = row1_y
        self.btn_env_load_config.draw(screen)
        self._btn_path_labels[self.btn_env_load_config] = (
            self.env_config_path or "未加载", (100, 220, 100) if self.env_config_path else TEXT_DIM)

        self.btn_env_load_cmake_tpl.rect.y = row1_y
        self.btn_env_load_cmake_tpl.draw(screen)
        self._btn_path_labels[self.btn_env_load_cmake_tpl] = (
            self.env_cmake_tpl_path, (100, 220, 100))

        # Row 2: [载入配置模版] [载入ecf_configList模版]
        row2_y = area1_y + 60
        self.btn_env_load_user_cfg.rect.y = row2_y
        self.btn_env_load_user_cfg.draw(screen)
        self._btn_path_labels[self.btn_env_load_user_cfg] = (
            self.env_rule_tpl_path, (100, 220, 100))

        self.btn_env_load_config_tpl.rect.y = row2_y
        self.btn_env_load_config_tpl.draw(screen)
        self._btn_path_labels[self.btn_env_load_config_tpl] = (
            self.env_config_tpl_path, (100, 220, 100))

        # 在每个按钮右侧显示当前路径 + 悬停 tooltip
        _area1_tooltip = None
        for btn, (path, color) in self._btn_path_labels.items():
            path_x = btn.rect.right + 8
            display = clip_text(os.path.basename(path), font_small, 200)
            draw_text(screen, display, path_x, btn.rect.y + 4, font_small, color)
            # 按钮悬停时显示完整路径
            if btn.hovered and path and path != "未加载":
                _area1_tooltip = collect_text_tooltip(
                    path, font_small, 800, btn.rect,
                    pygame.Rect(left, area1_y, cw, area1_h))

        # 在裁剪区外绘制 area1 tooltip
        if _area1_tooltip is not None:
            screen.set_clip(None)
            tip_x, tip_y, tip_w, tip_h, lines = _area1_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, line in enumerate(lines):
                s_surf = font_small.render(line, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))
            if env_clip:
                screen.set_clip(env_clip)

        # ---- 区域2：CMakeList模块启用 ----
        area2_y = area1_y + area1_h + 15
        self._draw_env_section_header("CMakeList模块启用", left, area2_y, 'env_module_enable')

        mod_list_y = area2_y + 27
        mod_item_h = 34
        modules = self.rules["modules"]
        mod_list_h = max(60, len(modules) * mod_item_h + 8)
        mod_list_rect = pygame.Rect(left, mod_list_y, cw, mod_list_h)
        draw_rect(screen, mod_list_rect, PANEL_COLOR)
        draw_rect_outline(screen, mod_list_rect, SEPARATOR, width=1)

        # 收集悬停 tooltip 信息（延迟到 clip 外绘制）
        _hover_tooltip = None

        screen.set_clip(mod_list_rect.clip(env_clip))
        for i, mod in enumerate(modules):
            item_y = mod_list_rect.y + 4 + i * mod_item_h
            if item_y + mod_item_h < mod_list_rect.y or item_y > mod_list_rect.bottom:
                continue
            item_rect = pygame.Rect(mod_list_rect.x + 4, item_y,
                                    mod_list_rect.width - 8, mod_item_h - 4)
            is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())
            bg = ITEM_HOVER if is_hovered else ITEM_BG
            draw_rect(screen, item_rect, bg)

            # 勾选框
            cb_rect = pygame.Rect(item_rect.x + 8, item_rect.y + 7, 20, 20)
            checked = self.env_module_checks.get(mod["name"], False)
            if checked:
                draw_rect(screen, cb_rect, (80, 180, 80))
                # 白色对勾
                cx, cy = cb_rect.centerx, cb_rect.centery
                pygame.draw.line(screen, WHITE, (cx - 3, cy), (cx - 1, cy + 2), 2)
                pygame.draw.line(screen, WHITE, (cx - 1, cy + 2), (cx + 3, cy - 3), 2)
            else:
                draw_rect_outline(screen, cb_rect, TEXT_DIM, border_radius=2, width=1)

            # 模块名
            name_surf = font_normal.render(mod["name"], True, WHITE if checked else TEXT_COLOR)
            screen.blit(name_surf, (item_rect.x + 36, item_rect.y + 6))

            # 备注
            note = mod.get("note", "")
            clipped_note = clip_text(note, font_small, 300)
            note_surf = font_small.render(clipped_note, True, TEXT_DIM)
            screen.blit(note_surf, (item_rect.x + 250, item_rect.y + 8))
            # 备注 tooltip：文本被裁切且悬停时收集
            if is_hovered and note and clipped_note != note:
                _hover_tooltip = collect_text_tooltip(note, font_small, 300,
                                                       item_rect, mod_list_rect)

            # 子模块数量
            submodules = mod.get("submodules", [])
            sub_count = len(submodules)
            if sub_count > 0:
                sub_surf = font_small.render(f"{sub_count}个子模块", True, TEXT_DIM)
                screen.blit(sub_surf, (item_rect.x + 500, item_rect.y + 8))

                # 记录 tooltip 信息，稍后在 clip 外绘制（仅当没有备注 tooltip 时）
                if is_hovered and _hover_tooltip is None:
                    max_name_w = max((font_small.render(s, True, (0,0,0)).get_width() for s in submodules), default=80)
                    tip_w = max_name_w + 24
                    tip_h = sub_count * 20 + 16
                    mx, my = pygame.mouse.get_pos()
                    tip_x = mx + 16
                    if tip_x + tip_w > mod_list_rect.right - 4:
                        tip_x = mx - tip_w - 8
                    tip_y = min(my, mod_list_rect.bottom - tip_h - 4)
                    tip_y = max(tip_y, mod_list_rect.y + 4)
                    _hover_tooltip = (tip_x, tip_y, tip_w, tip_h, submodules)

        screen.set_clip(env_clip)

        # 在裁剪区域外绘制 tooltip，确保不被遮挡
        if _hover_tooltip is not None:
            tip_x, tip_y, tip_w, tip_h, submodules = _hover_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, sname in enumerate(submodules):
                s_surf = font_small.render(sname, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))

        # ---- 区域3：CMakeList自定义 ----
        area3_y = mod_list_rect.bottom + 15
        self._draw_env_section_header("CMakeList自定义", left, area3_y, 'env_custom_paths')

        # 项目名称输入
        draw_text(screen, "项目名称：", left, area3_y + 32, font_small, TEXT_DIM)
        self.env_project_input.rect.x = left + 85
        self.env_project_input.rect.y = area3_y + 28
        self.env_project_input.draw(screen)

        self.btn_env_add_path.rect.y = area3_y + 65
        self.btn_env_add_path.draw(screen)

        path_list_y = area3_y + 101  # 按钮底部(32+32) + 4px 间距
        path_list_h = max(50, len(self.env_custom_paths) * 30 + 8)
        path_list_rect = pygame.Rect(left, path_list_y, cw, path_list_h)
        draw_rect(screen, path_list_rect, PANEL_COLOR)
        draw_rect_outline(screen, path_list_rect, SEPARATOR, width=1)

        screen.set_clip(path_list_rect.clip(env_clip))
        if not self.env_custom_paths:
            hint = font_small.render("点击\"选择文件夹\"添加自定义路径", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=path_list_rect.center)
            screen.blit(hint, hint_rect)
        else:
            for i, p in enumerate(self.env_custom_paths):
                item_y = path_list_rect.y + 4 + i * 30
                if item_y + 28 > path_list_rect.bottom:
                    break
                pi_rect = pygame.Rect(path_list_rect.x + 4, item_y, path_list_rect.width - 8, 26)
                draw_rect(screen, pi_rect, ITEM_BG)
                draw_text(screen, clip_text(p, font_small, pi_rect.width - 40),
                          pi_rect.x + 8, pi_rect.y + 4, font_small)
                # 删除
                del_r = pygame.Rect(pi_rect.right - 28, pi_rect.y + 2, 22, 22)
                is_hov = del_r.collidepoint(pygame.mouse.get_pos())
                draw_rect(screen, del_r, BUTTON_DELETE_HOVER if is_hov else BUTTON_DELETE)
                x_s = font_small.render("X", True, WHITE)
                screen.blit(x_s, x_s.get_rect(center=del_r.center))
        screen.set_clip(env_clip)

        # ---- 区域4：ecf_configList编辑 ----
        area4_y = path_list_rect.bottom + 15
        self._draw_env_section_header("ecf_configList编辑", left, area4_y, 'env_config_edit')

        self.env_define_item_map = []  # 记录每行的位置和所属模块，供事件点击使用

        # 收集已勾选且拥有宏定义的模块（保留模块内定义顺序）
        checked_modules_with_defines = []
        for mod in self.rules["modules"]:
            if not self.env_module_checks.get(mod["name"], False):
                continue
            mod_defines = [d for d in mod.get("defines", [])
                           if d["name"] not in self.env_excluded_defines]
            if mod_defines:
                checked_modules_with_defines.append((mod, mod_defines))

        # 尺寸常量
        sub_header_h = 24
        sub_padding = 4
        def_item_h = 48
        module_gap = 8

        # 第一遍：计算总高度
        total_defines_height = 0
        module_layouts = []  # [(mod, mod_defines, start_offset, block_h), ...]
        for mod, mod_defines in checked_modules_with_defines:
            block_h = sub_header_h + sub_padding + len(mod_defines) * def_item_h + 8
            module_layouts.append((mod, mod_defines, total_defines_height, block_h))
            total_defines_height += block_h + module_gap
        if total_defines_height > 0:
            total_defines_height -= module_gap  # 去掉最后一个间距

        has_any_define = len(checked_modules_with_defines) > 0

        def_list_y = area4_y + 27
        def_list_h = max(60, total_defines_height + 8) if has_any_define else 60
        def_list_rect = pygame.Rect(left, def_list_y, cw, def_list_h)
        self._env_def_list_rect = def_list_rect  # 保存供事件处理使用
        draw_rect(screen, def_list_rect, PANEL_COLOR)
        draw_rect_outline(screen, def_list_rect, SEPARATOR, width=1)

        screen.set_clip(def_list_rect.clip(env_clip))

        _def_tooltip = None  # 注释 tooltip

        if not has_any_define:
            hint = font_small.render("请先在区域2勾选拥有宏定义的模块", True, TEXT_DIM)
            hint_rect = hint.get_rect(center=def_list_rect.center)
            screen.blit(hint, hint_rect)
        else:
            current_y = def_list_rect.y + 4
            for mod, mod_defines, _offset, _block_h in module_layouts:
                # ---- 模块子区域标题 ----
                sub_rect = pygame.Rect(def_list_rect.x + 6, current_y,
                                       def_list_rect.width - 12, sub_header_h)
                draw_rect(screen, sub_rect, (55, 80, 125))
                # 模块名
                mod_name_surf = font_normal.render(mod["name"], True, WHITE)
                screen.blit(mod_name_surf, (sub_rect.x + 8, sub_rect.y + 3))
                # 宏定义数量标记
                count_label = f"({len(mod_defines)}个宏定义)"
                count_surf = font_small.render(count_label, True, TEXT_DIM)
                screen.blit(count_surf, (sub_rect.x + sub_rect.width - count_surf.get_width() - 10,
                                         sub_rect.y + 4))

                current_y += sub_header_h + sub_padding

                # ---- 该模块的宏定义列表 ----
                for i, d in enumerate(mod_defines):
                    item_y = current_y + i * def_item_h
                    if item_y + def_item_h < def_list_rect.y or item_y > def_list_rect.bottom:
                        continue
                    item_rect = pygame.Rect(def_list_rect.x + 8, item_y,
                                            def_list_rect.width - 16, def_item_h - 4)
                    is_hovered = item_rect.collidepoint(pygame.mouse.get_pos())
                    is_editing = (self.env_define_editing_index == i
                                  and self.env_define_editing_module_name == mod["name"])
                    if is_editing:
                        bg = ITEM_SELECTED
                    elif is_hovered:
                        bg = ITEM_HOVER
                    else:
                        bg = ITEM_BG
                    draw_rect(screen, item_rect, bg)
                    self.env_define_item_map.append((mod["name"], i, d, item_rect))

                    # 定义名（固定列宽，对齐后续内容）
                    name_col_w = 200
                    name_rect = pygame.Rect(item_rect.x + 8, item_rect.y + 4, name_col_w, 22)
                    draw_rect(screen, name_rect, (60, 100, 170))
                    name_surf = font_small.render(clip_text(d["name"], font_small, name_col_w - 10), True, WHITE)
                    screen.blit(name_surf, (name_rect.x + 6, name_rect.y + 3))

                    # 定义内容（可编辑 / 只读）
                    val_x = name_rect.right + 15
                    if is_editing:
                        self.env_define_edit_input.rect.x = val_x
                        self.env_define_edit_input.rect.y = item_rect.y + 5
                        self.env_define_edit_input.draw(screen)
                    else:
                        val_label = font_small.render("值:", True, TEXT_DIM)
                        screen.blit(val_label, (val_x, item_rect.y + 7))
                        override_val = self.env_define_overrides.get(d["name"], "")
                        has_override = bool(override_val) and override_val != d.get("value", "")
                        display_val = override_val if override_val else d.get("value", "")
                        val_surf = font_small.render(
                            clip_text(display_val, font_small, 200), True,
                            (255, 200, 80) if has_override else TEXT_COLOR)
                        screen.blit(val_surf, (val_x + 30, item_rect.y + 7))

                    # 注释
                    cmt = d.get("comment", "")
                    if cmt:
                        full_cmt = f"// {cmt}"
                        cmt_max_w = item_rect.right - 30 - (item_rect.x + 480) - 10  # 避开删除按钮
                        clipped_cmt = clip_text(full_cmt, font_small, cmt_max_w)
                        cmt_surf = font_small.render(clipped_cmt, True, TEXT_DIM)
                        screen.blit(cmt_surf, (item_rect.x + 480, item_rect.y + 7))
                        # 注释 tooltip
                        if is_hovered and clipped_cmt != full_cmt and _def_tooltip is None:
                            _def_tooltip = collect_text_tooltip(full_cmt, font_small, cmt_max_w,
                                                                 item_rect, def_list_rect)

                    # 临时移除按钮
                    rm_btn = pygame.Rect(item_rect.right - 30, item_rect.y + 10, 22, 22)
                    is_rm_hover = rm_btn.collidepoint(pygame.mouse.get_pos())
                    rm_col = BUTTON_DELETE_HOVER if is_rm_hover else BUTTON_DELETE
                    draw_rect(screen, rm_btn, rm_col)
                    x_mark = font_small.render("X", True, WHITE)
                    screen.blit(x_mark, x_mark.get_rect(center=rm_btn.center))

                current_y += len(mod_defines) * def_item_h + 8 + module_gap

        screen.set_clip(env_clip)

        # 绘制注释 tooltip
        if _def_tooltip is not None:
            tip_x, tip_y, tip_w, tip_h, lines = _def_tooltip
            tip_rect = pygame.Rect(tip_x, tip_y, tip_w, tip_h)
            draw_rect(screen, tip_rect, (35, 35, 40))
            draw_rect_outline(screen, tip_rect, BUTTON_COLOR, width=1)
            for si, line in enumerate(lines):
                s_surf = font_small.render(line, True, (200, 200, 200))
                screen.blit(s_surf, (tip_x + 10, tip_y + 8 + si * 20))

        # ---- 区域5：生成文件 ----
        area5_y = def_list_rect.bottom + 15
        self._draw_env_section_header("生成文件", left, area5_y, 'env_generate')
        area5_h = 90
        area5_rect = pygame.Rect(left, area5_y + 27, cw, area5_h)
        draw_rect(screen, area5_rect, PANEL_COLOR)
        draw_rect_outline(screen, area5_rect, SEPARATOR, width=1)

        # 三个复选框 + 生成按钮
        cb_labels = [
            ("env_gen_cmake", "CMakeLists.txt 环境文件生成"),
            ("env_gen_config", "ecf_configList.h 全局配置文件生成"),
            ("env_gen_userjson", "CFG_User.json 用户配置生成"),
        ]
        for idx, (attr, label) in enumerate(cb_labels):
            cb_x = area5_rect.x + 20
            cb_y = area5_rect.y + 8 + idx * 24
            cb_rect = pygame.Rect(cb_x, cb_y, 18, 18)
            checked = getattr(self, attr)
            if checked:
                draw_rect(screen, cb_rect, (80, 180, 80))
                cx, cy = cb_rect.centerx, cb_rect.centery
                pygame.draw.line(screen, WHITE, (cx - 3, cy), (cx - 1, cy + 2), 2)
                pygame.draw.line(screen, WHITE, (cx - 1, cy + 2), (cx + 3, cy - 3), 2)
            else:
                draw_rect_outline(screen, cb_rect, TEXT_DIM, border_radius=2, width=1)
            draw_text(screen, label, cb_x + 30, cb_y, font_small, TEXT_COLOR)

            # CMakeLists: 输出位置选择
            if attr == "env_gen_cmake":
                self.btn_env_cmake_outdir.rect.x = cb_x + 310
                self.btn_env_cmake_outdir.rect.y = cb_y - 3
                self.btn_env_cmake_outdir.draw(screen)
                if self.env_cmake_output_dir:
                    dir_disp = clip_text(self.env_cmake_output_dir, font_small, 200)
                    draw_text(screen, dir_disp,
                              self.btn_env_cmake_outdir.rect.right + 8, cb_y,
                              font_small, (100, 220, 100))

            # CFG_User.json 文件名自定义输入
            if attr == "env_gen_userjson":
                label_x = cb_x + 300
                draw_text(screen, "修改配置文件名:", label_x, cb_y, font_small, TEXT_DIM)
                inp_x = label_x + 130
                self.env_userjson_input.rect.x = inp_x
                self.env_userjson_input.rect.y = cb_y - 4
                self.env_userjson_input.rect.width = 140
                self.env_userjson_input.draw(screen)

        self.btn_env_generate.rect.y = area5_rect.y + area5_h - 38
        self.btn_env_generate.rect.x = area5_rect.x + area5_rect.width - 130
        self.btn_env_generate.draw(screen)

        # ---- 更新页面总高度 ----
        self._env_content_bottom = area5_rect.bottom + 20 + sy

        # ---- 绘制页面滚动条 ----
        content_h = self._env_content_bottom - self.TAB0_TOP  # 内容总高度（不变于滚动）
        visible_h = WIN_HEIGHT - self.TAB0_TOP - 28
        if content_h > visible_h:
            sb_x = WIN_WIDTH - 10
            sb_y = self.TAB0_TOP
            sb_h = visible_h
            thumb_h = max(30, visible_h * visible_h / content_h)
            max_sc = content_h - visible_h
            thumb_y = sb_y + (sb_h - thumb_h) * self.env_scroll_y / max_sc
            draw_rect(screen, (sb_x, sb_y, 8, sb_h), SCROLLBAR_BG)
            draw_rect(screen, (sb_x, thumb_y, 8, thumb_h), SCROLLBAR_THUMB)
            # 保存滚动条参数供事件处理
            self._env_sb_info = (sb_x, sb_y, sb_h, thumb_h, content_h, visible_h)
        else:
            self._env_sb_info = None

        screen.set_clip(None)  # 恢复无裁剪状态

    def _draw_env_section_header(self, title, x, y, help_key=None):
        """绘制区域标题栏，可选添加帮助按钮"""
        header_rect = pygame.Rect(x, y, self.CONTENT_WIDTH, 26)
        draw_rect(screen, header_rect, (55, 70, 100))
        title_surf = font_bold.render(title, True, WHITE)
        screen.blit(title_surf, (x + 10, y + 3))
        if help_key:
            self._draw_help_button(x + self.CONTENT_WIDTH - 30, y + 3, help_key)

    @staticmethod
    def _get_help_title(section_key):
        """将 help key 转换为弹窗标题"""
        titles = {
            'env_user_config': "用户配置读取",
            'env_module_enable': "CMakeList模块启用",
            'env_custom_paths': "CMakeList自定义",
            'env_config_edit': "ecf_configList编辑",
            'env_generate': "生成文件",
            'add_module': "添加模块",
            'edit_module': "编辑模块",
            'config_module_list': "模块列表",
            'config_define_edit': "编辑宏定义",
        }
        return titles.get(section_key, "帮助")

    def _draw_help_button(self, x, y, section_key):
        """在指定位置绘制问号帮助按钮，返回按钮矩形"""
        btn_rect = pygame.Rect(x, y, 20, 20)
        mx, my = pygame.mouse.get_pos()
        hovered = btn_rect.collidepoint(mx, my)
        bg = BUTTON_HOVER if hovered else BUTTON_COLOR
        draw_rect(screen, btn_rect, bg, border_radius=10)
        q_surf = font_small.render("?", True, WHITE)
        q_rect = q_surf.get_rect(center=btn_rect.center)
        screen.blit(q_surf, q_rect)
        self._help_btn_rects.append((btn_rect, section_key))

    # ============================================================
    # 环境文件生成 - 事件处理
    # ============================================================
    def handle_env_tab_events(self, event):
        """处理环境文件生成标签页的事件"""
        mx, my = pygame.mouse.get_pos()

        # 页面滚动
        if event.type == pygame.MOUSEWHEEL:
            total_h = self._env_content_bottom - self.TAB0_TOP
            visible_h = WIN_HEIGHT - self.TAB0_TOP - 28
            max_scroll = max(0, total_h - visible_h)
            self.env_scroll_y = max(0, min(max_scroll,
                                           self.env_scroll_y - event.y * 40))
            self.env_define_editing_index = -1  # 滚动时取消编辑
            self.env_define_editing_module_name = ""
            return True

        # 滚动条拖拽：点击滑块开始拖拽
        if (event.type == pygame.MOUSEBUTTONDOWN and event.button == 1
                and self._env_sb_info is not None):
            sb_x, sb_y, sb_h, thumb_h, content_h, visible_h = self._env_sb_info
            if self._sb_check_thumb((mx, my), sb_x, sb_y, sb_h,
                                     content_h, visible_h, self.env_scroll_y):
                self._sb_begin_drag('env', my, self.env_scroll_y, content_h, visible_h)
                return True

        # ---- 编辑模式：定义值输入 ----
        if self.env_define_editing_index >= 0 and self.env_define_editing_module_name:
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                self.env_define_editing_index = -1
                self.env_define_editing_module_name = ""
                return True
            if event.type == pygame.KEYDOWN and event.key == pygame.K_RETURN:
                # 确认编辑 — 从对应模块的 defines 中查找（使用与绘制一致的过滤列表）
                mod_name = self.env_define_editing_module_name
                for mod in self.rules["modules"]:
                    if mod["name"] == mod_name:
                        mod_defines = [d for d in mod.get("defines", [])
                                       if d["name"] not in self.env_excluded_defines]
                        if self.env_define_editing_index < len(mod_defines):
                            d = mod_defines[self.env_define_editing_index]
                            new_val = self.env_define_edit_input.text.strip()
                            if new_val:
                                self.env_define_overrides[d["name"]] = new_val
                            elif d["name"] in self.env_define_overrides:
                                del self.env_define_overrides[d["name"]]
                        break
                self.env_define_editing_index = -1
                self.env_define_editing_module_name = ""
                return True
            if event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING, pygame.KEYDOWN):
                self.env_define_edit_input.handle_event(event)
                return True
            if event.type == pygame.MOUSEBUTTONDOWN:
                # 点击其他地方取消编辑
                self.env_define_editing_index = -1
                self.env_define_editing_module_name = ""
            return True

        # ---- 文件名输入框处于激活状态时 ----
        if self.env_userjson_input.active:
            if event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING, pygame.KEYDOWN):
                self.env_userjson_input.handle_event(event)
                self.env_userjson_filename = self.env_userjson_input.text or "CFG_User"
                return True
            if event.type == pygame.MOUSEBUTTONDOWN:
                if not self.env_userjson_input.contains(event.pos):
                    self.env_userjson_input.active = False
                    # 继续往下处理点击

        # ---- 项目名称输入框处于激活状态时 ----
        if self.env_project_input.active:
            if event.type in (pygame.TEXTINPUT, pygame.TEXTEDITING, pygame.KEYDOWN):
                self.env_project_input.handle_event(event)
                self.env_project_name = self.env_project_input.text or "Project"
                return True
            if event.type == pygame.MOUSEBUTTONDOWN:
                if not self.env_project_input.contains(event.pos):
                    self.env_project_input.active = False
                    # 继续往下处理点击

        if event.type != pygame.MOUSEBUTTONDOWN or event.button != 1:
            return False

        # 转换屏幕坐标到内容坐标（加上滚动偏移）
        content_my = my + self.env_scroll_y

        # ---- 区域1：选择配置文件 ----
        if self.btn_env_load_config.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self._pending_user_config_load = True
            return True
        if self.btn_env_load_user_cfg.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self._pending_rule_load = True  # 延迟到 update() 处理
            return True
        if self.btn_env_load_cmake_tpl.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self._pending_tpl_load = "cmake"
            return True
        if self.btn_env_load_config_tpl.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self._pending_tpl_load = "config"
            return True

        # ---- 区域2：模块勾选 ----
        area2_y = self.TAB0_TOP + 100 + 15  # area1_y + area1_h(100) + gap
        mod_list_y = area2_y + 27
        mod_item_h = 34
        modules = self.rules["modules"]
        for i, mod in enumerate(modules):
            item_y = mod_list_y + 4 + i * mod_item_h
            item_rect = pygame.Rect(self.CONTENT_LEFT + 4, item_y, self.CONTENT_WIDTH - 8, mod_item_h - 4)
            cb_rect = pygame.Rect(item_rect.x + 8, item_rect.y + 7, 20, 20)
            if item_rect.collidepoint(mx, content_my):
                self.env_toggle_module(mod["name"])
                if mod["name"] in self.env_module_checks:
                    state = "勾选" if self.env_module_checks[mod["name"]] else "取消勾选"
                    self.set_status(f"模块 '{mod['name']}' {state}")
                return True

        # ---- 区域3：自定义路径 ----
        # 项目名称输入框（rect 使用屏幕坐标，直接用 mx, my 判断）
        if self.env_project_input.contains((mx, my)):
            self.env_project_input.active = True
            return True

        if self.btn_env_add_path.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                folder = select_folder()
                if folder and folder not in self.env_custom_paths:
                    self.env_custom_paths.append(folder)
                    self.set_status(f"已添加路径: {folder}")
            return True

        # 路径删除（需要计算 path_list 位置）
        mod_list_h = max(60, len(modules) * mod_item_h + 8)
        area3_y = mod_list_y + mod_list_h + 15
        path_list_y = area3_y + 101
        for i, p in enumerate(self.env_custom_paths):
            item_y = path_list_y + 4 + i * 30
            del_r = pygame.Rect(self.CONTENT_LEFT + self.CONTENT_WIDTH - 32, item_y + 2, 22, 22)
            if del_r.collidepoint(mx, content_my):
                del self.env_custom_paths[i]
                self.set_status(f"已移除路径: {p}")
                return True

        # ---- 区域4：宏定义编辑 ----
        # 使用绘制时填充的 env_define_item_map 匹配点击（item_rect 已是屏幕空间坐标）
        for mod_name, def_idx, d, item_rect in self.env_define_item_map:
            if item_rect.collidepoint(mx, my):
                # 临时移除按钮
                rm_btn = pygame.Rect(item_rect.right - 30, item_rect.y + 10, 22, 22)
                if rm_btn.collidepoint(mx, my):
                    dname = d["name"]
                    if dname in self.env_excluded_defines:
                        self.env_excluded_defines.discard(dname)
                        self.set_status(f"已恢复 '{dname}'")
                    else:
                        self.env_excluded_defines.add(dname)
                        self.set_status(f"已临时移除 '{dname}'")
                    return True
                # 进入编辑模式
                self.env_define_editing_index = def_idx
                self.env_define_editing_module_name = mod_name
                override_val = self.env_define_overrides.get(d["name"], d.get("value", ""))
                self.env_define_edit_input.set_text(override_val)
                self.env_define_edit_input.active = True
                pygame.key.set_text_input_rect(self.env_define_edit_input.rect)
                self.set_status(f"编辑宏定义 '{d['name']}' (回车确认, ESC取消)")
                return True

        # ---- 区域5：生成文件 ----
        # def_list_rect 已在绘制时保存为屏幕空间坐标，直接使用
        def_list_rect = self._env_def_list_rect
        area5_screen_y = def_list_rect.bottom + 15  # 屏幕空间
        area5_rect = pygame.Rect(self.CONTENT_LEFT, area5_screen_y + 27, self.CONTENT_WIDTH, 90)

        # 复选框点击 + 文件名输入
        cb_labels = [
            ("env_gen_cmake", "CMakeLists.txt"), ("env_gen_config", "ecf_configList.h"),
            ("env_gen_userjson", "CFG_User.json")
        ]
        for idx, (attr, _lbl) in enumerate(cb_labels):
            cb_x = area5_rect.x + 20
            cb_y = area5_rect.y + 8 + idx * 24
            cb_rect = pygame.Rect(cb_x, cb_y, 18, 18)
            if cb_rect.collidepoint(mx, my):
                setattr(self, attr, not getattr(self, attr))
                return True
            # CMakeLists 输出位置按钮
            if attr == "env_gen_cmake":
                outdir_btn_rect = pygame.Rect(cb_x + 310, cb_y - 3, 130, 26)
                if outdir_btn_rect.collidepoint(mx, my):
                    folder = select_folder()
                    if folder:
                        self.env_cmake_output_dir = folder.replace("\\", "/")
                        self.set_status(f"CMake输出: {self.env_cmake_output_dir}")
                    return True
            # 文件名输入框点击
            if attr == "env_gen_userjson":
                inp_rect = pygame.Rect(cb_x + 430, cb_y - 4, 140, 26)
                if inp_rect.collidepoint(mx, my):
                    self.env_userjson_input.active = True
                    self.env_userjson_input.set_text(self.env_userjson_filename)
                    pygame.key.set_text_input_rect(inp_rect)
                    return True

        # 生成按钮（坐标由 draw_env_tab 设置，已为屏幕空间）
        if self.btn_env_generate.handle_event(event):
            if event.type == pygame.MOUSEBUTTONDOWN:
                self.env_generate_files()
            return True

        return False

    def draw_confirm_dialog(self):
        """绘制确认删除对话框"""
        if not self.confirm_dialog_active:
            return

        # 半透明遮罩
        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        # 对话框
        dialog_w, dialog_h = 350, 150
        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect, BUTTON_COLOR, width=2)

        # 标题
        title = font_bold.render("确认删除", True, WHITE)
        title_rect = title.get_rect(center=(WIN_WIDTH // 2, dialog_y + 30))
        screen.blit(title, title_rect)

        # 内容
        msg = font_small.render(f"确定要删除模块 \"{self.confirm_dialog_module_name}\" 吗？", True, TEXT_COLOR)
        msg_rect = msg.get_rect(center=(WIN_WIDTH // 2, dialog_y + 65))
        screen.blit(msg, msg_rect)

        # 按钮
        confirm_btn = Button(WIN_WIDTH // 2 - 120, dialog_y + 95, 100, 34, "确认删除",
                             color=BUTTON_DELETE, font_obj=font_small)
        cancel_btn = Button(WIN_WIDTH // 2 + 20, dialog_y + 95, 100, 34, "取消",
                            color=(100, 100, 105), font_obj=font_small)

        # 手动检测hover
        mx, my = pygame.mouse.get_pos()
        confirm_btn.hovered = confirm_btn.rect.collidepoint(mx, my)
        cancel_btn.hovered = cancel_btn.rect.collidepoint(mx, my)

        confirm_btn.draw(screen)
        cancel_btn.draw(screen)

    def draw_batch_define_dialog(self):
        """绘制批量添加宏定义弹窗"""
        if not self.batch_dialog_active:
            return

        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        dialog_w, dialog_h = 640, 440
        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect, BUTTON_COLOR, width=2)

        # 标题
        title = font_bold.render("批量添加宏定义", True, WHITE)
        title_rect = title.get_rect(center=(WIN_WIDTH // 2, dialog_y + 25))
        screen.blit(title, title_rect)

        hint = font_small.render("输入 C 语言 #define 宏，自动提取并添加到当前模块", True, TEXT_DIM)
        hint_rect = hint.get_rect(center=(WIN_WIDTH // 2, dialog_y + 48))
        screen.blit(hint, hint_rect)

        # 文本编辑区
        self.batch_text_input.rect.x = dialog_x + 20
        self.batch_text_input.rect.y = dialog_y + 60
        self.batch_text_input.rect.width = dialog_w - 40
        self.batch_text_input.rect.height = 300
        self.batch_text_input.draw(screen)

        # 按钮
        mx, my = pygame.mouse.get_pos()
        self.btn_batch_confirm.rect.x = dialog_x + dialog_w // 2 - 110
        self.btn_batch_confirm.rect.y = dialog_y + dialog_h - 45
        self.btn_batch_cancel.rect.x = dialog_x + dialog_w // 2 + 10
        self.btn_batch_cancel.rect.y = dialog_y + dialog_h - 45

        self.btn_batch_confirm.hovered = self.btn_batch_confirm.rect.collidepoint(mx, my)
        self.btn_batch_cancel.hovered = self.btn_batch_cancel.rect.collidepoint(mx, my)
        self.btn_batch_confirm.draw(screen)
        self.btn_batch_cancel.draw(screen)

    def draw_gen_result_dialog(self):
        """绘制生成结果弹窗"""
        if not self.gen_result_active:
            return

        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        # 计算文本尺寸
        lines = self.gen_result_message.split('\n')
        max_line_w = max((font_small.render(l, True, WHITE).get_width() for l in lines), default=200)
        line_h = 22
        dialog_w = min(max_line_w + 80, WIN_WIDTH - 80)
        content_h = len(lines) * line_h
        # 标题区 + 内容区 + 按钮区 + 边距
        dialog_h = max(180, 60 + content_h + 70)

        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect,
                         BUTTON_DELETE if self.gen_result_is_error else BUTTON_GREEN, width=2)

        # 标题（纯文字，避免 emoji 字体缺失）
        title_text = "[失败] 生成失败" if self.gen_result_is_error else "[完成] 生成完成"
        title_color = (255, 100, 100) if self.gen_result_is_error else (100, 220, 100)
        title = font_bold.render(title_text, True, title_color)
        title_rect = title.get_rect(center=(WIN_WIDTH // 2, dialog_y + 30))
        screen.blit(title, title_rect)

        # 分隔线
        sep_y = dialog_y + 52
        pygame.draw.line(screen, SEPARATOR, (dialog_x + 20, sep_y), (dialog_x + dialog_w - 20, sep_y), 1)

        # 内容
        content_top = sep_y + 15
        for li, line in enumerate(lines):
            ls = font_small.render(line, True, TEXT_COLOR)
            screen.blit(ls, (dialog_x + 30, content_top + li * line_h))

        # 确定按钮（放在内容下方）
        mx, my = pygame.mouse.get_pos()
        btn_w, btn_h = 100, 34
        btn_x = WIN_WIDTH // 2 - btn_w // 2
        btn_y = dialog_y + dialog_h - btn_h - 18
        ok_btn = Button(btn_x, btn_y, btn_w, btn_h, "确定",
                        color=BUTTON_GREEN, font_obj=font_small)
        ok_btn.hovered = ok_btn.rect.collidepoint(mx, my)
        ok_btn.draw(screen)

        # 存储按钮位置供事件处理
        self._gen_ok_btn_rect = ok_btn.rect

    def _draw_help_popup(self):
        """绘制帮助弹窗"""
        if not self.help_active:
            return

        overlay = pygame.Surface((WIN_WIDTH, WIN_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 160))
        screen.blit(overlay, (0, 0))

        # 文本折行
        max_w = 500
        lines = []
        for paragraph in self.help_text.split('\n'):
            wrapped = wrap_text(paragraph, font_small, max_w)
            if wrapped:
                lines.extend(wrapped)
            else:
                lines.append("")
        line_h = 22
        content_h = max(len(lines) * line_h, 40)
        dialog_w = max_w + 60
        dialog_h = 60 + content_h + 70

        dialog_x = WIN_WIDTH // 2 - dialog_w // 2
        dialog_y = WIN_HEIGHT // 2 - dialog_h // 2
        dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_w, dialog_h)
        draw_rect(screen, dialog_rect, PANEL_COLOR)
        draw_rect_outline(screen, dialog_rect, BUTTON_COLOR, width=2)

        # 标题
        title_surf = font_bold.render(self.help_title, True, WHITE)
        title_rect = title_surf.get_rect(center=(WIN_WIDTH // 2, dialog_y + 28))
        screen.blit(title_surf, title_rect)

        # 分隔线
        sep_y = dialog_y + 50
        pygame.draw.line(screen, SEPARATOR, (dialog_x + 20, sep_y),
                         (dialog_x + dialog_w - 20, sep_y), 1)

        # 内容
        for li, line in enumerate(lines):
            ls = font_small.render(line, True, TEXT_COLOR)
            screen.blit(ls, (dialog_x + 30, sep_y + 15 + li * line_h))

        # 确定按钮
        mx, my = pygame.mouse.get_pos()
        btn_w, btn_h = 100, 34
        btn_x = WIN_WIDTH // 2 - btn_w // 2
        btn_y = dialog_y + dialog_h - btn_h - 18
        ok_btn = Button(btn_x, btn_y, btn_w, btn_h, "确定",
                        color=BUTTON_COLOR, font_obj=font_small)
        ok_btn.hovered = ok_btn.rect.collidepoint(mx, my)
        ok_btn.draw(screen)
        self._help_ok_btn_rect = ok_btn.rect

    def draw_status_bar(self):
        """绘制状态栏"""
        if self.status_message:
            # 底部状态栏
            status_rect = pygame.Rect(0, WIN_HEIGHT - 28, WIN_WIDTH, 28)
            is_error = "错误" in self.status_message
            bg = (180, 50, 50) if is_error else (50, 130, 50)
            draw_rect(screen, status_rect, bg, border_radius=0)

            status_surf = font_small.render(self.status_message, True, WHITE)
            status_rect2 = status_surf.get_rect(center=(WIN_WIDTH // 2, WIN_HEIGHT - 14))
            screen.blit(status_surf, status_rect2)

    def draw_current_tab(self):
        """绘制当前标签页内容"""
        if self.current_tab == 0:
            self.draw_env_tab()
        elif self.current_tab == 1:
            self.draw_add_module_area()
            self.draw_edit_module_area()
        elif self.current_tab == 2:
            self.draw_config_tab()

    def render(self):
        screen.fill(BG_COLOR)
        self._help_btn_rects = []  # 每帧重置帮助按钮区域
        self.draw_tab_bar()
        self.draw_current_tab()
        self.draw_confirm_dialog()
        self._draw_submodule_dialog()
        self.draw_config_delete_dialog()
        self.draw_batch_define_dialog()
        self.draw_gen_result_dialog()
        self._draw_help_popup()
        self.draw_status_bar()
        pygame.display.flip()

    # ============================================================
    # 主循环
    # ============================================================
    def run(self):
        while self.running:
            self.handle_events()
            self.update()
            self.render()

        pygame.quit()
        sys.exit()


# ============================================================
# 入口
# ============================================================
if __name__ == "__main__":
    app = App()
    app.run()
