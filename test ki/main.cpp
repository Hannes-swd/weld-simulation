// ================================================================
//  LEBENSSIMULATION v3.3 - ÜBERLEBENS-EDITION
//  Optimiert für langfristiges Überleben der Bevölkerung
//  Prio 1: Häuserbau, Prio 2: Wärmeschutz, Prio 3: Gemeinschaft
// ================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <deque>
#include <stdarg.h>
#include <set>

// ================================================================
//  FARBEN
// ================================================================
static const WORD C_DEFAULT = 0x07;
static const WORD C_BORDER = 0x02;
static const WORD C_HEADER = 0x20;
static const WORD C_OK = 0x0A;
static const WORD C_WARN = 0x0E;
static const WORD C_DANGER = 0x0C;
static const WORD C_SEL = 0x30;
static const WORD C_DIM = 0x08;
static const WORD C_ACCENT = 0x0B;
static const WORD C_TITLE = 0x60;
static const WORD C_BTN = 0x70;
static const WORD C_BTNSEL = 0x30;
static const WORD C_LOG = 0x0E;
static const WORD C_CHART = 0x0D;
static const WORD C_TRADE = 0x09;
static const WORD C_GRASS = 0x02;
static const WORD C_DIRT = 0x06;
static const WORD C_STONE = 0x07;
static const WORD C_WATER = 0x01;
static const WORD C_RIVER = 0x09;
static const WORD C_LAKE = 0x03;
static const WORD C_TREE = 0x0A;
static const WORD C_ORE = 0x0E;
static const WORD C_HOUSE = 0x0D;
static const WORD C_MINE = 0x08;
static const WORD C_PERSON = 0x0F;
static const WORD C_ROAD = 0x06;
static const WORD C_NIGHT = 0x08;
static const WORD C_FRIEDHOF = 0x05;
static const WORD C_KIRCHE = 0x06;
static const WORD C_TURM = 0x07;

// ================================================================
//  CONSOLE RENDERER
// ================================================================
struct ConRenderer {
    HANDLE hOut = INVALID_HANDLE_VALUE, hIn = INVALID_HANDLE_VALUE, hBuf = INVALID_HANDLE_VALUE;
    int W = 0, H = 0;
    std::vector<CHAR_INFO> back, front;

    void init() {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        hIn = GetStdHandle(STD_INPUT_HANDLE);
        hBuf = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(hBuf);
        SetConsoleOutputCP(437); SetConsoleCP(437);
        CONSOLE_CURSOR_INFO ci; ci.dwSize = 1; ci.bVisible = FALSE;
        SetConsoleCursorInfo(hBuf, &ci);
        DWORD mode = 0; GetConsoleMode(hIn, &mode);
        mode &= ~ENABLE_QUICK_EDIT_MODE; mode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(hIn, mode);
        refreshSize();
    }
    void refreshSize() {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hBuf, &info);
        int nw = info.srWindow.Right - info.srWindow.Left + 1;
        int nh = info.srWindow.Bottom - info.srWindow.Top + 1;
        if (nw != W || nh != H) {
            W = nw; H = nh;
            back.resize(W * H); front.resize(W * H);
            for (auto& c : front) { c.Char.AsciiChar = 0; c.Attributes = 0; }
        }
    }
    void clear() {
        CHAR_INFO b; b.Char.AsciiChar = ' '; b.Attributes = C_DEFAULT;
        std::fill(back.begin(), back.end(), b);
    }
    inline void put(int x, int y, char c, WORD a) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        back[y * W + x].Char.AsciiChar = c; back[y * W + x].Attributes = a;
    }
    void print(int x, int y, const char* s, WORD a) {
        for (int i = 0; s[i] && x + i < W; i++) put(x + i, y, s[i], a);
    }
    void printf2(int x, int y, WORD a, const char* fmt, ...) {
        char buf[512]; va_list v; va_start(v, fmt);
        _vsnprintf_s(buf, 511, _TRUNCATE, fmt, v); va_end(v);
        print(x, y, buf, a);
    }
    void fillLine(int y2, char c, WORD a) { for (int x = 0; x < W; x++) put(x, y2, c, a); }
    void box(int x, int y, int w, int h, WORD a) {
        put(x, y, '\xC9', a); put(x + w - 1, y, '\xBB', a);
        put(x, y + h - 1, '\xC8', a); put(x + w - 1, y + h - 1, '\xBC', a);
        for (int i = 1; i < w - 1; i++) { put(x + i, y, '\xCD', a); put(x + i, y + h - 1, '\xCD', a); }
        for (int i = 1; i < h - 1; i++) { put(x, y + i, '\xBA', a); put(x + w - 1, y + i, '\xBA', a); }
    }
    void boxTitle(int x, int y, int w, int h, const char* t, bool active) {
        WORD ba = active ? C_ACCENT : C_BORDER, ta = active ? C_TITLE : C_HEADER;
        box(x, y, w, h, ba);
        if (t && t[0]) {
            char buf[64]; _snprintf_s(buf, 63, _TRUNCATE, " %s ", t);
            int tl = (int)strlen(buf), tx = x + (w - tl) / 2; if (tx < x + 1) tx = x + 1;
            print(tx, y, buf, ta);
        }
    }
    void flip() {
        COORD sz = { (SHORT)W, (SHORT)H }, or2 = { 0, 0 };
        SMALL_RECT reg = { 0, 0, (SHORT)(W - 1), (SHORT)(H - 1) };
        WriteConsoleOutputA(hBuf, back.data(), sz, or2, &reg);
        std::swap(back, front);
    }
    int getKey() {
        DWORD cnt = 0; GetNumberOfConsoleInputEvents(hIn, &cnt);
        if (!cnt) return -1;
        INPUT_RECORD ir; DWORD rd = 0;
        ReadConsoleInputA(hIn, &ir, 1, &rd);
        if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) { refreshSize(); return -1; }
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) return -1;
        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        char c = ir.Event.KeyEvent.uChar.AsciiChar;
        if (vk == VK_UP)    return 1000;
        if (vk == VK_DOWN)  return 1001;
        if (vk == VK_LEFT)  return 1002;
        if (vk == VK_RIGHT) return 1003;
        if (vk == VK_PRIOR) return 1004;
        if (vk == VK_NEXT)  return 1005;
        if (vk == VK_F1)    return 1010;
        if (vk == VK_F2)    return 1011;
        if (vk == VK_F3)    return 1012;
        if (vk == VK_RETURN)return '\r';
        if (vk == VK_TAB) { bool sh = (GetKeyState(VK_SHIFT) & 0x8000) != 0; return sh ? 1020 : '\t'; }
        if (c > 0) return (unsigned char)c;
        return -1;
    }
    void restore() {
        SetConsoleActiveScreenBuffer(hOut);
        CONSOLE_CURSOR_INFO ci; ci.dwSize = 10; ci.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &ci);
    }
} con;

// ================================================================
//  ZUFALL
// ================================================================
static std::mt19937 RNG(std::random_device{}());
static float rf() { return std::uniform_real_distribution<float>(0, 1)(RNG); }
static int   ri(int lo, int hi) {
    if (lo >= hi) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(RNG);
}
static float fc(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float noise2(int x, int y, int seed = 42) {
    unsigned int n = (unsigned)(x)+(unsigned)(y) * 57u + (unsigned)(seed) * 131u;
    n = (n << 13u) ^ n;
    unsigned int r = (n * (n * n * 15731u + 789221u) + 1376312589u);
    return 1.f - (float)(r & 0x7fffffffu) / 1073741824.f;
}
static float smoothNoise(float x, float y, int seed = 42) {
    int   ix = (int)floor(x), iy = (int)floor(y);
    float fx = x - (float)ix, fy = y - (float)iy;
    float v00 = noise2(ix, iy, seed);
    float v10 = noise2(ix + 1, iy, seed);
    float v01 = noise2(ix, iy + 1, seed);
    float v11 = noise2(ix + 1, iy + 1, seed);
    float tx = fx * fx * (3.f - 2.f * fx), ty = fy * fy * (3.f - 2.f * fy);
    return v00 + (v10 - v00) * tx + (v01 - v00) * ty + (v00 - v10 - v01 + v11) * tx * ty;
}
static float fractalNoise(float x, float y, int octaves = 4, int seed = 42) {
    float v = 0, amp = 1, freq = 1, maxV = 0;
    for (int i = 0; i < octaves; i++) {
        v += smoothNoise(x * freq, y * freq, seed + i * 17) * amp;
        maxV += amp; amp *= 0.5f; freq *= 2.f;
    }
    return maxV > 0 ? v / maxV : 0.f;
}

// ================================================================
//  WELT-TILES
// ================================================================
enum class Tile : uint8_t {
    Wasser = 0, Flachland, Wald, Huegel, Berg, Wueste,
    Fluss, See,
    Haus, Mine, Markt, Lager, Farm, Werkstatt,
    Strasse, Bruecke,
    Friedhof, Kirche, Wachturm
};

static bool istWasserquelle(Tile t) {
    return t == Tile::Wasser || t == Tile::Fluss || t == Tile::See;
}

static char tileChar(Tile t) {
    switch (t) {
    case Tile::Wasser:    return '~';
    case Tile::Fluss:     return '\xF7';
    case Tile::See:       return '\xF0';
    case Tile::Flachland: return '.';
    case Tile::Wald:      return 'T';
    case Tile::Huegel:    return 'n';
    case Tile::Berg:      return '^';
    case Tile::Wueste:    return ':';
    case Tile::Haus:      return 'H';
    case Tile::Mine:      return 'M';
    case Tile::Markt:     return '$';
    case Tile::Lager:     return 'L';
    case Tile::Farm:      return 'F';
    case Tile::Werkstatt: return 'W';
    case Tile::Strasse:   return '\xC5';
    case Tile::Bruecke:   return '=';
    case Tile::Friedhof:  return '\xEB';
    case Tile::Kirche:    return '+';
    case Tile::Wachturm:  return 'I';
    default:              return '?';
    }
}
static WORD tileColor(Tile t) {
    switch (t) {
    case Tile::Wasser:    return C_WATER;
    case Tile::Fluss:     return C_RIVER;
    case Tile::See:       return C_LAKE;
    case Tile::Flachland: return C_GRASS;
    case Tile::Wald:      return C_TREE;
    case Tile::Huegel:    return C_DIRT;
    case Tile::Berg:      return C_STONE;
    case Tile::Wueste:    return C_WARN;
    case Tile::Haus:      return C_HOUSE;
    case Tile::Mine:      return C_MINE;
    case Tile::Markt:     return C_ACCENT;
    case Tile::Lager:     return C_WARN;
    case Tile::Farm:      return C_OK;
    case Tile::Werkstatt: return C_TRADE;
    case Tile::Strasse:   return C_ROAD;
    case Tile::Bruecke:   return C_DIRT;
    case Tile::Friedhof:  return C_FRIEDHOF;
    case Tile::Kirche:    return C_KIRCHE;
    case Tile::Wachturm:  return C_TURM;
    default:              return C_DEFAULT;
    }
}
static bool isBebaubar(Tile t) {
    return t == Tile::Flachland || t == Tile::Wald || t == Tile::Huegel;
}
static bool isPassierbar(Tile t) {
    return t != Tile::Wasser && t != Tile::Berg && t != Tile::Fluss && t != Tile::See;
}

struct TileRessourcen {
    int holz = 0, stein = 0, erz = 0, gold = 0, kohle = 0, nahrung = 0;
};
static TileRessourcen tileRess(Tile t) {
    TileRessourcen r;
    switch (t) {
    case Tile::Wald:      r.holz = 3; r.nahrung = 1; break;
    case Tile::Huegel:    r.stein = 2; r.erz = 1; break;
    case Tile::Berg:      r.stein = 4; r.erz = 3; r.kohle = 1; break;
    case Tile::Flachland: r.nahrung = 2; break;
    case Tile::Wueste:    r.stein = 1; r.gold = 1; break;
    default: break;
    }
    return r;
}

// ================================================================
//  CHUNK-SYSTEM
// ================================================================
static const int CHUNK_SZ = 16;

struct ChunkKey {
    int cx, cy;
    bool operator==(const ChunkKey& o) const { return cx == o.cx && cy == o.cy; }
};
struct ChunkHash {
    size_t operator()(const ChunkKey& k) const {
        return std::hash<long long>()(((long long)k.cx << 32) | ((unsigned)k.cy));
    }
};

struct ChunkData {
    Tile tiles[CHUNK_SZ][CHUNK_SZ];
    int  ressLeft[CHUNK_SZ][CHUNK_SZ];
    bool bebaut[CHUNK_SZ][CHUNK_SZ];
};

class Welt {
public:
    std::unordered_map<ChunkKey, ChunkData, ChunkHash> chunks;
    int worldSeed;

    Welt(int seed = 12345) : worldSeed(seed) { chunks.reserve(512); }

    ChunkData& getChunk(int cx, int cy) {
        ChunkKey key{ cx, cy };
        auto it = chunks.find(key);
        if (it != chunks.end()) return it->second;
        if (chunks.size() + 1 > (size_t)(chunks.bucket_count() * chunks.max_load_factor()))
            chunks.reserve(chunks.size() * 2 + 64);
        ChunkData tmp;
        generateChunk(tmp, cx, cy);
        auto res = chunks.emplace(key, tmp);
        return res.first->second;
    }

    void generateChunk(ChunkData& c, int cx, int cy) {
        memset(&c, 0, sizeof(c));
        for (int ly = 0; ly < CHUNK_SZ; ly++) {
            for (int lx = 0; lx < CHUNK_SZ; lx++) {
                int   wx = cx * CHUNK_SZ + lx;
                int   wy = cy * CHUNK_SZ + ly;
                float h = fractalNoise(wx * 0.05f, wy * 0.05f, 4, worldSeed);
                float w2 = fractalNoise(wx * 0.03f, wy * 0.03f, 2, worldSeed + 777);

                float rn1 = fractalNoise(wx * 0.04f, wy * 0.04f, 3, worldSeed + 1111);
                float rn2 = fractalNoise(wx * 0.04f, wy * 0.04f, 3, worldSeed + 2222);
                bool  istFluss = (std::abs(rn1) < 0.045f) && (h > -0.35f) && (w2 > -0.15f);
                bool  istFluss2 = (std::abs(rn2) < 0.035f) && (h > -0.35f) && (w2 > -0.1f);

                float sn = fractalNoise(wx * 0.07f, wy * 0.07f, 2, worldSeed + 3333);
                bool  istSee = (sn > 0.38f) && (h < -0.05f) && (w2 > -0.05f)
                    && !istFluss && !istFluss2;

                Tile t;
                if (w2 < -0.2f) t = Tile::Wasser;
                else if (h < -0.3f) t = Tile::Flachland;
                else if (h < 0.1f) t = Tile::Wald;
                else if (h < 0.4f) t = Tile::Huegel;
                else if (h < 0.7f) t = Tile::Berg;
                else                 t = Tile::Wueste;

                float dist = (float)sqrt((double)(wx * wx + wy * wy));
                if (dist < 15 && t == Tile::Wasser) t = Tile::Flachland;
                if (dist < 8)                       t = Tile::Flachland;

                if (dist >= 10 && t != Tile::Wasser && t != Tile::Berg) {
                    if (istSee)              t = Tile::See;
                    if (istFluss || istFluss2) t = Tile::Fluss;
                }

                c.tiles[ly][lx] = t;
                c.bebaut[ly][lx] = false;

                if (istWasserquelle(t)) {
                    c.ressLeft[ly][lx] = 9999;
                }
                else {
                    TileRessourcen r = tileRess(t);
                    c.ressLeft[ly][lx] = r.holz + r.stein + r.erz * 2 + r.kohle * 3 + r.gold * 5 + r.nahrung;
                    if (c.ressLeft[ly][lx] < 1) c.ressLeft[ly][lx] = 1;
                }
            }
        }
    }

    static void worldToChunk(int wx, int wy, int& cx, int& cy, int& lx, int& ly) {
        cx = (wx >= 0) ? wx / CHUNK_SZ : ((wx + 1) / CHUNK_SZ - 1);
        cy = (wy >= 0) ? wy / CHUNK_SZ : ((wy + 1) / CHUNK_SZ - 1);
        lx = wx - cx * CHUNK_SZ;
        ly = wy - cy * CHUNK_SZ;
        lx = ((lx % CHUNK_SZ) + CHUNK_SZ) % CHUNK_SZ;
        ly = ((ly % CHUNK_SZ) + CHUNK_SZ) % CHUNK_SZ;
        if (lx < 0) lx = 0; if (lx >= CHUNK_SZ) lx = CHUNK_SZ - 1;
        if (ly < 0) ly = 0; if (ly >= CHUNK_SZ) ly = CHUNK_SZ - 1;
    }

    Tile getTile(int wx, int wy) {
        int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly);
        return getChunk(cx, cy).tiles[ly][lx];
    }

    void setTile(int wx, int wy, Tile t) {
        int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly);
        ChunkData& c = getChunk(cx, cy);
        c.tiles[ly][lx] = t;
        c.bebaut[ly][lx] = true;
    }

    int getRess(int wx, int wy) {
        int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly);
        return getChunk(cx, cy).ressLeft[ly][lx];
    }

    void setRess(int wx, int wy, int val) {
        int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly);
        getChunk(cx, cy).ressLeft[ly][lx] = val;
    }

    int dekrementRess(int wx, int wy) {
        int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly);
        ChunkData& c = getChunk(cx, cy);
        if (c.ressLeft[ly][lx] > 0) c.ressLeft[ly][lx]--;
        return c.ressLeft[ly][lx];
    }

    bool findNearest(int ox, int oy, Tile ziel, int maxR, int& fx, int& fy) {
        for (int r = 1; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (abs(dx) != r && abs(dy) != r) continue;
                    int wx = ox + dx, wy = oy + dy;
                    if (getTile(wx, wy) == ziel && getRess(wx, wy) > 0) {
                        fx = wx; fy = wy; return true;
                    }
                }
            }
        }
        return false;
    }

    bool findNearestWasser(int ox, int oy, int maxR, int& fx, int& fy) {
        for (int r = 1; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (abs(dx) != r && abs(dy) != r) continue;
                    int wx = ox + dx, wy = oy + dy;
                    if (istWasserquelle(getTile(wx, wy))) {
                        fx = wx; fy = wy; return true;
                    }
                }
            }
        }
        return false;
    }

    bool findFreiePlatz(int ox, int oy, int maxR, int& fx, int& fy) {
        for (int r = 0; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (r > 0 && abs(dx) != r && abs(dy) != r) continue;
                    int wx = ox + dx, wy = oy + dy;
                    Tile t = getTile(wx, wy);
                    if (isBebaubar(t)) {
                        int cx2, cy2, lx2, ly2; worldToChunk(wx, wy, cx2, cy2, lx2, ly2);
                        if (!getChunk(cx2, cy2).bebaut[ly2][lx2]) { fx = wx; fy = wy; return true; }
                    }
                }
            }
        }
        return false;
    }
};

// ================================================================
//  INVENTAR
// ================================================================
enum class IT {
    Holz, Stein, Erz, Wasser, Pflanze, Fleisch, Fisch,
    Brett, Werkzeug, Stoff, NahrIT, Moebel, Waffe, Medizin, HausIT,
    Bier, Pelz, Kohle, Keramik, Brot, Gold, COUNT
};
static const int IT_COUNT = (int)IT::COUNT;

static const char* itStr(IT t) {
    static const char* n[] = {
        "Holz","Stein","Erz","Wasser","Pflanze","Fleisch","Fisch",
        "Brett","Werkzeug","Stoff","Nahrung","Moebel","Waffe","Medizin","Haus",
        "Bier","Pelz","Kohle","Keramik","Brot","Gold","?"
    };
    int i = (int)t; return (i >= 0 && i < IT_COUNT) ? n[i] : "?";
}
static float itBasiswert(IT t) {
    static const float bw[] = {
        3,2,7,1,2,5,4,
        9,15,11,7,20,24,30,60,
        5,11,5,8,6,25,0
    };
    int i = (int)t; return (i >= 0 && i < IT_COUNT) ? bw[i] : 1;
}

struct Inventar {
    int items[IT_COUNT] = {};
    void  add(IT t, int n = 1) { int i = (int)t; if (i >= 0 && i < IT_COUNT) items[i] += n; }
    bool  hat(IT t, int n = 1) const { int i = (int)t; return i >= 0 && i < IT_COUNT && items[i] >= n; }
    bool  nimm(IT t, int n = 1) { if (!hat(t, n)) return false; items[(int)t] -= n; return true; }
    int   anz(IT t) const { int i = (int)t; return (i >= 0 && i < IT_COUNT) ? items[i] : 0; }
    int   gesamt() const { int s = 0; for (int i = 0; i < IT_COUNT; i++) s += items[i]; return s; }
    float wert() const {
        float w = 0;
        for (int i = 0; i < IT_COUNT; i++) w += items[i] * itBasiswert((IT)i);
        return w;
    }
};

// ================================================================
//  SKILL-SYSTEM
// ================================================================
enum class SK {
    Holzfaellen, Bergbau, Jagd, Fischen, Kochen,
    Schmieden, Tischlern, Bauen,
    Handel, Fuehren, Lehren,
    Nahkampf, Verteidigung,
    Heilen, Forschen,
    Brauen, Gerben, Bergbaumeister,
    COUNT
};
static const int SK_COUNT = (int)SK::COUNT;
static const char* skStr(SK s) {
    static const char* n[] = {
        "Holzfaellen","Bergbau","Jagd","Fischen","Kochen",
        "Schmieden","Tischlern","Bauen",
        "Handel","Fuehren","Lehren",
        "Nahkampf","Verteidigung",
        "Heilen","Forschen",
        "Brauen","Gerben","Bergbaumeister",
        "?"
    };
    int i = (int)s; return (i >= 0 && i < SK_COUNT) ? n[i] : "?";
}

// ================================================================
//  BERUF
// ================================================================
enum class JB {
    Keiner, Holzfaeller, Bergmann, Jaeger, Fischer, Koch,
    Schmied, Tischler, Bauer, Haendler, Lehrer, Wache, Arzt,
    Brauer, Gerber, Forscher, Bergbaumeister
};
static const char* jbStr(JB j) {
    static const char* n[] = {
        "Keiner","Holzfaeller","Bergmann","Jaeger","Fischer","Koch",
        "Schmied","Tischler","Bauer","Haendler","Lehrer","Wache","Arzt",
        "Brauer","Gerber","Forscher","Bergbaumeister"
    };
    int i = (int)j; return (i >= 0 && i < 17) ? n[i] : "?";
}
static float berufsMinSkill(JB j) {
    switch (j) {
    case JB::Holzfaeller:    return 10;
    case JB::Bergmann:       return 15;
    case JB::Jaeger:         return 15;
    case JB::Fischer:        return 10;
    case JB::Koch:           return 20;
    case JB::Schmied:        return 30;
    case JB::Tischler:       return 25;
    case JB::Bauer:          return 10;
    case JB::Haendler:       return 20;
    case JB::Lehrer:         return 40;
    case JB::Wache:          return 25;
    case JB::Arzt:           return 35;
    case JB::Brauer:         return 25;
    case JB::Gerber:         return 20;
    case JB::Forscher:       return 50;
    case JB::Bergbaumeister: return 60;
    default:                 return  0;
    }
}
static SK berufsSkill(JB j) {
    switch (j) {
    case JB::Holzfaeller:    return SK::Holzfaellen;
    case JB::Bergmann:       return SK::Bergbau;
    case JB::Jaeger:         return SK::Jagd;
    case JB::Fischer:        return SK::Fischen;
    case JB::Koch:           return SK::Kochen;
    case JB::Schmied:        return SK::Schmieden;
    case JB::Tischler:       return SK::Tischlern;
    case JB::Bauer:          return SK::Holzfaellen;
    case JB::Haendler:       return SK::Handel;
    case JB::Lehrer:         return SK::Lehren;
    case JB::Wache:          return SK::Nahkampf;
    case JB::Arzt:           return SK::Heilen;
    case JB::Brauer:         return SK::Brauen;
    case JB::Gerber:         return SK::Gerben;
    case JB::Forscher:       return SK::Forschen;
    case JB::Bergbaumeister: return SK::Bergbaumeister;
    default:                 return SK::Holzfaellen;
    }
}

// ================================================================
//  GEBAEUDE (mit Isolierung/Heizung)
// ================================================================
enum class GebTyp { Haus, Mine, Werkstatt, Farm, Lager, Marktstand, Friedhof, Kirche, Wachturm };
struct Gebaeude {
    GebTyp typ;
    int    wx, wy;
    int    eigentuemerID;
    int    level = 1;
    int    haltbarkeit = 100;
    int    lagerkapazitaet = 50;
    Inventar lager;
    char   name[32] = "";
    int    isolierung = 0;  // 0-100, schützt vor Kälte
    int    heizung = 0;     // 0-100, aktiv gegen Kälte
    int    holzVorrat = 0;  // Für Heizung
};

// ================================================================
//  WETTER-SYSTEM
// ================================================================
enum class Wetter { Sonnig = 0, Bewoelkt, Regen, Gewitter, Schnee, Nebel };
static const char* wetterStr(Wetter w) {
    static const char* n[] = { "Sonnig","Bewoelkt","Regen","Gewitter","Schnee","Nebel" };
    return n[(int)w];
}
static WORD wetterFarbe(Wetter w) {
    switch (w) {
    case Wetter::Sonnig:    return C_WARN;
    case Wetter::Bewoelkt:  return C_DIM;
    case Wetter::Regen:     return C_RIVER;
    case Wetter::Gewitter:  return C_DANGER;
    case Wetter::Schnee:    return 0x0F;
    case Wetter::Nebel:     return C_STONE;
    default:                return C_DEFAULT;
    }
}
struct WetterSystem {
    Wetter aktuell = Wetter::Sonnig;
    int    dauer = 48;
    float  temp_mod = 0;

    void tick(int saison) {
        if (--dauer <= 0) wechsel(saison);
        switch (aktuell) {
        case Wetter::Regen:     temp_mod = -3.f; break;
        case Wetter::Gewitter:  temp_mod = -5.f; break;
        case Wetter::Schnee:    temp_mod = -8.f; break;
        case Wetter::Nebel:     temp_mod = -2.f; break;
        case Wetter::Sonnig:    temp_mod = 4.f; break;
        default:                temp_mod = 0.f; break;
        }
    }
    void wechsel(int saison) {
        float r = rf();
        if (saison == 3) {
            if (r < 0.35f) aktuell = Wetter::Schnee;
            else if (r < 0.55f) aktuell = Wetter::Bewoelkt;
            else if (r < 0.70f) aktuell = Wetter::Nebel;
            else if (r < 0.85f) aktuell = Wetter::Regen;
            else                aktuell = Wetter::Sonnig;
        }
        else if (saison == 1) {
            if (r < 0.45f) aktuell = Wetter::Sonnig;
            else if (r < 0.65f) aktuell = Wetter::Bewoelkt;
            else if (r < 0.80f) aktuell = Wetter::Gewitter;
            else                aktuell = Wetter::Regen;
        }
        else {
            if (r < 0.30f) aktuell = Wetter::Sonnig;
            else if (r < 0.55f) aktuell = Wetter::Bewoelkt;
            else if (r < 0.75f) aktuell = Wetter::Regen;
            else if (r < 0.85f) aktuell = Wetter::Gewitter;
            else                aktuell = Wetter::Nebel;
        }
        dauer = ri(12, 96);
    }
    float gesundheitsMod() const {
        switch (aktuell) {
        case Wetter::Regen:    return -0.05f;
        case Wetter::Gewitter: return -0.15f;
        case Wetter::Schnee:   return -0.20f;
        default:               return  0.f;
        }
    }
    bool verlangsamt() const {
        return aktuell == Wetter::Schnee || aktuell == Wetter::Gewitter || aktuell == Wetter::Nebel;
    }
    bool gibtRegen() const {
        return aktuell == Wetter::Regen || aktuell == Wetter::Gewitter;
    }
    char symbol() const {
        switch (aktuell) {
        case Wetter::Sonnig:   return '*';
        case Wetter::Bewoelkt: return 'o';
        case Wetter::Regen:    return ',';
        case Wetter::Gewitter: return '!';
        case Wetter::Schnee:   return '+';
        case Wetter::Nebel:    return '-';
        default:               return '?';
        }
    }
};

// ================================================================
//  MARKT / PREIS
// ================================================================
struct Angebot { int verkID; IT item; int menge; float preis; int wx, wy; };
struct Markt {
    std::vector<Angebot> liste;
    float marktpreis[IT_COUNT] = {};
    float angebot_s[IT_COUNT] = {};
    float nachfrage_s[IT_COUNT] = {};

    void init() {
        for (int i = 0; i < IT_COUNT; i++) {
            marktpreis[i] = itBasiswert((IT)i);
            angebot_s[i] = 5;
            nachfrage_s[i] = 3;
        }
    }
    float getPreis(IT t, int saison = 1) const {
        int i = (int)t; if (i < 0 || i >= IT_COUNT) return 1;
        float ratio = nachfrage_s[i] / (angebot_s[i] > 0.1f ? angebot_s[i] : 0.1f);
        float dyn = itBasiswert(t) * fc(ratio, 0.1f, 6.f);
        float sm = 1.f;
        if (t == IT::NahrIT || t == IT::Brot) sm = (saison == 3) ? 1.9f : (saison == 1) ? 0.8f : 1.f;
        if (t == IT::Stoff || t == IT::Pelz) sm = (saison == 3) ? 2.1f : (saison == 0) ? 0.6f : 1.f;
        if (t == IT::Medizin && saison == 3)   sm = 1.5f;
        return fc(dyn * sm, itBasiswert(t) * 0.15f, itBasiswert(t) * 8.f);
    }
    void addAngebot(int id, IT it, int m, float p, int wx = 0, int wy = 0) {
        liste.push_back({ id, it, m, p, wx, wy });
        if ((int)it < IT_COUNT) angebot_s[(int)it] += m;
    }
    void registriereNachfrage(IT t, float n = 1) {
        if ((int)t < IT_COUNT) nachfrage_s[(int)t] += n;
    }
    void tick(int saison) {
        for (int i = 0; i < IT_COUNT; i++) {
            float ziel = getPreis((IT)i, saison);
            marktpreis[i] = marktpreis[i] * 0.9f + ziel * 0.1f;
            angebot_s[i] = angebot_s[i] * 0.93f + 0.2f;
            nachfrage_s[i] = nachfrage_s[i] * 0.93f + 0.1f;
        }
        liste.erase(std::remove_if(liste.begin(), liste.end(),
            [](const Angebot& a) { return a.menge <= 0; }), liste.end());
        if (liste.size() > 800) liste.erase(liste.begin(), liste.begin() + 200);
    }
};

// ================================================================
//  SPIELZEIT
// ================================================================
struct Spielzeit {
    int tick = 0;
    int stunde() const { return tick % 24; }
    int tag()    const { return (tick / 24) % 30 + 1; }
    int monat()  const { return (tick / 24 / 30) % 12 + 1; }
    int jahr()   const { return tick / 24 / 30 / 12 + 1; }
    int saison() const { return (tick / 720) % 4; }
    void str(char* buf, int sz) const {
        static const char* sN[] = { "Fruehling","Sommer","Herbst","Winter" };
        static const char* mN[] = { "Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez" };
        _snprintf_s(buf, sz, _TRUNCATE, "%02d:00  %d. %s %s  Jahr %d",
            stunde(), tag(), mN[monat() - 1], sN[saison()], jahr());
    }
    WORD saisonFarbe() const {
        static const WORD c[] = { C_CHART, C_OK, C_WARN, C_ACCENT };
        return c[saison()];
    }
    float temperatur() const {
        static const float base[] = { 10, 24, 8, -6 };
        return base[saison()] + (float)sin((stunde() - 6) * 3.14159f / 12) * 5;
    }
};

// ================================================================
//  LOG
// ================================================================
struct LogEintrag { std::string text; int typ; };

// ================================================================
//  PERSON
// ================================================================
static int gNextID = 1;
struct Person {
    int  id;
    char name[32];
    int  alter;
    bool lebt, maennl;

    int  wx, wy;
    int  zielX, zielY;
    bool hatZiel = false;

    float hp, energie, staerke, geschick;
    float intel, weisheit, kreativ, lernfg;
    float mut, empathie, fleiss;
    float hunger, durst, glueck, stress, gesundheit;

    float skills[SK_COUNT] = {};
    Inventar inv;
    JB    beruf = JB::Keiner;
    float ruf = 0;
    float muenzen = 10;

    int partnerID = -1, mutterID = -1, vaterID = -1;
    std::vector<int> kinderIDs;
    std::vector<int> eigeneGebaeude;
    int  hausGebIdx = -1;

    char aktion[80] = "---";
    char todesursache[64] = "";
    int  handelAnzahl = 0;

    Person(const char* n, int altM, bool m, int startX = 0, int startY = 0) {
        id = gNextID++;
        strncpy_s(name, 32, n, _TRUNCATE);
        alter = altM; lebt = true; maennl = m;
        wx = startX + ri(-5, 5); wy = startY + ri(-5, 5);
        zielX = wx; zielY = wy;
        hp = rf() * 30 + 70; energie = rf() * 40 + 60;
        staerke = rf() * 50 + 20; geschick = rf() * 50 + 20;
        intel = rf() * 50 + 20; weisheit = rf() * 40 + 10;
        kreativ = rf() * 50 + 10; lernfg = rf() * 50 + 20;
        mut = rf() * 60 + 10; empathie = rf() * 60 + 10; fleiss = rf() * 60 + 10;
        hunger = rf() * 20;      durst = rf() * 20;
        glueck = rf() * 40 + 40; stress = rf() * 20; gesundheit = rf() * 30 + 70;
        for (int i = 0; i < SK_COUNT; i++) skills[i] = rf() * 8 + 1;
        muenzen = rf() * 15 + 5;
        inv.add(IT::NahrIT, ri(3, 8));
        inv.add(IT::Wasser, ri(5, 12));
    }
    int  alterJ()    const { return alter / 12; }
    bool istKind()   const { return alterJ() < 14; }
    bool istGreis()  const { return alterJ() > 68; }
    float skill(SK s) const { int i = (int)s; return (i >= 0 && i < SK_COUNT) ? skills[i] : 0; }
    void verbSkill(SK s, float amt) {
        int i = (int)s; if (i < 0 || i >= SK_COUNT) return;
        float b = 1.f + (intel / 100.f) * .6f + (lernfg / 100.f) * .4f;
        skills[i] = fc(skills[i] + amt * b, 0, 100);
    }
    SK besterSkill() const {
        SK b = SK::Holzfaellen; float bv = 0;
        for (int i = 0; i < SK_COUNT; i++) if (skills[i] > bv) { bv = skills[i]; b = (SK)i; }
        return b;
    }
    JB berechneberuf() const {
        if (istKind()) return JB::Keiner;
        static const JB reihenfolge[] = {
            JB::Bergbaumeister, JB::Forscher, JB::Arzt, JB::Lehrer,
            JB::Schmied, JB::Brauer, JB::Gerber,
            JB::Wache, JB::Haendler, JB::Tischler,
            JB::Koch, JB::Fischer, JB::Jaeger, JB::Bergmann,
            JB::Holzfaeller, JB::Bauer
        };
        for (auto j : reihenfolge) {
            SK s = berufsSkill(j);
            if (skill(s) >= berufsMinSkill(j)) return j;
        }
        return JB::Bauer;
    }
    void bewegeZuZiel() {
        if (!hatZiel || (wx == zielX && wy == zielY)) { hatZiel = false; return; }
        int dx = zielX - wx, dy = zielY - wy;
        if (abs(dx) + abs(dy) > 200) { hatZiel = false; return; }
        if (abs(dx) > abs(dy)) { wx += (dx > 0 ? 1 : -1); }
        else { wy += (dy > 0 ? 1 : -1); }
        wx = std::max(-500, std::min(500, wx));
        wy = std::max(-500, std::min(500, wy));
    }
    int distTo(int tx, int ty) const { return abs(wx - tx) + abs(wy - ty); }
};

// ================================================================
//  SIMULATION (optimierte Überlebensversion)
// ================================================================
static const char* mNam[] = {
    "Karl","Fritz","Hans","Konrad","Wilhelm","Otto","Ernst","Heinrich",
    "Georg","Ludwig","Bernd","Dieter","Rolf","Uwe","Felix","Gustav","Arno","Jonas",
    "Werner","Caspar","Klaus","Horst","Juergen","Lothar","Ewald","Bastian","Ingo","Hartmut",
    "Rupert","Siegfried","Albrecht","Bruno","Edmund","Friedrich" };
static const char* wNam[] = {
    "Anna","Maria","Lisa","Helga","Greta","Ingrid","Brigitte","Renate",
    "Monika","Ute","Ulrike","Sabine","Petra","Gudrun","Elfriede","Clara","Hanna",
    "Barbara","Anke","Astrid","Beate","Dagmar","Eva","Franziska","Julia","Ilona",
    "Christine","Dorothee","Edith","Frieda","Gerda","Hildegard","Irene","Johanna" };

struct Simulation {
    std::vector<std::shared_ptr<Person>> plist;
    std::vector<std::shared_ptr<Person>> neuePersonen;
    std::vector<Gebaeude>                gebaeude;
    Welt                                 welt;
    Markt                                markt;
    Spielzeit                            zeit;
    WetterSystem                         wetter;
    std::deque<LogEintrag>               logbuch;
    int geburten = 0, tode = 0, handels = 0, gebaeudeAnz = 0;
    int raeuberfaelle = 0;
    std::deque<float> histBev;

    struct Rekorde {
        int    maxBev = 0;
        int    maxAlter = 0;
        float  maxReichtum = 0;
        int    maxKinder = 0;
        char   aeltesterName[32] = "";
        char   reichsterName[32] = "";
        char   kinderreichName[32] = "";
    } rekorde;

    struct Grabstein { char name[32]; int alterJ; char ursache[64]; int tick; };
    std::deque<Grabstein> friedhof;

    int handelsvolumen[IT_COUNT] = {};

    Simulation() : welt(42) {}

    void addLog(const char* msg, int typ = 0) {
        char buf[256];
        _snprintf_s(buf, 255, _TRUNCATE, "[J%d %c %d.%02d] %s",
            zeit.jahr(), "SSHW"[zeit.saison()], zeit.tag(), zeit.stunde(), msg);
        logbuch.push_back({ buf, typ });
        if (logbuch.size() > 500) logbuch.pop_front();
    }
    const char* zname(bool m) {
        int nm = 34, nw = 34;
        return m ? mNam[ri(0, nm - 1)] : wNam[ri(0, nw - 1)];
    }

    // ============= NEUE HILFSFUNKTIONEN FÜR ÜBERLEBEN =============

    void wasserTrinken(Person& p) {
        if (p.inv.nimm(IT::Wasser, 2)) {
            p.durst = fc(p.durst - 35, 0, 100);
            strcpy_s(p.aktion, 80, "Trinkt");
            return;
        }

        // Nachbar-Wasserquellen prüfen
        for (int ddy = -1; ddy <= 1; ddy++) {
            for (int ddx = -1; ddx <= 1; ddx++) {
                if (ddx == 0 && ddy == 0) continue;
                Tile nt = welt.getTile(p.wx + ddx, p.wy + ddy);
                if (istWasserquelle(nt)) {
                    int menge = (nt == Tile::Fluss) ? 12 : (nt == Tile::See) ? 10 : 8;
                    p.inv.add(IT::Wasser, menge);
                    p.durst = fc(p.durst - 45, 0, 100);
                    strcpy_s(p.aktion, 80, nt == Tile::Fluss ? "Trinkt am Fluss" :
                        nt == Tile::See ? "Trinkt am See" : "Schoepft Wasser");
                    return;
                }
            }
        }

        // Wasserquelle suchen
        int fx, fy;
        if (welt.findNearestWasser(p.wx, p.wy, 60, fx, fy)) {
            p.zielX = fx; p.zielY = fy; p.hatZiel = true;
            strcpy_s(p.aktion, 80, "Geht zu Wasser");
        }
        else {
            // Notfall: Wasser kaufen
            for (auto& a : markt.liste) {
                if (a.item == IT::Wasser && a.menge > 0 && a.preis <= p.muenzen) {
                    p.muenzen -= a.preis; p.inv.add(IT::Wasser, 3); a.menge--;
                    strcpy_s(p.aktion, 80, "Kauft Wasser");
                    break;
                }
            }
        }
    }

    void essen(Person& p) {
        if (p.inv.nimm(IT::NahrIT)) { p.hunger = fc(p.hunger - 33, 0, 100); strcpy_s(p.aktion, 80, "Isst Nahrung"); }
        else if (p.inv.nimm(IT::Brot)) { p.hunger = fc(p.hunger - 28, 0, 100); strcpy_s(p.aktion, 80, "Isst Brot"); }
        else if (p.inv.nimm(IT::Fleisch)) { p.hunger = fc(p.hunger - 22, 0, 100); strcpy_s(p.aktion, 80, "Isst Fleisch"); }
        else if (p.inv.nimm(IT::Fisch)) { p.hunger = fc(p.hunger - 18, 0, 100); strcpy_s(p.aktion, 80, "Isst Fisch"); }
        else if (p.inv.nimm(IT::Pflanze)) { p.hunger = fc(p.hunger - 8, 0, 100); strcpy_s(p.aktion, 80, "Isst Pflanzen"); }
    }

    void schlafen(Person& p) {
        p.energie = fc(p.energie + 35, 0, 100);
        p.stress = fc(p.stress - 15, 0, 100);
        p.hp = fc(p.hp + 1.0f, 0, 100);
        strcpy_s(p.aktion, 80, "Schlaeft (erholt sich)");
    }

    void heilen(Person& p) {
        if (p.inv.hat(IT::Medizin)) {
            p.inv.nimm(IT::Medizin);
            p.gesundheit = fc(p.gesundheit + 25, 0, 100);
            p.hp = fc(p.hp + 15, 0, 100);
            strcpy_s(p.aktion, 80, "Nimmt Medizin");
        }
        else if (p.inv.hat(IT::Pflanze, 2)) {
            p.inv.nimm(IT::Pflanze, 2);
            p.gesundheit = fc(p.gesundheit + 8, 0, 100);
            strcpy_s(p.aktion, 80, "Heilt mit Pflanzen");
        }
    }

    void hausBauen(Person& p) {
        // Materialien für Haus besorgen
        if (!p.inv.hat(IT::Brett, 8) || !p.inv.hat(IT::Stein, 4)) {
            if (!p.inv.hat(IT::Brett, 8) && p.inv.hat(IT::Holz, 3)) {
                p.inv.nimm(IT::Holz, 3);
                p.inv.add(IT::Brett, 4);
                strcpy_s(p.aktion, 80, "Sagt Bretter");
                return;
            }

            int fx, fy;
            if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx, fy)) {
                p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                strcpy_s(p.aktion, 80, "Holt Holz");
                return;
            }
        }

        // Haus bauen, wenn Materialien da
        if (p.inv.hat(IT::Brett, 8) && p.inv.hat(IT::Stein, 4)) {
            int fx, fy;
            if (welt.findFreiePlatz(p.wx, p.wy, 6, fx, fy)) {
                p.hausGebIdx = baueGebaeude(p, GebTyp::Haus, fx, fy);
            }
        }
    }

    void kleidungBesorgen(Person& p) {
        // Versuche Stoff/Pelz zu kaufen
        for (auto& a : markt.liste) {
            if ((a.item == IT::Stoff || a.item == IT::Pelz) && a.preis <= p.muenzen) {
                p.muenzen -= a.preis;
                p.inv.add(a.item, 1);
                a.menge--;
                strcpy_s(p.aktion, 80, "Kauft Kleidung");
                return;
            }
        }

        // Oder selbst herstellen
        if (p.beruf == JB::Gerber && p.inv.hat(IT::Pelz, 2)) {
            p.inv.nimm(IT::Pelz, 2);
            p.inv.add(IT::Stoff, 3);
            strcpy_s(p.aktion, 80, "Gerbt Pelze");
        }
    }

    void heizungAuffuellen(Person& p) {
        if (p.hausGebIdx >= 0) {
            auto& haus = gebaeude[p.hausGebIdx];
            if (haus.holzVorrat < 10 && p.inv.hat(IT::Holz, 3)) {
                p.inv.nimm(IT::Holz, 3);
                haus.holzVorrat += 10;
                haus.heizung = std::min(100, haus.heizung + 20);
                strcpy_s(p.aktion, 80, "Heizt Haus");
            }
        }
    }

    void teileRessourcen(Person& p) {
        if (p.hausGebIdx < 0) return;

        for (auto& o : plist) {
            if (!o || !o->lebt || o->id == p.id) continue;
            if (p.distTo(o->wx, o->wy) > 5) continue;

            if (o->hunger > 70 && p.inv.hat(IT::NahrIT, 3)) {
                p.inv.nimm(IT::NahrIT, 2);
                o->inv.add(IT::NahrIT, 2);
                p.ruf += 1;
                strcpy_s(p.aktion, 80, "Teilt Nahrung");
            }
            if (o->durst > 70 && p.inv.hat(IT::Wasser, 5)) {
                p.inv.nimm(IT::Wasser, 3);
                o->inv.add(IT::Wasser, 3);
                p.ruf += 1;
            }
            if (zeit.saison() == 3 && o->hausGebIdx < 0 &&
                !o->inv.hat(IT::Stoff) && p.inv.hat(IT::Stoff, 2)) {
                p.inv.nimm(IT::Stoff, 1);
                o->inv.add(IT::Stoff, 1);
                p.ruf += 2;
            }
        }
    }

    // ============= BESTEHENDE FUNKTIONEN (angepasst) =============

    void init(int n = 80) {
        plist.clear(); neuePersonen.clear(); gebaeude.clear(); logbuch.clear();
        geburten = tode = handels = gebaeudeAnz = raeuberfaelle = 0;
        zeit.tick = 0; gNextID = 1;
        markt.init();
        histBev.clear();
        friedhof.clear();
        memset(&rekorde, 0, sizeof(rekorde));
        memset(handelsvolumen, 0, sizeof(handelsvolumen));
        wetter = WetterSystem{};

        for (int cy = -5; cy <= 5; cy++) for (int cx = -5; cx <= 5; cx++) welt.getChunk(cx, cy);

        // Garantierten Startfluss anlegen
        {
            for (int ry = -20; ry <= 20; ry++) {
                Tile t = welt.getTile(14, ry);
                if (t != Tile::Berg && t != Tile::Wasser) {
                    welt.setTile(14, ry, Tile::Fluss);
                    int cx2, cy2, lx2, ly2;
                    Welt::worldToChunk(14, ry, cx2, cy2, lx2, ly2);
                    welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                    welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
                }
            }
            for (int ry = -20; ry <= 20; ry++) {
                Tile t = welt.getTile(-14, ry);
                if (t != Tile::Berg && t != Tile::Wasser) {
                    welt.setTile(-14, ry, Tile::Fluss);
                    int cx2, cy2, lx2, ly2;
                    Welt::worldToChunk(-14, ry, cx2, cy2, lx2, ly2);
                    welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                    welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
                }
            }
            for (int dy = -2; dy <= 2; dy++) for (int dx = -3; dx <= 3; dx++) {
                int wx = 18 + dx, wy = 18 + dy;
                Tile t = welt.getTile(wx, wy);
                if (t != Tile::Berg) {
                    welt.setTile(wx, wy, Tile::See);
                    int cx2, cy2, lx2, ly2;
                    Welt::worldToChunk(wx, wy, cx2, cy2, lx2, ly2);
                    welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                    welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
                }
            }
        }

        for (int i = 0; i < n; i++) {
            bool  m = (rf() < .5f);
            int   alt;
            float r = rf();
            if (r < .15f) alt = ri(1 * 12, 12 * 12);
            else if (r < .80f) alt = ri(18 * 12, 42 * 12);
            else               alt = ri(43 * 12, 60 * 12);
            auto p = std::make_shared<Person>(zname(m), alt, m, 0, 0);
            if (!p->istKind()) p->beruf = p->berechneberuf();
            p->inv.add(IT::Wasser, ri(15, 25));
            p->inv.add(IT::NahrIT, ri(5, 12));
            p->durst = rf() * 15.f;
            p->hunger = rf() * 15.f;
            plist.push_back(p);
        }

        Gebaeude mkt;
        mkt.typ = GebTyp::Marktstand; mkt.wx = 0; mkt.wy = 0;
        mkt.eigentuemerID = -1; mkt.level = 1;
        strcpy_s(mkt.name, 32, "Zentralmarkt");
        gebaeude.push_back(mkt);
        welt.setTile(0, 0, Tile::Markt);
        gebaeudeAnz = 1;
        addLog("Siedlung gegruendet! Ueberlebensmodus aktiv.", 3);
    }

    int baueGebaeude(Person& p, GebTyp typ, int wx, int wy) {
        Gebaeude g;
        g.typ = typ; g.wx = wx; g.wy = wy; g.eigentuemerID = p.id; g.level = 1;
        g.isolierung = 20 + (int)(p.skill(SK::Bauen) / 5);
        g.heizung = p.inv.hat(IT::Holz, 5) ? 30 : 10;
        g.holzVorrat = p.inv.hat(IT::Holz, 5) ? 20 : 5;
        if (p.inv.hat(IT::Holz, 5)) p.inv.nimm(IT::Holz, 5);

        switch (typ) {
        case GebTyp::Haus:
            strcpy_s(g.name, 32, "Haus");
            welt.setTile(wx, wy, Tile::Haus);
            p.inv.nimm(IT::Brett, 8); p.inv.nimm(IT::Stein, 4);
            break;
        case GebTyp::Mine:
            strcpy_s(g.name, 32, "Mine");
            welt.setTile(wx, wy, Tile::Mine);
            p.inv.nimm(IT::Brett, 5); p.inv.nimm(IT::Werkzeug, 1);
            p.verbSkill(SK::Bergbau, 5);
            break;
        case GebTyp::Werkstatt:
            strcpy_s(g.name, 32, "Werkstatt");
            welt.setTile(wx, wy, Tile::Werkstatt);
            p.inv.nimm(IT::Brett, 12); p.inv.nimm(IT::Stein, 6);
            break;
        case GebTyp::Farm:
            strcpy_s(g.name, 32, "Farm");
            welt.setTile(wx, wy, Tile::Farm);
            p.inv.nimm(IT::Brett, 4);
            break;
        case GebTyp::Lager:
            strcpy_s(g.name, 32, "Lager");
            welt.setTile(wx, wy, Tile::Lager);
            p.inv.nimm(IT::Brett, 6); p.inv.nimm(IT::Stein, 3);
            break;
        case GebTyp::Friedhof:
            strcpy_s(g.name, 32, "Friedhof");
            welt.setTile(wx, wy, Tile::Friedhof);
            p.inv.nimm(IT::Stein, 5);
            break;
        case GebTyp::Kirche:
            strcpy_s(g.name, 32, "Kirche");
            welt.setTile(wx, wy, Tile::Kirche);
            p.inv.nimm(IT::Brett, 10); p.inv.nimm(IT::Stein, 8);
            break;
        case GebTyp::Wachturm:
            strcpy_s(g.name, 32, "Wachturm");
            welt.setTile(wx, wy, Tile::Wachturm);
            p.inv.nimm(IT::Brett, 6); p.inv.nimm(IT::Stein, 6);
            break;
        default: break;
        }
        int idx = (int)gebaeude.size();
        gebaeude.push_back(g);
        p.eigeneGebaeude.push_back(idx);
        gebaeudeAnz++;
        p.verbSkill(SK::Bauen, 3);
        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s baut %s bei (%d,%d)", p.name, g.name, wx, wy);
        addLog(buf, 5);
        return idx;
    }

    bool hatGebaeude(const Person& p, GebTyp typ) const {
        for (int idx : p.eigeneGebaeude) {
            if (idx >= 0 && idx < (int)gebaeude.size() &&
                gebaeude[idx].typ == typ &&
                gebaeude[idx].eigentuemerID == p.id) return true;
        }
        return false;
    }

    void arbeit(Person& p) {
        if (p.istKind()) {
            SK s = (SK)ri(0, SK_COUNT - 1);
            p.verbSkill(s, 0.05f);
            strcpy_s(p.aktion, 80, "Lernt und spielt");
            return;
        }
        if (zeit.tick % 120 == p.id % 120) p.beruf = p.berechneberuf();

        float eff = p.istGreis() ? 0.5f : 1.f;
        int   saison = zeit.saison();

        switch (p.beruf) {
        case JB::Holzfaeller: {
            int fx, fy;
            if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx, fy)) {
                if (p.distTo(fx, fy) > 1) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    strcpy_s(p.aktion, 80, "Geht zu Wald");
                }
                else {
                    int r = welt.getRess(fx, fy);
                    if (r > 0) {
                        welt.dekrementRess(fx, fy);
                        int menge = (int)(1 + p.skill(SK::Holzfaellen) / 30.f);
                        p.inv.add(IT::Holz, menge);
                        p.verbSkill(SK::Holzfaellen, 0.3f);
                        strcpy_s(p.aktion, 80, "Faellt Baum");
                    }
                }
            }
            else { strcpy_s(p.aktion, 80, "Sucht Wald..."); }
            break;
        }
        case JB::Bergmann: {
            bool hatMine = hatGebaeude(p, GebTyp::Mine);
            int  fx, fy; bool gefunden = false;
            if (hatMine) {
                for (int idx : p.eigeneGebaeude) {
                    if (idx < (int)gebaeude.size() && gebaeude[idx].typ == GebTyp::Mine) {
                        fx = gebaeude[idx].wx; fy = gebaeude[idx].wy; gefunden = true; break;
                    }
                }
            }
            else {
                gefunden = welt.findNearest(p.wx, p.wy, Tile::Huegel, 20, fx, fy);
                if (!gefunden) gefunden = welt.findNearest(p.wx, p.wy, Tile::Berg, 25, fx, fy);
            }
            if (gefunden) {
                if (p.distTo(fx, fy) > 1) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    strcpy_s(p.aktion, 80, "Geht zu Mine");
                }
                else {
                    int  r = welt.getRess(fx, fy);
                    Tile t = welt.getTile(fx, fy);
                    if (r > 0 || hatMine) {
                        if (r > 0) welt.dekrementRess(fx, fy);
                        float bonus = hatMine ? 1.5f : 1.f;
                        bonus *= (1.f + p.skill(SK::Bergbau) / 50.f) * eff;
                        if (rf() < 0.5f) {
                            p.inv.add(IT::Stein, (int)(bonus * 1.5f));
                            strcpy_s(p.aktion, 80, "Baut Stein ab");
                        }
                        else if (rf() < 0.6f || t == Tile::Huegel) {
                            p.inv.add(IT::Erz, (int)(bonus));
                            strcpy_s(p.aktion, 80, "Schuerft Erz");
                        }
                        else {
                            p.inv.add(IT::Kohle, (int)(bonus * .5f));
                            strcpy_s(p.aktion, 80, "Foerdert Kohle");
                        }
                        p.verbSkill(SK::Bergbau, 0.4f);
                    }
                }
            }
            else { strcpy_s(p.aktion, 80, "Sucht Huegel..."); }
            break;
        }
        case JB::Bergbaumeister: {
            int fx, fy; bool gef = false;
            for (int idx : p.eigeneGebaeude) {
                if (idx < (int)gebaeude.size() && gebaeude[idx].typ == GebTyp::Mine) {
                    fx = gebaeude[idx].wx; fy = gebaeude[idx].wy; gef = true; break;
                }
            }
            if (!gef) gef = welt.findNearest(p.wx, p.wy, Tile::Berg, 30, fx, fy);
            if (gef) {
                if (p.distTo(fx, fy) > 1) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    strcpy_s(p.aktion, 80, "Geht zu Tiefmine");
                }
                else {
                    float bonus = 2.f + p.skill(SK::Bergbaumeister) / 20.f;
                    p.inv.add(IT::Erz, (int)(bonus));
                    p.inv.add(IT::Stein, (int)(bonus * 1.5f));
                    if (rf() < 0.3f) { p.inv.add(IT::Gold, 1); strcpy_s(p.aktion, 80, "Findet Gold!"); }
                    else               strcpy_s(p.aktion, 80, "Tiefbau: Erz+Stein");
                    p.verbSkill(SK::Bergbaumeister, 0.5f);
                    p.verbSkill(SK::Bergbau, 0.2f);
                }
            }
            break;
        }
        case JB::Jaeger: {
            int fx, fy;
            if (welt.findNearest(p.wx, p.wy, Tile::Wald, 20, fx, fy)) {
                if (p.distTo(fx, fy) > 2) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    strcpy_s(p.aktion, 80, "Geht jagen");
                }
                else if (rf() < 0.4f + p.skill(SK::Jagd) / 200.f) {
                    p.inv.add(IT::Fleisch, ri(1, 4)); p.inv.add(IT::Pelz, 1);
                    p.verbSkill(SK::Jagd, 0.4f); strcpy_s(p.aktion, 80, "Jagt Tier");
                }
            }
            break;
        }
        case JB::Fischer: {
            int fx, fy;
            if (welt.findNearestWasser(p.wx, p.wy, 20, fx, fy)) {
                if (p.distTo(fx, fy) > 1) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    Tile wt = welt.getTile(fx, fy);
                    if (wt == Tile::Fluss) strcpy_s(p.aktion, 80, "Geht zum Fluss angeln");
                    else if (wt == Tile::See)   strcpy_s(p.aktion, 80, "Geht zum See angeln");
                    else                        strcpy_s(p.aktion, 80, "Geht fischen");
                }
                else {
                    Tile wt = welt.getTile(fx, fy);
                    int  menge = ri(1, 3) + (wt == Tile::Fluss ? 1 : 0);
                    p.inv.add(IT::Fisch, menge);
                    if (p.inv.anz(IT::Wasser) < 12) p.inv.add(IT::Wasser, 8);
                    p.verbSkill(SK::Fischen, 0.3f);
                    if (wt == Tile::Fluss) strcpy_s(p.aktion, 80, "Angelt im Fluss");
                    else if (wt == Tile::See)   strcpy_s(p.aktion, 80, "Angelt im See");
                    else                        strcpy_s(p.aktion, 80, "Angelt");
                }
            }
            break;
        }
        case JB::Koch: {
            if (p.inv.hat(IT::Fleisch, 1) && rf() < .6f) {
                p.inv.nimm(IT::Fleisch, 1); p.inv.add(IT::NahrIT, 3);
                p.verbSkill(SK::Kochen, 0.4f); strcpy_s(p.aktion, 80, "Kocht Fleisch");
            }
            else if (p.inv.hat(IT::Pflanze, 2)) {
                p.inv.nimm(IT::Pflanze, 2); p.inv.add(IT::NahrIT, 2);
                p.verbSkill(SK::Kochen, 0.3f); strcpy_s(p.aktion, 80, "Kocht Pflanzen");
            }
            else if (p.inv.hat(IT::Fisch)) {
                p.inv.nimm(IT::Fisch); p.inv.add(IT::NahrIT, 2);
                p.verbSkill(SK::Kochen, 0.3f); strcpy_s(p.aktion, 80, "Kocht Fisch");
            }
            else if (p.inv.hat(IT::Pflanze, 3) && rf() < .4f) {
                p.inv.nimm(IT::Pflanze, 3); p.inv.add(IT::Brot, 2);
                p.verbSkill(SK::Kochen, 0.5f); strcpy_s(p.aktion, 80, "Backt Brot");
            }
            break;
        }
        case JB::Schmied: {
            bool hatWerkstatt = hatGebaeude(p, GebTyp::Werkstatt);
            int  fx, fy;
            if (!hatWerkstatt && p.inv.hat(IT::Brett, 12) && p.inv.hat(IT::Stein, 6)) {
                if (welt.findFreiePlatz(p.wx, p.wy, 5, fx, fy))
                    baueGebaeude(p, GebTyp::Werkstatt, fx, fy);
            }
            if (p.inv.hat(IT::Erz, 3)) {
                p.inv.nimm(IT::Erz, 3);
                bool mitKohle = p.inv.nimm(IT::Kohle, 1);
                int  menge = mitKohle ? 2 : 1;
                p.inv.add(IT::Werkzeug, menge);
                p.verbSkill(SK::Schmieden, 0.6f);
                markt.addAngebot(p.id, IT::Werkzeug, menge,
                    markt.getPreis(IT::Werkzeug, saison) * 0.85f, p.wx, p.wy);
                _snprintf_s(p.aktion, 80, _TRUNCATE, mitKohle ? "Schmiedet Stahl-Wkz" : "Schmiedet Werkzeug");
            }
            else {
                strcpy_s(p.aktion, 80, "Wartet auf Erz...");
            }
            if (p.inv.hat(IT::Erz, 5) && rf() < .2f) {
                p.inv.nimm(IT::Erz, 5); p.inv.add(IT::Waffe, 1);
                markt.addAngebot(p.id, IT::Waffe, 1,
                    markt.getPreis(IT::Waffe, saison) * 0.8f, p.wx, p.wy);
                p.verbSkill(SK::Schmieden, 1.f); strcpy_s(p.aktion, 80, "Schmiedet Waffe");
            }
            break;
        }
        case JB::Tischler: {
            if (p.inv.hat(IT::Holz, 3)) {
                p.inv.nimm(IT::Holz, 3); p.inv.add(IT::Brett, 4);
                p.verbSkill(SK::Tischlern, 0.4f); strcpy_s(p.aktion, 80, "Saeagt Bretter");
            }
            else if (p.inv.hat(IT::Brett, 5) && rf() < .3f) {
                p.inv.nimm(IT::Brett, 5); p.inv.add(IT::Moebel, 1);
                markt.addAngebot(p.id, IT::Moebel, 1,
                    markt.getPreis(IT::Moebel, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Tischlern, 0.8f); strcpy_s(p.aktion, 80, "Baut Moebel");
            }
            else {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx, fy)) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                    strcpy_s(p.aktion, 80, "Holt Holz");
                }
            }
            break;
        }
        case JB::Bauer: {
            bool hatFarm = hatGebaeude(p, GebTyp::Farm);
            if (!hatFarm && p.inv.hat(IT::Brett, 4) && saison != 3) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 4, fx, fy))
                    baueGebaeude(p, GebTyp::Farm, fx, fy);
            }
            if (saison != 3) {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Flachland, 10, fx, fy)) {
                    if (p.distTo(fx, fy) > 1) {
                        p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                        strcpy_s(p.aktion, 80, "Geht aufs Feld");
                    }
                    else {
                        int menge = ri(1, 3) + (saison == 2 ? 2 : 0);
                        if (hatFarm) menge = (int)(menge * 1.5f);
                        p.inv.add(IT::Pflanze, menge);
                        p.verbSkill(SK::Holzfaellen, 0.1f);
                        strcpy_s(p.aktion, 80, saison == 2 ? "Erntet Feld" : "Pflegt Feld");
                    }
                }
            }
            else { strcpy_s(p.aktion, 80, "Winterpause"); }
            break;
        }
        case JB::Haendler: {
            tausch(p); break;
        }
        case JB::Lehrer: {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || p.distTo(o->wx, o->wy) > 3) continue;
                if (ri(0, 25) == 0) {
                    SK s = (SK)ri(0, SK_COUNT - 1);
                    if (p.skill(s) > o->skill(s) + 10) {
                        o->verbSkill(s, 0.5f * p.skill(SK::Lehren) / 100.f);
                        p.verbSkill(SK::Lehren, 0.2f); p.muenzen += 1.5f;
                        _snprintf_s(p.aktion, 80, _TRUNCATE, "Lehrt %s: %s", o->name, skStr(s));
                    }
                    break;
                }
            }
            break;
        }
        case JB::Wache: {
            if (abs(p.wx) > 20 || abs(p.wy) > 20) { p.zielX = 0; p.zielY = 0; p.hatZiel = true; }
            p.verbSkill(SK::Verteidigung, 0.15f); p.verbSkill(SK::Nahkampf, 0.1f);
            if (rf() < .4f) p.muenzen += 2.f;
            strcpy_s(p.aktion, 80, "Bewacht Siedlung");
            break;
        }
        case JB::Arzt: {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || p.distTo(o->wx, o->wy) > 6) continue;
                if (o->gesundheit < 60 && ri(0, 12) == 0) {
                    if (p.inv.hat(IT::Medizin)) {
                        float heil = 12 + p.skill(SK::Heilen) / 6.f;
                        o->gesundheit = fc(o->gesundheit + heil, 0, 100);
                        o->hp = fc(o->hp + 8, 0, 100);
                        p.inv.nimm(IT::Medizin); p.muenzen += 4; p.ruf += 2;
                        p.verbSkill(SK::Heilen, 0.5f);
                        _snprintf_s(p.aktion, 80, _TRUNCATE, "Behandelt %s", o->name);
                    }
                    else if (p.inv.hat(IT::Pflanze, 4)) {
                        p.inv.nimm(IT::Pflanze, 4); p.inv.add(IT::Medizin, 1);
                        p.verbSkill(SK::Heilen, 0.3f); strcpy_s(p.aktion, 80, "Stellt Medizin her");
                    }
                    break;
                }
            }
            break;
        }
        case JB::Brauer: {
            if (p.inv.hat(IT::Pflanze, 3)) {
                p.inv.nimm(IT::Pflanze, 3); p.inv.add(IT::Bier, 2);
                markt.addAngebot(p.id, IT::Bier, 2,
                    markt.getPreis(IT::Bier, saison) * .9f, p.wx, p.wy);
                p.verbSkill(SK::Brauen, 0.4f); strcpy_s(p.aktion, 80, "Braut Bier");
            }
            break;
        }
        case JB::Gerber: {
            if (p.inv.hat(IT::Pelz, 2)) {
                p.inv.nimm(IT::Pelz, 2); p.inv.add(IT::Stoff, 3);
                markt.addAngebot(p.id, IT::Stoff, 2,
                    markt.getPreis(IT::Stoff, saison) * .85f, p.wx, p.wy);
                p.verbSkill(SK::Gerben, 0.4f); strcpy_s(p.aktion, 80, "Gerbt Pelze");
            }
            break;
        }
        case JB::Forscher: {
            p.verbSkill(SK::Forschen, 0.5f);
            p.verbSkill((SK)ri(0, SK_COUNT - 1), 0.2f);
            if (rf() < .01f) {
                char b[80]; _snprintf_s(b, 80, _TRUNCATE, "%s macht Entdeckung!", p.name);
                addLog(b, 3);
            }
            strcpy_s(p.aktion, 80, "Forscht");
            break;
        }
        default: {
            if (rf() < .5f) { p.inv.add(IT::Pflanze, ri(1, 2)); strcpy_s(p.aktion, 80, "Sammelt Pflanzen"); }
            else {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Wald, 10, fx, fy)) {
                    p.zielX = fx; p.zielY = fy; p.hatZiel = true;
                }
            }
            break;
        }
        }
        float ek = rf() * 1.5f + 0.3f;
        if (p.beruf == JB::Bergmann || p.beruf == JB::Bergbaumeister) ek *= 1.8f;
        p.energie = fc(p.energie - ek, 0, 100);
    }

    void tausch(Person& p) {
        int saison = zeit.saison();
        IT    wunsch = IT::NahrIT;
        float maxB = 0;
        auto check = [&](IT it, float b) { if (b > maxB) { maxB = b; wunsch = it; } };
        check(IT::NahrIT, p.hunger);
        check(IT::Wasser, p.durst);
        check(IT::Medizin, p.gesundheit < 40 ? 70.f : 0.f);
        check(IT::Brett, p.hausGebIdx < 0 ? 55.f : 0.f);
        check(IT::Werkzeug, p.beruf == JB::Bergmann && !p.inv.hat(IT::Werkzeug) ? 60.f : 0.f);
        check(IT::Stoff, zeit.saison() == 3 && p.hausGebIdx < 0 ? 80.f : 0.f);
        check(IT::Pelz, zeit.saison() == 3 && p.hausGebIdx < 0 ? 80.f : 0.f);
        check(IT::Bier, p.stress > 70 ? 40.f : 0.f);

        markt.registriereNachfrage(wunsch);

        for (auto& a : markt.liste) {
            if (a.verkID == p.id || a.item != wunsch || a.menge <= 0) continue;
            float pr = markt.getPreis(wunsch, saison) * (1 - p.skill(SK::Handel) / 600.f);
            if (p.muenzen >= pr) {
                p.muenzen -= pr; p.inv.add(a.item); a.menge--;
                if (a.verkID > 0) {
                    for (auto& s : plist) {
                        if (s && s->lebt && s->id == a.verkID) {
                            s->muenzen += pr; s->handelAnzahl++; break;
                        }
                    }
                }
                p.verbSkill(SK::Handel, 0.2f);
                _snprintf_s(p.aktion, 80, _TRUNCATE, "Kauft %s %.1fM", itStr(wunsch), pr);
                handels++; break;
            }
        }
        static const IT off[] = {
            IT::Holz, IT::Stein, IT::Erz, IT::NahrIT, IT::Brot, IT::Fleisch,
            IT::Brett, IT::Werkzeug, IT::Moebel, IT::Bier, IT::Stoff, IT::Medizin,
            IT::Gold, IT::Pelz, IT::Kohle, IT::Keramik
        };
        for (IT it : off) {
            int cnt = p.inv.anz(it);
            int schw = (it == IT::NahrIT) ? 10 : (it == IT::Wasser) ? 14 : 4;
            if (cnt > schw) {
                float auf = (p.beruf == JB::Haendler) ? 1.15f : 0.9f;
                auf += p.skill(SK::Handel) / 400.f;
                markt.addAngebot(p.id, it, cnt - schw,
                    markt.getPreis(it, saison) * auf, p.wx, p.wy);
            }
        }
    }

    void sozial(Person& p) {
        if (p.partnerID < 0 && p.alterJ() >= 18 && p.alterJ() <= 52 && rf() < .06f) {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || o->maennl == p.maennl ||
                    o->partnerID >= 0 || o->istKind()) continue;
                if (p.distTo(o->wx, o->wy) > 20) continue;
                float k = 1.f - std::abs(p.empathie - o->empathie) / 100.f
                    - std::abs(p.intel - o->intel) / 200.f;
                if (k > .4f && rf() < k * .2f) {
                    p.partnerID = o->id; o->partnerID = p.id;
                    p.glueck = fc(p.glueck + 20, 0, 100);
                    o->glueck = fc(o->glueck + 20, 0, 100);
                    char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "Hochzeit: %s & %s", p.name, o->name);
                    addLog(buf, 4); break;
                }
            }
        }
        if (p.partnerID >= 0 && p.maennl && !p.istGreis()) {
            for (auto& partner : plist) {
                if (!partner || partner->id != p.partnerID || !partner->lebt || partner->istKind()) continue;
                bool res = p.inv.anz(IT::NahrIT) >= 3 && p.inv.anz(IT::Wasser) >= 5;
                if (res && partner->alterJ() >= 18 && partner->alterJ() <= 43 &&
                    p.alterJ() <= 53 && rf() < .04f)
                    neuesKind(p, *partner);
                break;
            }
        }
        if (p.inv.hat(IT::Bier) && rf() < .12f) {
            p.inv.nimm(IT::Bier);
            p.glueck = fc(p.glueck + 15, 0, 100);
            p.stress = fc(p.stress - 15, 0, 100);
            strcpy_s(p.aktion, 80, "Trinkt Bier");
        }
        p.glueck = fc(p.glueck + rf() * 2, 0, 100);
        p.stress = fc(p.stress - rf() * 3, 0, 100);
    }

    void neuesKind(Person& v, Person& m) {
        bool male = (rf() < .5f);
        auto c = std::make_shared<Person>(zname(male), 0, male, v.wx, v.wy);
        auto inh = [](float a, float b) { return fc((a + b) / 2 + (-4 + rf() * 8), 1, 100); };
        c->intel = inh(v.intel, m.intel);
        c->staerke = inh(v.staerke, m.staerke);
        c->empathie = inh(v.empathie, m.empathie);
        c->fleiss = inh(v.fleiss, m.fleiss);
        c->lernfg = inh(v.lernfg, m.lernfg);
        for (int s = 0; s < SK_COUNT; s++) c->skills[s] = (v.skills[s] + m.skills[s]) * .06f;
        c->vaterID = v.id; c->mutterID = m.id;
        if (v.hausGebIdx >= 0) c->hausGebIdx = v.hausGebIdx;
        v.kinderIDs.push_back(c->id); m.kinderIDs.push_back(c->id);
        v.glueck = fc(v.glueck + 25, 0, 100);
        m.glueck = fc(m.glueck + 25, 0, 100);
        char buf[128]; _snprintf_s(buf, 128, _TRUNCATE, "Geburt: %s (Eltern: %s & %s)", c->name, v.name, m.name);
        addLog(buf, 2);
        neuePersonen.push_back(c);
        geburten++;
    }

    void sterbenCheck(Person& p) {
        if (p.hp <= 0) { tod(p, "Verletzung/Erschoepfung"); return; }
        if (p.hunger >= 100) { tod(p, "Verhungert");  return; }
        if (p.durst >= 100) { tod(p, "Verdurstet");  return; }
        int s = zeit.saison();
        if (s == 3 && p.hausGebIdx < 0 && !p.inv.hat(IT::Stoff) && !p.inv.hat(IT::Pelz) && rf() < .006f) {
            tod(p, "Erfroren"); return;
        }
        if (p.alterJ() > 72) {
            float sc = 0.000004f * (p.alterJ() - 72);
            if (rf() < sc) tod(p, "Hohes Alter");
        }
    }
    void tod(Person& p, const char* ursache) {
        p.lebt = false; tode++;
        strncpy_s(p.todesursache, 64, ursache, _TRUNCATE);
        char b[128]; _snprintf_s(b, 128, _TRUNCATE, "TOD: %s (%dJ) - %s", p.name, p.alterJ(), ursache);
        addLog(b, 1);
        if (p.partnerID >= 0) {
            for (auto& o : plist)
                if (o && o->id == p.partnerID) { o->partnerID = -1; o->glueck = fc(o->glueck - 25, 0, 100); break; }
        }
    }

    void ereignisse() {
        if (rf() < .0012f) {
            int  k = ri(1, 6); char txt[128]; int typ = 3;
            switch (k) {
            case 1:
                _snprintf_s(txt, 128, _TRUNCATE, "DUERRE! Felder trocknen aus.");
                for (auto& p : plist) if (p->lebt) p->inv.nimm(IT::Pflanze, p->inv.anz(IT::Pflanze) / 2);
                break;
            case 2:
                _snprintf_s(txt, 128, _TRUNCATE, "FLUT! Keller unter Wasser.");
                for (auto& p : plist) if (p->lebt && rf() < .2f)
                    p->inv.nimm(IT::NahrIT, p->inv.anz(IT::NahrIT) / 3);
                break;
            case 3: {
                static const char* sn[] = { "Pest","Grippe","Typhus","Cholera" };
                const char* s = sn[ri(0, 3)];
                _snprintf_s(txt, 128, _TRUNCATE, "SEUCHE: %s bricht aus!", s); typ = 1;
                for (auto& p : plist) if (p->lebt && rf() < .2f)
                    p->gesundheit = fc(p->gesundheit - 25, 0, 100);
                break;
            }
            case 4: {
                static const char* en[] = { "Muehle","Glasherstellung","Stahllegierung","Drainagesystem" };
                _snprintf_s(txt, 128, _TRUNCATE, "ENTDECKUNG: %s!", en[ri(0, 3)]); typ = 2;
                for (auto& p : plist) if (p->lebt && p->intel > 60) p->verbSkill(SK::Forschen, 10);
                break;
            }
            case 5: {
                int n2 = ri(8, 20);
                _snprintf_s(txt, 128, _TRUNCATE, "EINWANDERUNG: %d Personen kommen!", n2); typ = 2;
                for (int i = 0; i < n2; i++) {
                    bool m = (rf() < .5f);
                    auto np = std::make_shared<Person>(zname(m), ri(20 * 12, 35 * 12), m, ri(-10, 10), ri(-10, 10));
                    np->beruf = np->berechneberuf();
                    neuePersonen.push_back(np);
                }
                break;
            }
            case 6:
                if (zeit.saison() == 2) {
                    _snprintf_s(txt, 128, _TRUNCATE, "REICHE ERNTE! Alle Felder voll."); typ = 2;
                    for (auto& p : plist) if (p->lebt && (p->beruf == JB::Bauer || hatGebaeude(*p, GebTyp::Farm)))
                        p->inv.add(IT::Pflanze, ri(8, 20));
                }
                else return;
                break;
            default: return;
            }
            addLog(txt, typ);
        }
    }

    void schritt() {
        zeit.tick++;
        markt.tick(zeit.saison());
        wetter.tick(zeit.saison());
        ereignisse();

        // Rohstoff-Regeneration
        if (zeit.tick % 240 == 0) {
            for (auto& kv : welt.chunks) {
                for (int ly = 0; ly < CHUNK_SZ; ly++) {
                    for (int lx = 0; lx < CHUNK_SZ; lx++) {
                        Tile t = kv.second.tiles[ly][lx];
                        if ((t == Tile::Wald || t == Tile::Huegel) && kv.second.ressLeft[ly][lx] < 20)
                            kv.second.ressLeft[ly][lx]++;
                    }
                }
            }
        }

        // Häuser heizen im Winter
        if (zeit.saison() == 3 && zeit.tick % 24 == 0) {
            for (auto& g : gebaeude) {
                if (g.typ == GebTyp::Haus && g.holzVorrat > 0) {
                    g.holzVorrat--;
                    g.heizung = std::min(100, g.heizung + 5);
                }
                else {
                    g.heizung = std::max(0, g.heizung - 2);
                }
            }
        }

        bool  istNacht = (zeit.stunde() >= 22 || zeit.stunde() <= 5);
        bool  istWinter = (zeit.saison() == 3);
        float nm = istWinter ? 1.8f : (zeit.saison() == 2) ? 1.2f : 1.0f;

        // Regen
        if (wetter.gibtRegen()) {
            for (auto& p : plist) {
                if (p->lebt && p->hausGebIdx < 0 && p->inv.anz(IT::Wasser) < 20)
                    p->inv.add(IT::Wasser, 2);
            }
        }

        for (auto& p : plist) {
            if (!p->lebt) continue;
            if (zeit.tick % 720 == (p->id % 720)) p->alter++;

            float hr = 0.30f * nm * (p->istKind() ? .7f : 1.f);
            p->hunger = fc(p->hunger + rf() * hr + hr * .20f, 0, 100);
            p->durst = fc(p->durst + rf() * .30f + .08f, 0, 100);
            p->energie = fc(p->energie - rf() * .35f - .1f, 0, 100);
            p->stress = fc(p->stress + rf() * .15f, 0, 100);

            // Wetter-Effekte
            if (p->hausGebIdx >= 0) {
                // Im Haus - Wärmeschutz
                auto& haus = gebaeude[p->hausGebIdx];
                float schutz = (haus.isolierung + haus.heizung) / 200.f;
                p->gesundheit = fc(p->gesundheit + wetter.gesundheitsMod() * (1.f - schutz), 0, 100);
                if (istWinter && haus.heizung > 20)
                    p->gesundheit = fc(p->gesundheit + 0.3f, 0, 100);
            }
            else {
                // Draussen
                p->gesundheit = fc(p->gesundheit + wetter.gesundheitsMod(), 0, 100);
                if (wetter.verlangsamt() && p->hatZiel && rf() < 0.5f)
                    p->bewegeZuZiel();
            }

            if (p->hunger > 72) p->gesundheit = fc(p->gesundheit - 0.7f, 0, 100);
            if (p->durst > 72) p->gesundheit = fc(p->gesundheit - 1.1f, 0, 100);
            if (p->gesundheit < 25) p->hp = fc(p->hp - 0.8f, 0, 100);

            // Winter-Kälteschutz
            if (istWinter && p->hausGebIdx < 0) {
                float k = istNacht ? 2.5f : 1.f;
                if (!p->inv.hat(IT::Stoff) && !p->inv.hat(IT::Pelz)) {
                    p->gesundheit = fc(p->gesundheit - k, 0, 100);
                }
                else {
                    p->gesundheit = fc(p->gesundheit - k * 0.4f, 0, 100);
                }
            }

            sterbenCheck(*p); if (!p->lebt) continue;

            if (p->hatZiel) p->bewegeZuZiel();

            // ===== OPTIMIERTE PRIORITÄTEN-REIHENFOLGE =====

            // 1. Schlaf (höchste Priorität bei niedriger Energie)
            if (p->energie < 30) {
                schlafen(*p);
                continue;
            }

            // 2. Durst (lebensbedrohlich)
            if (p->durst > 40) {
                wasserTrinken(*p);
                continue;
            }

            // 3. Hunger
            if (p->hunger > 50) {
                essen(*p);
                continue;
            }

            // 4. Gesundheit
            if (p->gesundheit < 40) {
                heilen(*p);
                continue;
            }

            // 5. Winter-Kälteschutz
            if (istWinter && p->hausGebIdx < 0) {
                if (!p->inv.hat(IT::Stoff) && !p->inv.hat(IT::Pelz)) {
                    kleidungBesorgen(*p);
                    continue;
                }
            }

            // 6. Haus bauen (wenn kein Haus)
            if (p->hausGebIdx < 0 && !p->istKind()) {
                hausBauen(*p);
                continue;
            }

            // 7. Heizung auffüllen
            if (istWinter && p->hausGebIdx >= 0 && p->inv.hat(IT::Holz, 3)) {
                heizungAuffuellen(*p);
                // Nicht continue - kann trotzdem arbeiten
            }

            // 8. Ressourcen teilen (sozial)
            if (p->ruf < 50 && rf() < 0.1f) {
                teileRessourcen(*p);
                // Nicht continue
            }

            // 9. Normale Arbeit/Soziales
            float rr = rf();
            if (rr < .50f) arbeit(*p);
            else if (rr < .65f) tausch(*p);
            else if (rr < .80f) sozial(*p);
            else {
                SK s = (SK)ri(0, SK_COUNT - 1); p->verbSkill(s, .1f);
                _snprintf_s(p->aktion, 80, _TRUNCATE, "Uebt %s", skStr(s));
            }
        }

        for (auto& np : neuePersonen) plist.push_back(np);
        neuePersonen.clear();

        if (zeit.tick % 24 == 0) {
            int leb = 0; for (auto& p : plist) if (p->lebt) leb++;
            histBev.push_back((float)leb);
            if (histBev.size() > 60) histBev.pop_front();

            // Rekorde aktualisieren
            if (leb > rekorde.maxBev) rekorde.maxBev = leb;
        }
    }

    struct Stats {
        int   lebendig = 0, kinder = 0, paare = 0, haeuser = 0;
        float avgGlueck = 0, avgGesund = 0, avgHunger = 0, avgAlter = 0;
        std::map<JB, int> berufe;
        int   aelteste = 0; float reichste = 0; char reichsterName[32] = "";
        float tempAktuell = 0;
    };
    Stats getStats() const {
        Stats s; int n = 0;
        s.tempAktuell = zeit.temperatur();
        for (auto& p : plist) {
            if (!p->lebt) continue;
            s.lebendig++; n++;
            if (p->istKind())       s.kinder++;
            if (p->partnerID >= 0)  s.paare++;
            if (p->hausGebIdx >= 0) s.haeuser++;
            s.avgGlueck += p->glueck;
            s.avgGesund += p->gesundheit;
            s.avgHunger += p->hunger;
            s.avgAlter += (float)p->alterJ();
            s.berufe[p->beruf]++;
            if (p->alterJ() > s.aelteste) s.aelteste = p->alterJ();
            float w = p->muenzen + p->inv.wert();
            if (w > s.reichste) { s.reichste = w; strncpy_s(s.reichsterName, 32, p->name, _TRUNCATE); }
        }
        if (n) { s.avgGlueck /= n; s.avgGesund /= n; s.avgHunger /= n; s.avgAlter /= n; }
        s.paare /= 2;
        return s;
    }
};

// ================================================================
//  TUI (unverändert)
// ================================================================
struct TUI {
    Simulation sim;
    bool simLaeuft = false;
    int  speed = 5;
    int  btnSel = 0; bool btnFokus = false;
    int  lstSel = 0, lstScroll = 0; bool lstDetail = false;
    int  aktPanel = 0;
    static const int PANELS = 5;
    int  logScroll = 0;

    int  camX = 0, camY = 0;
    bool folgeSelected = false;

    std::chrono::steady_clock::time_point lastTick;

    void run() {
        con.init();
        SetConsoleTitleA("Lebenssimulation v3.3 - Ueberlebens-Edition");
        sim.init(80);
        lastTick = std::chrono::steady_clock::now();
        while (true) {
            con.refreshSize();
            if (simLaeuft) {
                int ms = 1100 - speed * 100;
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count() >= ms) {
                    sim.schritt(); lastTick = now;
                }
            }
            con.clear();
            draw(con.W, con.H);
            con.flip();
            int k = con.getKey();
            if (k == -1) { Sleep(15); continue; }
            if (!input(k)) break;
        }
        con.restore();
    }

    void draw(int W, int H) {
        int btnH = 3, botH = (std::max)(9, H / 4);
        int mainT = btnH + 1, mainH = H - mainT - botH;
        int leftW = (std::max)(24, W * 2 / 5), rightW = W - leftW;
        int statsW = W / 3, handelW = W / 3, logW = W - statsW - handelW;

        drawTitle(0, W);
        drawButtons(1, 0, btnH, W);
        drawListe(mainT, 0, mainH, leftW);
        drawMap(mainT, leftW, mainH, rightW);
        drawStats(H - botH, 0, botH, statsW);
        drawMarkt(H - botH, statsW, botH, handelW);
        drawLog(H - botH, statsW + handelW, botH, logW);
        drawHint(H - 1, W);
    }

    void drawTitle(int y, int W) {
        con.fillLine(y, ' ', C_HEADER);
        char zeitBuf[80]; sim.zeit.str(zeitBuf, 80);
        auto st = sim.getStats();
        char left[160];
        _snprintf_s(left, 160, _TRUNCATE, "  LEBENSSIMULATION v3.3 (UEBERLEBEN)  |  %s  |  %.1f%cC",
            zeitBuf, st.tempAktuell, '\xF8');
        char right[120];
        _snprintf_s(right, 120, _TRUNCATE, " Bev:%d  Kinder:%d  Haeuser:%d  Geb:%d  Tod:%d  ",
            st.lebendig, st.kinder, st.haeuser, sim.geburten, sim.tode);
        con.print(0, y, left, C_HEADER);
        int rp = W - (int)strlen(right); if (rp > 0) con.print(rp, y, right, C_HEADER);
    }

    void drawButtons(int y, int x, int h, int W) {
        for (int i = 0; i < W; i++) con.put(i, y + h - 1, '\xC4', C_BORDER);
        const char* btns[] = { "[1] START","[2] PAUSE","[3] RESET","[+] SCHNELL","[-] LANGSAM" };
        int bx = x + 2;
        for (int i = 0; i < 5; i++) {
            bool sel = btnFokus && btnSel == i;
            bool act = (i == 0 && simLaeuft) || (i == 1 && !simLaeuft && sim.zeit.tick > 0);
            WORD attr = sel ? C_BTNSEL : (act ? C_OK : C_BTN);
            char buf[32]; _snprintf_s(buf, 32, _TRUNCATE, " %s ", btns[i]);
            con.print(bx, y + 1, buf, attr); bx += (int)strlen(btns[i]) + 4;
        }
        int sx = bx + 2;
        if (sx + 30 < W) {
            con.print(sx, y + 1, "Geschw:", C_DIM);
            for (int i = 0; i < 10; i++) con.put(sx + 8 + i, y + 1, i < speed ? '\xDB' : '-', C_ACCENT);
            char spd[10]; _snprintf_s(spd, 10, _TRUNCATE, " %d/10", speed);
            con.print(sx + 18, y + 1, spd, C_ACCENT);
        }
        char ss[30]; _snprintf_s(ss, 30, _TRUNCATE, " Kamera:(%d,%d) ", camX, camY);
        con.print(W - 28, y + 1, ss, C_DIM);
        const char* st = simLaeuft ? ">> LAUFEND" : sim.zeit.tick > 0 ? "|| PAUSIERT" : "   GESTOPPT";
        WORD        stc = simLaeuft ? C_OK : sim.zeit.tick > 0 ? C_WARN : C_DIM;
        con.print(W - 11, y + 1, st, stc);
    }

    Person* getSelectedPerson() {
        std::vector<int> alive;
        for (int i = 0; i < (int)sim.plist.size(); i++) if (sim.plist[i]->lebt) alive.push_back(i);
        if (alive.empty() || lstSel >= (int)alive.size()) return nullptr;
        return sim.plist[alive[lstSel]].get();
    }

    void drawListe(int y, int x, int h, int w) {
        bool act = (aktPanel == 0);
        con.boxTitle(x, y, w, h, lstDetail ? "PERSON DETAIL" : "BEWOHNER", act);
        if (h < 4 || w < 10) return;
        int iw = w - 2, ih = h - 2;

        if (!lstDetail) {
            std::vector<int> alive;
            for (int i = 0; i < (int)sim.plist.size(); i++) if (sim.plist[i]->lebt) alive.push_back(i);
            int total = (int)alive.size();
            if (total == 0) { con.print(x + 1, y + 1, "Alle gestorben!", C_DANGER); return; }
            if (lstSel >= total)  lstSel = total - 1;
            if (lstSel < lstScroll)           lstScroll = lstSel;
            if (lstSel >= lstScroll + ih)     lstScroll = lstSel - ih + 1;

            for (int row = 0; row < ih && row + lstScroll < total; row++) {
                int     idx = alive[row + lstScroll];
                Person& p = *sim.plist[idx];
                bool    sel = (row + lstScroll == lstSel);
                WORD    base = sel ? C_SEL : C_DEFAULT;
                int     nmw = iw - 20; if (nmw < 4) nmw = 4;
                char age[6];
                if (p.istKind()) _snprintf_s(age, 6, _TRUNCATE, "%2dM", p.alter);
                else             _snprintf_s(age, 6, _TRUNCATE, "%2dJ", p.alterJ());
                char line[128];
                _snprintf_s(line, 128, _TRUNCATE, "%-*.*s%s %-8s", nmw, nmw, p.name, age, jbStr(p.beruf));
                con.print(x + 1, y + 1 + row, line, base);
                if (!sel) {
                    int  hf = (int)(7 * p.hp / 100);
                    char hb[10]; for (int i = 0; i < 7; i++) hb[i] = (i < hf ? '\xDB' : '-'); hb[7] = 0;
                    WORD hc = p.hp > 60 ? C_OK : p.hp > 30 ? C_WARN : C_DANGER;
                    con.put(x + 1 + iw - 9, y + 1 + row, '[', hc);
                    con.print(x + 1 + iw - 8, y + 1 + row, hb, hc);
                    con.put(x + 1 + iw - 1, y + 1 + row, ']', hc);
                }
                if (sel) for (int fx = x + 1 + (int)strlen(line) + 1; fx < x + 1 + iw; fx++) con.put(fx, y + 1 + row, ' ', C_SEL);
            }
            if (total > ih) {
                int pos = (int)((float)(ih - 1) * lstScroll / (float)(std::max)(1, total - ih));
                for (int i = 0; i < ih; i++) con.put(x + w - 1, y + 1 + i, i == pos ? '\xDB' : '|', C_DIM);
            }
        }
        else {
            Person* pp = getSelectedPerson(); if (!pp) { con.print(x + 2, y + 2, "Niemand", C_WARN); return; }
            Person& p = *pp;
            int cy2 = y + 1;
            char hdr[64]; _snprintf_s(hdr, 64, _TRUNCATE, "%-*s", iw, p.name); con.print(x + 1, cy2++, hdr, C_ACCENT);
            char sub[80]; _snprintf_s(sub, 80, _TRUNCATE, "%dJ  %s  Beruf: %s", p.alterJ(), p.maennl ? "M" : "W", jbStr(p.beruf));
            con.print(x + 1, cy2++, sub, C_DIM);
            char pos[40]; _snprintf_s(pos, 40, _TRUNCATE, "Pos:(%d,%d)  Muenzen:%.0f  Ruf:%.0f", p.wx, p.wy, p.muenzen, p.ruf);
            con.print(x + 1, cy2++, pos, C_DIM);
            if (cy2 >= y + h - 1) return;
            auto sr = [&](const char* l, float v, WORD c) {
                if (cy2 >= y + h - 1) return;
                int bw = iw - 13; if (bw < 2) bw = 2;
                char lb[10]; _snprintf_s(lb, 10, _TRUNCATE, "%-8s", l);
                con.print(x + 1, cy2, lb, C_DIM);
                con.put(x + 9, cy2, '[', c);
                int f = (int)(bw * v / 100);
                for (int i = 0; i < bw; i++) con.put(x + 10 + i, cy2, i < f ? '\xDB' : '-', c);
                con.put(x + 10 + bw, cy2, ']', c);
                char pt[6]; _snprintf_s(pt, 6, _TRUNCATE, "%3d%%", (int)v);
                con.print(x + 11 + bw, cy2, pt, c); cy2++;
                };
            sr("HP", p.hp, C_OK);
            sr("Energie", p.energie, C_ACCENT);
            sr("Hunger", p.hunger, p.hunger > 70 ? C_DANGER : C_WARN);
            sr("Durst", p.durst, p.durst > 70 ? C_DANGER : C_WARN);
            sr("Glueck", p.glueck, C_OK);
            sr("Gesundh", p.gesundheit, p.gesundheit < 40 ? C_DANGER : C_OK);
            sr("Stress", p.stress, p.stress > 70 ? C_DANGER : C_WARN);
            if (cy2 < y + h - 1) {
                char ex[80];
                _snprintf_s(ex, 80, _TRUNCATE, "Kinder:%d  Partner:%s  Haus:%s",
                    (int)p.kinderIDs.size(), p.partnerID >= 0 ? "ja" : "nein", p.hausGebIdx >= 0 ? "ja" : "nein");
                con.print(x + 1, cy2++, ex, C_DIM);
            }
            if (cy2 < y + h - 1) {
                char ak[82]; _snprintf_s(ak, 82, _TRUNCATE, ">> %s", p.aktion);
                con.print(x + 1, cy2++, ak, C_LOG);
            }
            if (cy2 < y + h - 2) con.print(x + 1, cy2++, "Inventar:", C_DIM);
            for (int i = 0; i < IT_COUNT && cy2 < y + h - 1; i++) {
                if (p.inv.items[i] > 0) {
                    char iv[32]; _snprintf_s(iv, 32, _TRUNCATE, "  %-10s:%3d", itStr((IT)i), p.inv.items[i]);
                    con.print(x + 1, cy2++, iv, C_CHART);
                }
            }
            if (cy2 < y + h - 2) con.print(x + 1, cy2++, "Top Skills:", C_DIM);
            std::vector<std::pair<float, int>> sv;
            for (int i = 0; i < SK_COUNT; i++) sv.push_back({ p.skills[i], i });
            std::sort(sv.rbegin(), sv.rend());
            for (int i = 0; i < 5 && cy2 < y + h - 1; i++) {
                int bw = (std::max)(1, iw - 18);
                int fill = (int)(bw * sv[i].first / 100);
                char sl[16]; _snprintf_s(sl, 16, _TRUNCATE, "%-14s", skStr((SK)sv[i].second));
                con.print(x + 1, cy2, sl, C_DIM);
                for (int b = 0; b < bw; b++) con.put(x + 15 + b, cy2, b < fill ? '\xDB' : '-', C_CHART);
                char sv2[8]; _snprintf_s(sv2, 8, _TRUNCATE, "%5.1f", sv[i].first);
                con.print(x + 15 + bw, cy2, sv2, C_CHART); cy2++;
            }
        }
    }

    void drawMap(int y, int x, int h, int w) {
        bool act = (aktPanel == 1);
        con.boxTitle(x, y, w, h, "WELT (Pfeile=Kamera, F=Folgen)", act);
        if (h < 4 || w < 8) return;
        int dW = w - 2, dH = h - 2;

        if (folgeSelected) {
            Person* pp = getSelectedPerson();
            if (pp) { camX = pp->wx - (dW / 2); camY = pp->wy - (dH / 2); }
        }

        for (int dy = 0; dy < dH; dy++) {
            for (int dx = 0; dx < dW; dx++) {
                int  wx = camX + dx, wy = camY + dy;
                Tile t = sim.welt.getTile(wx, wy);
                char c = tileChar(t);
                WORD col = tileColor(t);
                if (t == Tile::Wald || t == Tile::Huegel || t == Tile::Berg) {
                    if (sim.welt.getRess(wx, wy) <= 2) col = C_DIM;
                }
                if (t == Tile::Fluss && (dx + dy) % 2 == 0) col = C_RIVER | 0x80;
                con.put(x + 1 + dx, y + 1 + dy, c, col);
            }
        }

        std::map<std::pair<int, int>, int> personPos;
        for (auto& p : sim.plist) {
            if (!p->lebt) continue;
            int dx = p->wx - camX, dy = p->wy - camY;
            if (dx >= 0 && dx < dW && dy >= 0 && dy < dH)
                personPos[{dx, dy}]++;
        }
        for (auto& pp : personPos) {
            int  dx = pp.first.first, dy = pp.first.second;
            int  cnt = pp.second;
            char c = (cnt > 9) ? '+' : ('0' + cnt);
            WORD col = cnt > 5 ? C_DANGER : cnt > 2 ? C_WARN : C_PERSON;
            con.put(x + 1 + dx, y + 1 + dy, c, col);
        }

        Person* selP = getSelectedPerson();
        if (selP) {
            int dx = selP->wx - camX, dy = selP->wy - camY;
            if (dx >= 0 && dx < dW && dy >= 0 && dy < dH)
                con.put(x + 1 + dx, y + 1 + dy, '@', C_BTNSEL);
            if (selP->hatZiel) {
                int tdx = selP->zielX - camX, tdy = selP->zielY - camY;
                if (tdx >= 0 && tdx < dW && tdy >= 0 && tdy < dH)
                    con.put(x + 1 + tdx, y + 1 + tdy, 'x', C_TRADE);
            }
        }

        int ly = y + h - 2;
        if (dH > 4) con.print(x + 1, ly,
            "~=Meer \xF7=Fluss \xF0=See .=Feld T=Wald n=Hgl ^=Berg H=Haus M=Mine $=Markt @=Sel", C_DIM);
        for (int dx = 0; dx < dW; dx++) {
            int wx = camX + dx;
            if (((wx % CHUNK_SZ) + CHUNK_SZ) % CHUNK_SZ == 0) con.put(x + 1 + dx, y, '\xC2', C_DIM);
        }
        char coord[32]; _snprintf_s(coord, 32, _TRUNCATE, "(%d,%d)-(+%d,+%d)", camX, camY, dW, dH);
        con.print(x + w - 1 - (int)strlen(coord) - 1, y, coord, C_DIM);
    }

    void drawStats(int y, int x, int h, int w) {
        bool act = (aktPanel == 2);
        con.boxTitle(x, y, w, h, "STATISTIK", act);
        if (h < 4 || w < 10) return;
        auto st = sim.getStats();
        int  cy2 = y + 1, iw = w - 2;

        char kz[128]; _snprintf_s(kz, 128, _TRUNCATE, "Bev:%d (K:%d)  Paare:%d  Haeuser:%d/%d",
            st.lebendig, st.kinder, st.paare, st.haeuser, st.lebendig);
        con.print(x + 1, cy2++, kz, C_ACCENT); if (cy2 >= y + h - 1) return;
        WORD tc = st.tempAktuell < 0 ? C_ACCENT : st.tempAktuell > 25 ? C_DANGER : C_OK;
        char ts[80]; _snprintf_s(ts, 80, _TRUNCATE, "Temp:%.1f%cC  Aelt:%dJ  Reichst:%s(%.0f)",
            st.tempAktuell, '\xF8', st.aelteste, st.reichsterName, st.reichste);
        con.print(x + 1, cy2++, ts, tc); if (cy2 >= y + h - 1) return;
        auto sb = [&](const char* l, float v, WORD c) {
            if (cy2 >= y + h - 1) return;
            int bw = iw - 13; if (bw < 4) bw = 4;
            int f = (int)(bw * v / 100);
            char lb[12]; _snprintf_s(lb, 12, _TRUNCATE, "%-8s[", l);
            con.print(x + 1, cy2, lb, C_DIM);
            for (int i = 0; i < bw; i++) con.put(x + 10 + i, cy2, i < f ? '\xDB' : '-', c);
            char pt[8]; _snprintf_s(pt, 8, _TRUNCATE, "]%3d%%", (int)v);
            con.print(x + 10 + bw, cy2, pt, c); cy2++;
            };
        sb("Glueck", st.avgGlueck, C_OK);
        sb("Gesundh", st.avgGesund, st.avgGesund < 40 ? C_DANGER : C_OK);
        sb("Hunger", st.avgHunger, st.avgHunger > 70 ? C_DANGER : C_WARN);

        if (cy2 < y + h - 3 && !sim.histBev.empty()) {
            if (cy2 < y + h - 1) { for (int i = 1; i < iw + 1; i++) con.put(x + i, cy2, '\xC4', C_BORDER); cy2++; }
            con.print(x + 1, cy2++, "Bev-Verlauf:", C_HEADER);
            if (cy2 < y + h - 1) {
                float mx = 0, mn = 9999;
                for (auto v : sim.histBev) { if (v > mx) mx = v; if (v < mn) mn = v; }
                if (mx - mn < 1) mx = mn + 1;
                int cw = iw - 2, n = (int)sim.histBev.size();
                for (int i = 0; i < cw && i < n; i++) {
                    float val = sim.histBev[(int)((float)i * n / cw)];
                    int   bH = (int)(4 * (val - mn) / (mx - mn));
                    char  c2 = bH >= 3 ? '\xDB' : bH >= 2 ? '\xB2' : bH >= 1 ? '\xB1' : '\xB0';
                    con.put(x + 1 + i, cy2, c2, val > sim.histBev[0] * 0.9f ? C_OK : C_WARN);
                }
                char mm[32]; _snprintf_s(mm, 32, _TRUNCATE, "[%.0f-%.0f]", mn, mx);
                con.print(x + 1 + cw - (int)strlen(mm), cy2, mm, C_DIM); cy2++;
            }
        }
        if (cy2 < y + h - 1) { for (int i = 1; i < iw + 1; i++) con.put(x + i, cy2, '\xC4', C_BORDER); cy2++; }
        if (cy2 < y + h - 2) con.print(x + 1, cy2++, "Berufe:", C_HEADER);
        int maxC = 1; for (auto& b : st.berufe) if (b.second > maxC) maxC = b.second;
        int bwJ = (std::min)(iw - 20, 20);
        for (auto& b : st.berufe) {
            if (cy2 >= y + h - 1) break;
            int  f = (int)((float)bwJ * b.second / maxC);
            char jb[12]; _snprintf_s(jb, 12, _TRUNCATE, "%-10s", jbStr(b.first));
            con.print(x + 1, cy2, jb, C_DIM);
            for (int i = 0; i < bwJ; i++) con.put(x + 11 + i, cy2, i < f ? '\xDB' : '-', C_CHART);
            char cnt[6]; _snprintf_s(cnt, 6, _TRUNCATE, "%3d", b.second);
            con.print(x + 11 + bwJ, cy2, cnt, C_CHART); cy2++;
        }
    }

    void drawMarkt(int y, int x, int h, int w) {
        bool act = (aktPanel == 3);
        con.boxTitle(x, y, w, h, "MARKT & PREISE", act);
        if (h < 4 || w < 10) return;
        int  cy2 = y + 1, iw = w - 2;
        int  saison = sim.zeit.saison();
        char ks[80]; _snprintf_s(ks, 80, _TRUNCATE, "Deals:%d  Angebote:%d",
            sim.handels, (int)sim.markt.liste.size());
        con.print(x + 1, cy2++, ks, C_ACCENT); if (cy2 >= y + h - 1) return;
        if (cy2 < y + h - 1) con.print(x + 1, cy2++, "Ware       Preis  Ang/Nfr", C_HEADER);
        static const IT wl[] = {
            IT::NahrIT, IT::Brot, IT::Wasser, IT::Holz, IT::Stein, IT::Erz, IT::Kohle,
            IT::Brett, IT::Werkzeug, IT::Stoff, IT::Medizin, IT::Bier, IT::Pelz, IT::Gold, IT::Moebel, IT::Waffe
        };
        for (IT it : wl) {
            if (cy2 >= y + h - 1) break;
            int   i = (int)it;
            float pr = sim.markt.getPreis(it, saison);
            float ang = sim.markt.angebot_s[i], nfr = sim.markt.nachfrage_s[i];
            float ratio = nfr / (ang > 0.01f ? ang : 0.01f);
            WORD  pc = ratio > 1.5f ? C_DANGER : ratio > 0.9f ? C_WARN : C_OK;
            char  arr = ratio > 1.3f ? '\x18' : ratio < 0.7f ? '\x19' : '-';
            int   bw = 6;
            int   af = (int)(bw * ang / (ang + nfr + 0.01f));
            char  row[64]; _snprintf_s(row, 64, _TRUNCATE, "%-9s%5.1fM %c ", itStr(it), pr, arr);
            con.print(x + 1, cy2, row, pc);
            int rx = x + 1 + (int)strlen(row);
            for (int b = 0; b < bw && rx + b < x + 1 + iw; b++)
                con.put(rx + b, cy2, b < af ? '\xDB' : '\xB0', b < af ? C_OK : C_DANGER);
            cy2++;
        }
    }

    void drawLog(int y, int x, int h, int w) {
        bool act = (aktPanel == 4);
        con.boxTitle(x, y, w, h, "LOG", act);
        if (h < 4 || w < 10) return;
        int  iw = w - 2, rows = h - 2;
        auto& lg = sim.logbuch;
        int  total = (int)lg.size();
        int  maxS = (std::max)(0, total - rows);
        if (logScroll > maxS) logScroll = maxS;
        int  start = total - rows - logScroll; if (start < 0) start = 0;
        for (int i = 0; i < rows && start + i < total; i++) {
            const LogEintrag& e = lg[start + i];
            WORD c = C_DIM;
            switch (e.typ) {
            case 1: c = C_DANGER;  break; case 2: c = C_OK;    break;
            case 3: c = C_WARN;    break; case 4: c = C_ACCENT; break;
            case 5: c = C_CHART;   break; case 6: c = C_TRADE;  break;
            }
            char line[256]; _snprintf_s(line, 255, _TRUNCATE, "%-*.*s", iw, iw, e.text.c_str());
            con.print(x + 1, y + 1 + i, line, c);
        }
        if (total > rows) {
            for (int i = 0; i < rows; i++) con.put(x + w - 1, y + 1 + i, '|', C_DIM);
            int pos = rows - 1 - (int)((float)(rows - 1) * logScroll / (float)(std::max)(1, maxS));
            con.put(x + w - 1, y + 1 + pos, '\xDB', C_ACCENT);
        }
        if (logScroll > 0) {
            char sc[12]; _snprintf_s(sc, 12, _TRUNCATE, "[-%d]", logScroll);
            con.print(x + w - 1 - (int)strlen(sc) - 1, y, sc, C_TITLE);
        }
    }

    void drawHint(int y, int /*W*/) {
        con.fillLine(y, ' ', C_HEADER);
        con.print(0, y, " TAB:Panel  Pfeile:Nav/Kamera  ENTER:Detail  F=KameraFolgt  1:Start  2:Pause  3:Reset  +/-:Speed  B:Buttons  S:Schritt  Q:Quit", C_HEADER);
    }

    bool input(int k) {
        if (k == 'q' || k == 'Q') return false;
        if (k == '\t') { btnFokus = false; aktPanel = (aktPanel + 1) % PANELS; return true; }
        if (k == 1020) { btnFokus = false; aktPanel = (aktPanel + PANELS - 1) % PANELS; return true; }
        if (k == 1000) { // UP
            if (btnFokus)                          btnSel = (std::max)(0, btnSel - 1);
            else if (aktPanel == 0 && !lstDetail)  lstSel = (std::max)(0, lstSel - 1);
            else if (aktPanel == 1)                camY--;
            else if (aktPanel == 4)                logScroll = (std::min)(logScroll + 1, (int)sim.logbuch.size());
            return true;
        }
        if (k == 1001) { // DOWN
            if (btnFokus) { btnSel = (std::min)(4, btnSel + 1); }
            else if (aktPanel == 0 && !lstDetail) {
                int al = 0; for (auto& p : sim.plist) if (p->lebt) al++;
                lstSel = (std::min)((std::max)(0, al - 1), lstSel + 1);
            }
            else if (aktPanel == 1) camY++;
            else if (aktPanel == 4) logScroll = (std::max)(0, logScroll - 1);
            return true;
        }
        if (k == 1002) { // LEFT
            if (btnFokus) btnSel = (std::max)(0, btnSel - 1);
            else if (aktPanel == 1) camX--;
            return true;
        }
        if (k == 1003) { // RIGHT
            if (btnFokus) btnSel = (std::min)(4, btnSel + 1);
            else if (aktPanel == 1) camX++;
            return true;
        }
        if (k == 1004) { // PgUp
            if (aktPanel == 0) lstSel = (std::max)(0, lstSel - 10);
            else if (aktPanel == 1) camY -= 5;
            else if (aktPanel == 4) logScroll = (std::min)(logScroll + 10, (int)sim.logbuch.size());
            return true;
        }
        if (k == 1005) { // PgDown
            if (aktPanel == 0) {
                int al = 0; for (auto& p : sim.plist) if (p->lebt) al++;
                lstSel = (std::min)((std::max)(0, al - 1), lstSel + 10);
            }
            else if (aktPanel == 1) camY += 5;
            else if (aktPanel == 4) logScroll = (std::max)(0, logScroll - 10);
            return true;
        }
        if (k == '\r') {
            if (btnFokus) {
                switch (btnSel) {
                case 0: simLaeuft = true;  if (sim.zeit.tick == 0) sim.init(80); break;
                case 1: simLaeuft = false; break;
                case 2: simLaeuft = false; sim.init(80); break;
                case 3: speed = (std::min)(10, speed + 1); break;
                case 4: speed = (std::max)(1, speed - 1); break;
                }
            }
            else if (aktPanel == 0) lstDetail = !lstDetail;
            return true;
        }
        if (k == '1') { simLaeuft = true;  if (sim.zeit.tick == 0) sim.init(80); return true; }
        if (k == '2') { simLaeuft = false; return true; }
        if (k == '3') { simLaeuft = false; sim.init(80); return true; }
        if (k == '+' || k == '=') { speed = (std::min)(10, speed + 1); return true; }
        if (k == '-' || k == '_') { speed = (std::max)(1, speed - 1); return true; }
        if (k == 'b' || k == 'B') { btnFokus = !btnFokus; return true; }
        if (k == 's' || k == 'S') { if (!simLaeuft) sim.schritt(); return true; }
        if (k == 'f' || k == 'F') { folgeSelected = !folgeSelected; return true; }
        if (k == 'c' || k == 'C') {
            Person* pp = getSelectedPerson();
            if (pp) { camX = pp->wx - (con.W / 3) / 2; camY = pp->wy - (con.H / 3) / 2; }
            return true;
        }
        if (k == 'w' || k == 'W') { camY -= 3; return true; }
        if (k == 'a' || k == 'A') { camX -= 3; return true; }
        if (k == 'x' || k == 'X' || k == 'y') { camY += 3; return true; }
        if (k == 'd' || k == 'D') { camX += 3; return true; }
        return true;
    }
};

int main() {
    TUI ui;
    ui.run();
    return 0;
}