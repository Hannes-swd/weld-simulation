// ================================================================
//  LEBENSSIMULATION v5.0 - DOERFER, EINSIEDLER, HAENDLERROUTEN
//  BUGFIXES v2:
//  - Gesundheit regeneriert natuerlich wenn satt+nicht durstig+sauber
//  - HP regeneriert passiv (+0.08/Tick bei Gesundheit>40)
//  - HP sinkt erst bei Gesundheit<15 (statt <25)
//  - Schlafen heilt auch Gesundheit (+1.5 * Hausbonus)
//  - Energie-Prioritaet: Durst > Schlaf(15) > Hunger > Schlaf(35)
//  - Nachts automatisch +8 Energie/Tick
//  - Schlafen gibt 50*bonus Energie (statt 35)
//  - Arbeitskosten halbiert
//  - Kälte-Fixes: Schaden reduziert, mehr Startkleidung
//  LNK1168: .exe noch offen -> Programm schliessen, dann neu kompilieren
// ================================================================
//  - PersonModus vor struct Person verschoben (Fix #1)
//  - drawTitle() implementiert (Fix #2)
//  - drawDoerfer() Klammerstruktur korrigiert (Fix #4)
//  - goto durch bool-Flag ersetzt (Fix #3)
//  - lebendigeBewohner() Null-Check fuer dorfIndexVonID (Fix #6)
//  - PANELS auf 5 korrigiert (Fix #7)
//  - itStr/itBasiswert OOB-Schutz (Fix #5)
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
static const WORD C_FRIEDHOF = 0x05;
static const WORD C_KIRCHE = 0x06;
static const WORD C_TURM = 0x07;
static const WORD C_MUEHLE = 0x0E;
static const WORD C_IMKEREI = 0x06;
static const WORD C_SALZWERK = 0x0F;
static const WORD C_DORF = 0x6F;
static const WORD C_DORF2 = 0x5F;
static const WORD C_EINSIEDLER = 0x0D;
static const WORD C_HAENDLER_REISE = 0x0B;

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
        if (x < 0 || y < 0 || x >= W || y >= H)return;
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
    void fillLine(int y2, char c, WORD a) { for (int x = 0; x < W; x++)put(x, y2, c, a); }
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
            int tl = (int)strlen(buf), tx = x + (w - tl) / 2; if (tx < x + 1)tx = x + 1;
            print(tx, y, buf, ta);
        }
    }
    void flip() {
        COORD sz = { (SHORT)W,(SHORT)H }, or2 = { 0,0 };
        SMALL_RECT reg = { 0,0,(SHORT)(W - 1),(SHORT)(H - 1) };
        WriteConsoleOutputA(hBuf, back.data(), sz, or2, &reg);
        std::swap(back, front);
    }
    int getKey() {
        DWORD cnt = 0; GetNumberOfConsoleInputEvents(hIn, &cnt);
        if (!cnt)return -1;
        INPUT_RECORD ir; DWORD rd = 0;
        ReadConsoleInputA(hIn, &ir, 1, &rd);
        if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) { refreshSize(); return -1; }
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown)return -1;
        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        char c = ir.Event.KeyEvent.uChar.AsciiChar;
        if (vk == VK_UP)    return 1000;
        if (vk == VK_DOWN)  return 1001;
        if (vk == VK_LEFT)  return 1002;
        if (vk == VK_RIGHT) return 1003;
        if (vk == VK_PRIOR) return 1004;
        if (vk == VK_NEXT)  return 1005;
        if (vk == VK_RETURN)return '\r';
        if (vk == VK_TAB) { bool sh = (GetKeyState(VK_SHIFT) & 0x8000) != 0; return sh ? 1020 : '\t'; }
        if (c > 0)return(unsigned char)c;
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
static int ri(int lo, int hi) { if (lo >= hi)return lo; return std::uniform_int_distribution<int>(lo, hi)(RNG); }
static float fc(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float noise2(int x, int y, int seed = 42) {
    unsigned int n = (unsigned)(x)+(unsigned)(y) * 57u + (unsigned)(seed) * 131u;
    n = (n << 13u) ^ n;
    unsigned int r = (n * (n * n * 15731u + 789221u) + 1376312589u);
    return 1.f - (float)(r & 0x7fffffffu) / 1073741824.f;
}
static float smoothNoise(float x, float y, int seed = 42) {
    int ix = (int)floor(x), iy = (int)floor(y);
    float fx = x - (float)ix, fy = y - (float)iy;
    float v00 = noise2(ix, iy, seed), v10 = noise2(ix + 1, iy, seed);
    float v01 = noise2(ix, iy + 1, seed), v11 = noise2(ix + 1, iy + 1, seed);
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
enum class Tile :uint8_t {
    Wasser = 0, Flachland, Wald, Huegel, Berg, Wueste, Fluss, See,
    Haus, Mine, Markt, Lager, Farm, Werkstatt, Strasse, Bruecke,
    Friedhof, Kirche, Wachturm,
    Muehle, Imkerei, Salzwerk, Topferei, Glashuette, Tavernen
};
static bool istWasserquelle(Tile t) { return t == Tile::Wasser || t == Tile::Fluss || t == Tile::See; }
static char tileChar(Tile t) {
    switch (t) {
    case Tile::Wasser:    return '~'; case Tile::Fluss:     return '\xF7';
    case Tile::See:       return '\xF0'; case Tile::Flachland: return '.';
    case Tile::Wald:      return 'T'; case Tile::Huegel:    return 'n';
    case Tile::Berg:      return '^'; case Tile::Wueste:    return ':';
    case Tile::Haus:      return 'H'; case Tile::Mine:      return 'M';
    case Tile::Markt:     return '$'; case Tile::Lager:     return 'L';
    case Tile::Farm:      return 'F'; case Tile::Werkstatt: return 'W';
    case Tile::Strasse:   return '\xC5'; case Tile::Bruecke:   return '=';
    case Tile::Friedhof:  return '\xEB'; case Tile::Kirche:    return '+';
    case Tile::Wachturm:  return 'I'; case Tile::Muehle:    return 'O';
    case Tile::Imkerei:   return 'B'; case Tile::Salzwerk:  return 'S';
    case Tile::Topferei:  return 'P'; case Tile::Glashuette:return 'G';
    case Tile::Tavernen:  return 'V';
    default:              return '?';
    }
}
static WORD tileColor(Tile t) {
    switch (t) {
    case Tile::Wasser:    return C_WATER; case Tile::Fluss:     return C_RIVER;
    case Tile::See:       return C_LAKE;  case Tile::Flachland: return C_GRASS;
    case Tile::Wald:      return C_TREE;  case Tile::Huegel:    return C_DIRT;
    case Tile::Berg:      return C_STONE; case Tile::Wueste:    return C_WARN;
    case Tile::Haus:      return C_HOUSE; case Tile::Mine:      return C_MINE;
    case Tile::Markt:     return C_ACCENT; case Tile::Lager:     return C_WARN;
    case Tile::Farm:      return C_OK;    case Tile::Werkstatt: return C_TRADE;
    case Tile::Strasse:   return C_ROAD;  case Tile::Bruecke:   return C_DIRT;
    case Tile::Friedhof:  return C_FRIEDHOF; case Tile::Kirche: return C_KIRCHE;
    case Tile::Wachturm:  return C_TURM; case Tile::Muehle:    return C_MUEHLE;
    case Tile::Imkerei:   return C_IMKEREI; case Tile::Salzwerk:return C_SALZWERK;
    case Tile::Topferei:  return C_DIRT;  case Tile::Glashuette:return C_ACCENT;
    case Tile::Tavernen:  return C_TRADE;
    default:              return C_DEFAULT;
    }
}
static bool isBebaubar(Tile t) { return t == Tile::Flachland || t == Tile::Wald || t == Tile::Huegel; }
static bool isPassierbar(Tile t) { return t != Tile::Wasser && t != Tile::Berg && t != Tile::Fluss && t != Tile::See; }

struct TileRessourcen { int holz = 0, stein = 0, erz = 0, gold = 0, kohle = 0, nahrung = 0, salz = 0, ton = 0; };
static TileRessourcen tileRess(Tile t) {
    TileRessourcen r;
    switch (t) {
    case Tile::Wald:      r.holz = 3; r.nahrung = 1; break;
    case Tile::Huegel:    r.stein = 2; r.erz = 1; r.ton = 1; break;
    case Tile::Berg:      r.stein = 4; r.erz = 3; r.kohle = 1; break;
    case Tile::Flachland: r.nahrung = 2; break;
    case Tile::Wueste:    r.stein = 1; r.gold = 1; r.salz = 2; break;
    default: break;
    }
    return r;
}

// ================================================================
//  CHUNK-SYSTEM
// ================================================================
static const int CHUNK_SZ = 16;
struct ChunkKey { int cx, cy; bool operator==(const ChunkKey& o)const { return cx == o.cx && cy == o.cy; } };
struct ChunkHash {
    size_t operator()(const ChunkKey& k)const {
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
    Welt(int seed = 12345) :worldSeed(seed) { chunks.reserve(512); }
    ChunkData& getChunk(int cx, int cy) {
        ChunkKey key{ cx,cy };
        auto it = chunks.find(key);
        if (it != chunks.end())return it->second;
        if (chunks.size() + 1 > (size_t)(chunks.bucket_count() * chunks.max_load_factor()))
            chunks.reserve(chunks.size() * 2 + 64);
        ChunkData tmp; generateChunk(tmp, cx, cy);
        auto res = chunks.emplace(key, tmp); return res.first->second;
    }
    void generateChunk(ChunkData& c, int cx, int cy) {
        memset(&c, 0, sizeof(c));
        for (int ly = 0; ly < CHUNK_SZ; ly++) {
            for (int lx = 0; lx < CHUNK_SZ; lx++) {
                int wx = cx * CHUNK_SZ + lx, wy = cy * CHUNK_SZ + ly;
                float h = fractalNoise(wx * 0.05f, wy * 0.05f, 4, worldSeed);
                float w2 = fractalNoise(wx * 0.03f, wy * 0.03f, 2, worldSeed + 777);
                float rn1 = fractalNoise(wx * 0.04f, wy * 0.04f, 3, worldSeed + 1111);
                float rn2 = fractalNoise(wx * 0.04f, wy * 0.04f, 3, worldSeed + 2222);
                bool istFluss = (std::abs(rn1) < 0.045f) && (h > -0.35f) && (w2 > -0.15f);
                bool istFluss2 = (std::abs(rn2) < 0.035f) && (h > -0.35f) && (w2 > -0.1f);
                float sn = fractalNoise(wx * 0.07f, wy * 0.07f, 2, worldSeed + 3333);
                bool istSee = (sn > 0.38f) && (h < -0.05f) && (w2 > -0.05f) && !istFluss && !istFluss2;
                Tile t;
                if (w2 < -0.2f)      t = Tile::Wasser;
                else if (h < -0.3f)  t = Tile::Flachland;
                else if (h < 0.1f)   t = Tile::Wald;
                else if (h < 0.4f)   t = Tile::Huegel;
                else if (h < 0.7f)   t = Tile::Berg;
                else              t = Tile::Wueste;
                float dist = (float)sqrt((double)(wx * wx + wy * wy));
                if (dist < 15 && t == Tile::Wasser) t = Tile::Flachland;
                if (dist < 8) t = Tile::Flachland;
                if (dist >= 10 && t != Tile::Wasser && t != Tile::Berg) {
                    if (istSee)              t = Tile::See;
                    if (istFluss || istFluss2) t = Tile::Fluss;
                }
                c.tiles[ly][lx] = t; c.bebaut[ly][lx] = false;
                if (istWasserquelle(t)) { c.ressLeft[ly][lx] = 9999; }
                else {
                    TileRessourcen r = tileRess(t);
                    c.ressLeft[ly][lx] = r.holz + r.stein + r.erz * 2 + r.kohle * 3 + r.gold * 5 + r.nahrung + r.salz * 2 + r.ton;
                    if (c.ressLeft[ly][lx] < 1) c.ressLeft[ly][lx] = 1;
                }
            }
        }
    }
    static void worldToChunk(int wx, int wy, int& cx, int& cy, int& lx, int& ly) {
        cx = (wx >= 0) ? wx / CHUNK_SZ : ((wx + 1) / CHUNK_SZ - 1);
        cy = (wy >= 0) ? wy / CHUNK_SZ : ((wy + 1) / CHUNK_SZ - 1);
        lx = wx - cx * CHUNK_SZ; ly = wy - cy * CHUNK_SZ;
        lx = ((lx % CHUNK_SZ) + CHUNK_SZ) % CHUNK_SZ; ly = ((ly % CHUNK_SZ) + CHUNK_SZ) % CHUNK_SZ;
        if (lx < 0)lx = 0; if (lx >= CHUNK_SZ)lx = CHUNK_SZ - 1;
        if (ly < 0)ly = 0; if (ly >= CHUNK_SZ)ly = CHUNK_SZ - 1;
    }
    Tile getTile(int wx, int wy) { int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly); return getChunk(cx, cy).tiles[ly][lx]; }
    void setTile(int wx, int wy, Tile t) { int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly); ChunkData& c = getChunk(cx, cy); c.tiles[ly][lx] = t; c.bebaut[ly][lx] = true; }
    int getRess(int wx, int wy) { int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly); return getChunk(cx, cy).ressLeft[ly][lx]; }
    void setRess(int wx, int wy, int val) { int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly); getChunk(cx, cy).ressLeft[ly][lx] = val; }
    int dekrementRess(int wx, int wy) { int cx, cy, lx, ly; worldToChunk(wx, wy, cx, cy, lx, ly); ChunkData& c = getChunk(cx, cy); if (c.ressLeft[ly][lx] > 0)c.ressLeft[ly][lx]--; return c.ressLeft[ly][lx]; }
    bool findNearest(int ox, int oy, Tile ziel, int maxR, int& fx, int& fy) {
        for (int r = 1; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (abs(dx) != r && abs(dy) != r)continue;
                    int wx = ox + dx, wy = oy + dy;
                    if (getTile(wx, wy) == ziel && getRess(wx, wy) > 0) { fx = wx; fy = wy; return true; }
                }
            }
        }
        return false;
    }
    bool findNearestWasser(int ox, int oy, int maxR, int& fx, int& fy) {
        for (int r = 1; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (abs(dx) != r && abs(dy) != r)continue;
                    int wx = ox + dx, wy = oy + dy;
                    if (istWasserquelle(getTile(wx, wy))) { fx = wx; fy = wy; return true; }
                }
            }
        }
        return false;
    }
    bool findFreiePlatz(int ox, int oy, int maxR, int& fx, int& fy) {
        for (int r = 0; r <= maxR; r++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (r > 0 && abs(dx) != r && abs(dy) != r)continue;
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
    Kohle, Gold, Ton, Salz, Wolle, Wachs, Harz,
    Brett, Werkzeug, Stoff, Pelz, Leder, Seil, Asche, Kalk, Ziegel, Teer,
    NahrIT, Brot, Bier, Honig, Kaese, Getrankl, Essig, Suppentopf,
    Moebel, Waffe, Medizin, HausIT,
    Kerze, Seife, Schmuck, Buch, Glas, Keramik, Pergament, Fett,
    KleidungEinfach, KleidungWarm, KleidungFein,
    Muenzen,
    COUNT
};
static const int IT_COUNT = (int)IT::COUNT;

static const char* itStr(IT t) {
    static const char* n[] = {
        "Holz","Stein","Erz","Wasser","Pflanze","Fleisch","Fisch",
        "Kohle","Gold","Ton","Salz","Wolle","Wachs","Harz",
        "Brett","Werkzeug","Stoff","Pelz","Leder","Seil","Asche","Kalk","Ziegel","Teer",
        "Nahrung","Brot","Bier","Honig","Kaese","Getraenkl","Essig","Suppe",
        "Moebel","Waffe","Medizin","HausMat",
        "Kerze","Seife","Schmuck","Buch","Glas","Keramik","Pergament","Fett",
        "Kleid.Einf","Kleid.Warm","Kleid.Fein",
        "Muenzen"
    };
    int i = (int)t;
    // FIX #5: Bounds-Check, COUNT selbst ist kein gueltiger Index
    if (i >= 0 && i < IT_COUNT)return n[i];
    return "?";
}
static float itBasiswert(IT t) {
    static const float bw[] = {
        3,2,7,1,2,5,4,   5,25,2,4,6,8,3,
        9,15,11,11,14,5,1,3,6,7,
        7,6,5,12,10,8,6,9,
        20,24,30,60,  8,10,35,25,18,8,15,4,
        8,18,40,
        1
    };
    int i = (int)t;
    if (i >= 0 && i < IT_COUNT)return bw[i];
    return 1;
}

struct Inventar {
    int items[IT_COUNT] = {};
    void  add(IT t, int n = 1) { int i = (int)t; if (i >= 0 && i < IT_COUNT)items[i] += n; }
    bool  hat(IT t, int n = 1)const { int i = (int)t; return i >= 0 && i < IT_COUNT && items[i] >= n; }
    bool  nimm(IT t, int n = 1) { if (!hat(t, n))return false; items[(int)t] -= n; return true; }
    int   anz(IT t)const { int i = (int)t; return(i >= 0 && i < IT_COUNT) ? items[i] : 0; }
    int   gesamt()const { int s = 0; for (int i = 0; i < IT_COUNT; i++)s += items[i]; return s; }
    float wert()const { float w = 0; for (int i = 0; i < IT_COUNT; i++)w += items[i] * itBasiswert((IT)i); return w; }
    bool hatKleidung()const { return hat(IT::KleidungEinfach) || hat(IT::KleidungWarm) || hat(IT::KleidungFein) || hat(IT::Stoff) || hat(IT::Pelz); }
    bool hatWarmeKleidung()const { return hat(IT::KleidungWarm) || hat(IT::KleidungFein) || hat(IT::Pelz); }
};

// ================================================================
//  KLEIDUNGS-VERSCHLEISS
// ================================================================
static const int MAX_KLEIDUNG = 3;
struct KleidungsSlot { int haltbarkeit = 0; };

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
    Seilen, Toepfern, Glasmachen, Muellern, Imkern, Salzsieden, Buchbinden,
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
        "Seilen","Toepfern","Glasmachen","Muellern","Imkern","Salzsieden","Buchbinden",
        "?"
    };
    int i = (int)s; return(i >= 0 && i < SK_COUNT) ? n[i] : "?";
}

// ================================================================
//  BERUF
// ================================================================
enum class JB {
    Keiner, Holzfaeller, Bergmann, Jaeger, Fischer, Koch,
    Schmied, Tischler, Bauer, Haendler, Lehrer, Wache, Arzt,
    Brauer, Gerber, Forscher, Bergbaumeister,
    Seiler, Toepfer, Glasmacher, Muller, Imker, Salzsieder, Buchbinder, Wirt
};
static const char* jbStr(JB j) {
    static const char* n[] = {
        "Keiner","Holzfaeller","Bergmann","Jaeger","Fischer","Koch",
        "Schmied","Tischler","Bauer","Haendler","Lehrer","Wache","Arzt",
        "Brauer","Gerber","Forscher","Bergbaumeister",
        "Seiler","Toepfer","Glasmacher","Muller","Imker","Salzsieder","Buchbinder","Wirt"
    };
    int i = (int)j; return(i >= 0 && i < 25) ? n[i] : "?";
}
static float berufsMinSkill(JB j) {
    switch (j) {
    case JB::Holzfaeller:    return 10; case JB::Bergmann:       return 15;
    case JB::Jaeger:         return 15; case JB::Fischer:        return 10;
    case JB::Koch:           return 20; case JB::Schmied:        return 30;
    case JB::Tischler:       return 25; case JB::Bauer:          return 10;
    case JB::Haendler:       return 20; case JB::Lehrer:         return 40;
    case JB::Wache:          return 25; case JB::Arzt:           return 35;
    case JB::Brauer:         return 25; case JB::Gerber:         return 20;
    case JB::Forscher:       return 50; case JB::Bergbaumeister: return 60;
    case JB::Seiler:         return 18; case JB::Toepfer:        return 20;
    case JB::Glasmacher:     return 35; case JB::Muller:         return 22;
    case JB::Imker:          return 15; case JB::Salzsieder:     return 20;
    case JB::Buchbinder:     return 40; case JB::Wirt:           return 25;
    default: return 0;
    }
}
static SK berufsSkill(JB j) {
    switch (j) {
    case JB::Holzfaeller:    return SK::Holzfaellen;  case JB::Bergmann:       return SK::Bergbau;
    case JB::Jaeger:         return SK::Jagd;          case JB::Fischer:        return SK::Fischen;
    case JB::Koch:           return SK::Kochen;        case JB::Schmied:        return SK::Schmieden;
    case JB::Tischler:       return SK::Tischlern;     case JB::Bauer:          return SK::Holzfaellen;
    case JB::Haendler:       return SK::Handel;        case JB::Lehrer:         return SK::Lehren;
    case JB::Wache:          return SK::Nahkampf;      case JB::Arzt:           return SK::Heilen;
    case JB::Brauer:         return SK::Brauen;        case JB::Gerber:         return SK::Gerben;
    case JB::Forscher:       return SK::Forschen;      case JB::Bergbaumeister: return SK::Bergbaumeister;
    case JB::Seiler:         return SK::Seilen;        case JB::Toepfer:        return SK::Toepfern;
    case JB::Glasmacher:     return SK::Glasmachen;    case JB::Muller:         return SK::Muellern;
    case JB::Imker:          return SK::Imkern;        case JB::Salzsieder:     return SK::Salzsieden;
    case JB::Buchbinder:     return SK::Buchbinden;    case JB::Wirt:           return SK::Brauen;
    default: return SK::Holzfaellen;
    }
}

// ================================================================
//  GEBAEUDE
// ================================================================
enum class GebTyp {
    Haus, Mine, Werkstatt, Farm, Lager, Marktstand, Friedhof, Kirche, Wachturm,
    Muehle, Imkerei, Salzwerk, Topferei, Glashuette, Taverne
};
struct Gebaeude {
    GebTyp typ; int wx, wy, eigentuemerID;
    int level = 1, haltbarkeit = 100, lagerkapazitaet = 50;
    Inventar lager;
    char name[32] = "";
    int isolierung = 0, heizung = 0, holzVorrat = 0;
    int gaestezahl = 0; float einnahmen = 0;
};

// ================================================================
//  WETTER
// ================================================================
enum class Wetter { Sonnig = 0, Bewoelkt, Regen, Gewitter, Schnee, Nebel };
static const char* wetterStr(Wetter w) { static const char* n[] = { "Sonnig","Bewoelkt","Regen","Gewitter","Schnee","Nebel" }; return n[(int)w]; }
struct WetterSystem {
    Wetter aktuell = Wetter::Sonnig; int dauer = 48; float temp_mod = 0;
    void tick(int saison) {
        if (--dauer <= 0)wechsel(saison);
        switch (aktuell) {
        case Wetter::Regen:    temp_mod = -3.f; break; case Wetter::Gewitter:temp_mod = -5.f; break;
        case Wetter::Schnee:   temp_mod = -8.f; break; case Wetter::Nebel:   temp_mod = -2.f; break;
        case Wetter::Sonnig:   temp_mod = 4.f; break; default:temp_mod = 0.f; break;
        }
    }
    void wechsel(int saison) {
        float r = rf();
        if (saison == 3) {
            if (r < 0.35f)aktuell = Wetter::Schnee; else if (r < 0.55f)aktuell = Wetter::Bewoelkt;
            else if (r < 0.70f)aktuell = Wetter::Nebel; else if (r < 0.85f)aktuell = Wetter::Regen;
            else aktuell = Wetter::Sonnig;
        }
        else if (saison == 1) {
            if (r < 0.45f)aktuell = Wetter::Sonnig; else if (r < 0.65f)aktuell = Wetter::Bewoelkt;
            else if (r < 0.80f)aktuell = Wetter::Gewitter; else aktuell = Wetter::Regen;
        }
        else {
            if (r < 0.30f)aktuell = Wetter::Sonnig; else if (r < 0.55f)aktuell = Wetter::Bewoelkt;
            else if (r < 0.75f)aktuell = Wetter::Regen; else if (r < 0.85f)aktuell = Wetter::Gewitter;
            else aktuell = Wetter::Nebel;
        }
        dauer = ri(12, 96);
    }
    float gesundheitsMod()const {
        switch (aktuell) {
        case Wetter::Regen:return-0.02f; case Wetter::Gewitter:return-0.06f;
        case Wetter::Schnee:return-0.06f; default:return 0.f;
        }
    }
    bool verlangsamt()const { return aktuell == Wetter::Schnee || aktuell == Wetter::Gewitter || aktuell == Wetter::Nebel; }
    bool gibtRegen()const { return aktuell == Wetter::Regen || aktuell == Wetter::Gewitter; }
    char symbol()const {
        switch (aktuell) {
        case Wetter::Sonnig:return'*'; case Wetter::Bewoelkt:return'o';
        case Wetter::Regen:return','; case Wetter::Gewitter:return'!';
        case Wetter::Schnee:return'+'; case Wetter::Nebel:return'-'; default:return'?';
        }
    }
};

// ================================================================
//  MARKT
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
            angebot_s[i] = 5; nachfrage_s[i] = 3;
        }
    }
    float getPreis(IT t, int saison = 1)const {
        int i = (int)t; if (i < 0 || i >= IT_COUNT)return 1;
        float ratio = nachfrage_s[i] / (angebot_s[i] > 0.1f ? angebot_s[i] : 0.1f);
        float dyn = itBasiswert(t) * fc(ratio, 0.1f, 6.f);
        float sm = 1.f;
        if (t == IT::NahrIT || t == IT::Brot || t == IT::Suppentopf) sm = (saison == 3) ? 2.1f : (saison == 1) ? 0.8f : 1.f;
        if (t == IT::KleidungWarm || t == IT::KleidungFein || t == IT::Pelz) sm = (saison == 3) ? 2.5f : (saison == 0) ? 0.6f : 1.f;
        if (t == IT::KleidungEinfach || t == IT::Stoff) sm = (saison == 3) ? 1.8f : 1.f;
        if (t == IT::Medizin && saison == 3) sm = 1.8f;
        if (t == IT::Kerze) sm = (saison == 3) ? 1.6f : 1.f;
        if (t == IT::Holz && saison == 3)   sm = 1.9f;
        if (t == IT::Salz) sm = (saison == 3) ? 1.4f : 1.f;
        return fc(dyn * sm, itBasiswert(t) * 0.15f, itBasiswert(t) * 8.f);
    }
    void addAngebot(int id, IT it, int m, float p, int wx = 0, int wy = 0) {
        liste.push_back({ id,it,m,p,wx,wy });
        if ((int)it < IT_COUNT)angebot_s[(int)it] += m;
    }
    void registriereNachfrage(IT t, float n = 1) { if ((int)t < IT_COUNT)nachfrage_s[(int)t] += n; }
    void tick(int saison) {
        for (int i = 0; i < IT_COUNT; i++) {
            float ziel = getPreis((IT)i, saison);
            marktpreis[i] = marktpreis[i] * 0.9f + ziel * 0.1f;
            angebot_s[i] = angebot_s[i] * 0.93f + 0.2f;
            nachfrage_s[i] = nachfrage_s[i] * 0.93f + 0.1f;
        }
        liste.erase(std::remove_if(liste.begin(), liste.end(), [](const Angebot& a) {return a.menge <= 0; }), liste.end());
        if (liste.size() > 1000)liste.erase(liste.begin(), liste.begin() + 200);
    }
};

// ================================================================
//  SPIELZEIT
// ================================================================
struct Spielzeit {
    int tick = 0;
    int stunde()const { return tick % 24; } int tag()const { return(tick / 24) % 30 + 1; }
    int monat()const { return(tick / 24 / 30) % 12 + 1; } int jahr()const { return tick / 24 / 30 / 12 + 1; }
    int saison()const { return(tick / 720) % 4; }
    void str(char* buf, int sz)const {
        static const char* sN[] = { "Fruehling","Sommer","Herbst","Winter" };
        static const char* mN[] = { "Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez" };
        _snprintf_s(buf, sz, _TRUNCATE, "%02d:00  %d. %s %s  Jahr %d", stunde(), tag(), mN[monat() - 1], sN[saison()], jahr());
    }
    float temperatur()const { static const float base[] = { 10,24,8,-6 }; return base[saison()] + (float)sin((stunde() - 6) * 3.14159f / 12) * 5; }
};

// ================================================================
//  FIX #1: PersonModus VOR struct Person deklariert
// ================================================================
enum class PersonModus {
    Normal,
    Einsiedler,
    Reisender,
    Auswanderer
};

// ================================================================
//  PERSON
// ================================================================
static int gNextID = 1;
struct Person {
    int  id; char name[32]; int alter; bool lebt, maennl;
    int  wx, wy, zielX, zielY; bool hatZiel = false;
    float hp, energie, staerke, geschick, intel, weisheit, kreativ, lernfg, mut, empathie, fleiss;
    float hunger, durst, glueck, stress, gesundheit;
    float skills[SK_COUNT] = {};
    Inventar inv;
    JB beruf = JB::Keiner;
    float ruf = 0, muenzen = 10;
    int partnerID = -1, mutterID = -1, vaterID = -1;
    std::vector<int> kinderIDs, eigeneGebaeude;
    int hausGebIdx = -1;
    char aktion[80] = "---", todesursache[64] = "";
    int handelAnzahl = 0;

    int  dorfID = -1;
    PersonModus modus = PersonModus::Normal;
    int  reisezielDorfID = -1;
    int  reiseZaehler = 0;
    bool istEinsiedler = false;

    int kleidHaltbarkeit[MAX_KLEIDUNG] = { 0,0,0 };
    int steuerZaehler = 0;
    int werkzeugHaltbarkeit = 0;
    float sauberkeit = 80.f;
    float bildung = 0.f;

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
        hunger = rf() * 10; durst = rf() * 10;
        glueck = rf() * 40 + 40; stress = rf() * 20; gesundheit = rf() * 30 + 70;
        for (int i = 0; i < SK_COUNT; i++) skills[i] = rf() * 8 + 1;
        muenzen = rf() * 20 + 8;
        inv.add(IT::NahrIT, ri(5, 10));
        inv.add(IT::Wasser, ri(25, 40));
        inv.add(IT::KleidungEinfach, 1);
        kleidHaltbarkeit[0] = ri(40, 80);
        werkzeugHaltbarkeit = ri(20, 50);
        sauberkeit = rf() * 40 + 50;
        bildung = rf() * 20;
    }
    int alterJ()const { return alter / 12; }
    bool istKind()const { return alterJ() < 14; }
    bool istGreis()const { return alterJ() > 68; }
    float skill(SK s)const { int i = (int)s; return(i >= 0 && i < SK_COUNT) ? skills[i] : 0; }
    void verbSkill(SK s, float amt) {
        int i = (int)s; if (i < 0 || i >= SK_COUNT)return;
        float b = 1.f + (intel / 100.f) * .6f + (lernfg / 100.f) * .4f + (bildung / 200.f) * .3f;
        skills[i] = fc(skills[i] + amt * b, 0, 100);
    }
    JB berechneberuf()const {
        if (istKind())return JB::Keiner;
        static const JB reihenfolge[] = {
            JB::Bergbaumeister,JB::Forscher,JB::Buchbinder,JB::Arzt,JB::Lehrer,
            JB::Glasmacher,JB::Schmied,JB::Brauer,JB::Salzsieder,JB::Wirt,
            JB::Gerber,JB::Seiler,JB::Muller,JB::Toepfer,JB::Imker,
            JB::Wache,JB::Haendler,JB::Tischler,
            JB::Koch,JB::Fischer,JB::Jaeger,JB::Bergmann,
            JB::Holzfaeller,JB::Bauer
        };
        for (auto j : reihenfolge) { SK s = berufsSkill(j); if (skill(s) >= berufsMinSkill(j))return j; }
        return JB::Bauer;
    }
    void bewegeZuZiel() {
        if (!hatZiel || (wx == zielX && wy == zielY)) { hatZiel = false; return; }
        int dx = zielX - wx, dy = zielY - wy;
        if (abs(dx) + abs(dy) > 200) { hatZiel = false; return; }
        if (abs(dx) > abs(dy)) { wx += (dx > 0 ? 1 : -1); }
        else { wy += (dy > 0 ? 1 : -1); }
        wx = std::max(-500, std::min(500, wx)); wy = std::max(-500, std::min(500, wy));
    }
    int distTo(int tx, int ty)const { return abs(wx - tx) + abs(wy - ty); }
    bool hatWarmeKleidung()const {
        return (inv.hat(IT::KleidungWarm) && kleidHaltbarkeit[1] > 0) ||
            (inv.hat(IT::KleidungFein) && kleidHaltbarkeit[2] > 0) ||
            inv.hat(IT::Pelz) || inv.hat(IT::Leder);
    }
    bool hatIrgendKleidung()const {
        return (inv.hat(IT::KleidungEinfach) && kleidHaltbarkeit[0] > 0) || hatWarmeKleidung() || inv.hat(IT::Stoff);
    }
    bool hatFunktionsfaehigesWerkzeug()const { return inv.hat(IT::Werkzeug) && werkzeugHaltbarkeit > 0; }
};

// ================================================================
//  LOG
// ================================================================
struct LogEintrag { std::string text; int typ; };

// ================================================================
//  DORF-SYSTEM
// ================================================================
static int gNextDorfID = 1;

enum class DorfTyp {
    Weiler,
    Dorf,
    Grossdorf,
    Stadt
};

struct Dorf {
    int  id;
    char name[32];
    int  wx, wy;
    int  radius = 8;
    DorfTyp typ = DorfTyp::Weiler;
    std::vector<int> bewohnerIDs;
    std::vector<int> gebaeudIdxs;
    Inventar gemeinschaftslager;
    float wohlstand = 0;
    bool hatMarkt = false, hatFarm = false, hatMine = false, hatWerkstatt = false, hatTaverne = false;
    int  gruendungsTick = 0;
    char spezialitat[32] = "";

    static const char* typStr(DorfTyp t) {
        switch (t) {
        case DorfTyp::Weiler:return"Weiler"; case DorfTyp::Dorf:return"Dorf";
        case DorfTyp::Grossdorf:return"Grossdorf"; case DorfTyp::Stadt:return"Stadt"; default:return"?";
        }
    }
    void aktualisiereTyp() {
        int n = (int)bewohnerIDs.size();
        if (n >= 41)      typ = DorfTyp::Stadt;
        else if (n >= 16) typ = DorfTyp::Grossdorf;
        else if (n >= 5)  typ = DorfTyp::Dorf;
        else           typ = DorfTyp::Weiler;
        radius = 6 + n / 3;
        if (radius > 30)radius = 30;
    }
    WORD farbe()const {
        switch (typ) {
        case DorfTyp::Weiler:    return 0x08;
        case DorfTyp::Dorf:      return 0x0A;
        case DorfTyp::Grossdorf: return 0x0E;
        case DorfTyp::Stadt:     return 0x0B;
        default:                 return 0x07;
        }
    }
    char symbol()const {
        switch (typ) {
        case DorfTyp::Weiler:    return 'w';
        case DorfTyp::Dorf:      return 'D';
        case DorfTyp::Grossdorf: return 'G';
        case DorfTyp::Stadt:     return '#';
        default:                 return '?';
        }
    }
};

// ================================================================
//  NAMES
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

// ================================================================
//  SIMULATION
// ================================================================
struct Simulation {
    std::vector<std::shared_ptr<Person>> plist, neuePersonen;
    std::vector<Gebaeude> gebaeude;
    std::vector<Dorf> doerfer;
    Welt welt; Markt markt; Spielzeit zeit; WetterSystem wetter;
    std::deque<LogEintrag> logbuch;
    int geburten = 0, tode = 0, handels = 0, gebaeudeAnz = 0, raeuberfaelle = 0;
    std::deque<float> histBev;
    struct Rekorde { int maxBev = 0, maxAlter = 0; float maxReichtum = 0; char reichsterName[32] = ""; } rekorde;
    struct Grabstein { char name[32]; int alterJ; char ursache[64]; int tick; };
    std::deque<Grabstein> friedhof;
    int handelsvolumen[IT_COUNT] = {};

    Simulation() :welt(42) {}

    void addLog(const char* msg, int typ = 0) {
        char buf[256];
        _snprintf_s(buf, 255, _TRUNCATE, "[J%d %c %d.%02d] %s", zeit.jahr(), "SSHW"[zeit.saison()], zeit.tag(), zeit.stunde(), msg);
        logbuch.push_back({ buf,typ });
        if (logbuch.size() > 600)logbuch.pop_front();
    }
    const char* zname(bool m) { return m ? mNam[ri(0, 33)] : wNam[ri(0, 33)]; }

    // ================================================================
    //  DORF-VERWALTUNG
    // ================================================================
    static const char* dorfname() {
        static const char* n[] = {
            "Birkenau","Steinbach","Waldheim","Flusstal","Eichdorf",
            "Krottenburg","Hammerhof","Silberbach","Tannengrund","Moorburg",
            "Fichtenau","Salzburg","Kaltenbach","Neumark","Altenfeld",
            "Wiesenbach","Bergdorf","Seeheim","Haselbach","Rotental"
        };
        return n[ri(0, 19)];
    }

    int gruendeDorf(int wx, int wy, int gruenderID = -1) {
        Dorf d;
        d.id = gNextDorfID++;
        strncpy_s(d.name, 32, dorfname(), _TRUNCATE);
        d.wx = wx; d.wy = wy;
        d.typ = DorfTyp::Weiler;
        d.gruendungsTick = zeit.tick;
        d.radius = 6;
        Tile t = welt.getTile(wx, wy);
        if (t == Tile::Wald)       strncpy_s(d.spezialitat, 32, "Holz", _TRUNCATE);
        else if (t == Tile::Huegel)strncpy_s(d.spezialitat, 32, "Bergbau", _TRUNCATE);
        else if (t == Tile::Berg)  strncpy_s(d.spezialitat, 32, "Erz", _TRUNCATE);
        else if (t == Tile::Fluss || t == Tile::See)strncpy_s(d.spezialitat, 32, "Fischerei", _TRUNCATE);
        else                    strncpy_s(d.spezialitat, 32, "Landwirtschaft", _TRUNCATE);
        int idx = (int)doerfer.size();
        doerfer.push_back(d);
        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "Neuer %s: %s (%s) bei (%d,%d)",
            Dorf::typStr(d.typ), d.name, d.spezialitat, wx, wy);
        addLog(buf, 2);
        return idx;
    }

    void trittDorfBei(Person& p, int dorfIdx) {
        if (p.dorfID >= 0) {
            for (auto& d : doerfer) {
                if (d.id == p.dorfID) {
                    d.bewohnerIDs.erase(std::remove(d.bewohnerIDs.begin(), d.bewohnerIDs.end(), p.id), d.bewohnerIDs.end());
                    break;
                }
            }
        }
        if (dorfIdx < 0 || dorfIdx >= (int)doerfer.size())return;
        Dorf& d = doerfer[dorfIdx];
        d.bewohnerIDs.push_back(p.id);
        p.dorfID = d.id;
        p.modus = PersonModus::Normal;
        p.reisezielDorfID = -1;
        d.aktualisiereTyp();
        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s tritt %s bei", p.name, d.name);
        addLog(buf, 0);
    }

    int dorfIndexVonID(int id)const {
        for (int i = 0; i < (int)doerfer.size(); i++)if (doerfer[i].id == id)return i;
        return -1;
    }

    int naechstesDorf(int wx, int wy, int ausschliessen = -1)const {
        int best = -1; int bestDist = 999999;
        for (int i = 0; i < (int)doerfer.size(); i++) {
            if (doerfer[i].id == ausschliessen)continue;
            int d = abs(doerfer[i].wx - wx) + abs(doerfer[i].wy - wy);
            if (d < bestDist) { bestDist = d; best = i; }
        }
        return best;
    }

    int lebendigeBewohner(int dorfIdx)const {
        // FIX #6: Gueltigkeit pruefen
        if (dorfIdx < 0 || dorfIdx >= (int)doerfer.size())return 0;
        int n = 0;
        for (int id : doerfer[dorfIdx].bewohnerIDs) {
            for (auto& p : plist) { if (p && p->lebt && p->id == id) { n++; break; } }
        }
        return n;
    }

    void dorfTick() {
        if (zeit.tick % 24 != 0)return;

        for (int di = 0; di < (int)doerfer.size(); di++) {
            Dorf& d = doerfer[di];
            int n = lebendigeBewohner(di);
            d.aktualisiereTyp();

            if (zeit.tick % 120 == di % 120) {
                for (int pid : d.bewohnerIDs) {
                    for (auto& p : plist) {
                        if (!p || !p->lebt || p->id != pid)continue;
                        auto abgeben = [&](IT it, int schw) {
                            int cnt = p->inv.anz(it);
                            if (cnt > schw) {
                                int ab = cnt - schw;
                                p->inv.nimm(it, ab);
                                d.gemeinschaftslager.add(it, ab);
                            }
                            };
                        abgeben(IT::NahrIT, 12); abgeben(IT::Holz, 10);
                        abgeben(IT::Stein, 8);   abgeben(IT::Brot, 6);
                        abgeben(IT::Wasser, 20);
                    }
                }
            }

            if (!d.hatMarkt && n >= 5 && (int)gebaeude.size() < 500) {
                bool hatNahenMarkt = false;
                for (auto& g : gebaeude) {
                    if (g.typ == GebTyp::Marktstand &&
                        abs(g.wx - d.wx) + abs(g.wy - d.wy) < 20) {
                        hatNahenMarkt = true; break;
                    }
                }
                if (!hatNahenMarkt) {
                    int fx, fy;
                    if (welt.findFreiePlatz(d.wx, d.wy, 8, fx, fy)) {
                        Gebaeude g; g.typ = GebTyp::Marktstand;
                        g.wx = fx; g.wy = fy; g.eigentuemerID = -1; g.level = 1;
                        _snprintf_s(g.name, 32, _TRUNCATE, "Markt-%s", d.name);
                        gebaeude.push_back(g);
                        welt.setTile(fx, fy, Tile::Markt);
                        d.hatMarkt = true; d.gebaeudIdxs.push_back((int)gebaeude.size() - 1);
                        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s errichtet Markt", d.name);
                        addLog(buf, 5);
                    }
                }
            }

            d.wohlstand = 0;
            for (int pid : d.bewohnerIDs) {
                for (auto& p : plist) {
                    if (p && p->lebt && p->id == pid) { d.wohlstand += p->muenzen + p->inv.wert() * 0.1f; break; }
                }
            }
            d.wohlstand += d.gemeinschaftslager.wert();

            if (n >= 4 && (int)doerfer.size() >= 2 && zeit.tick % 360 == (di * 37) % 360) {
                for (int pid : d.bewohnerIDs) {
                    for (auto& p : plist) {
                        if (!p || !p->lebt || p->id != pid)continue;
                        if (p->modus != PersonModus::Normal)continue;
                        if (p->beruf != JB::Haendler && p->beruf != JB::Bauer)continue;
                        if (p->inv.gesamt() < 10)continue;
                        int zielIdx = naechstesDorf(d.wx, d.wy, d.id);
                        if (zielIdx < 0)continue;
                        Dorf& ziel = doerfer[zielIdx];
                        p->modus = PersonModus::Reisender;
                        p->reisezielDorfID = ziel.id;
                        p->reiseZaehler = 0;
                        p->zielX = ziel.wx + ri(-3, 3);
                        p->zielY = ziel.wy + ri(-3, 3);
                        p->hatZiel = true;
                        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE,
                            "%s reist von %s nach %s", p->name, d.name, ziel.name);
                        addLog(buf, 6); break;
                    }
                    break;
                }
            }
        }

        // FIX #3: goto durch bool-Flag ersetzt
        bool mergeGeschehen = false;
        for (int i = 0; i < (int)doerfer.size() && !mergeGeschehen; i++) {
            for (int j = i + 1; j < (int)doerfer.size() && !mergeGeschehen; j++) {
                int dist = abs(doerfer[i].wx - doerfer[j].wx) + abs(doerfer[i].wy - doerfer[j].wy);
                if (dist < 15 && lebendigeBewohner(i) + lebendigeBewohner(j) >= 3) {
                    int gros = lebendigeBewohner(i) >= lebendigeBewohner(j) ? i : j;
                    int klein = gros == i ? j : i;
                    Dorf& dg = doerfer[gros];
                    Dorf& dk = doerfer[klein];
                    for (int pid : dk.bewohnerIDs) {
                        for (auto& p : plist) {
                            if (p && p->lebt && p->id == pid) { p->dorfID = dg.id; dg.bewohnerIDs.push_back(pid); break; }
                        }
                    }
                    for (int k = 0; k < IT_COUNT; k++)
                        dg.gemeinschaftslager.items[k] += dk.gemeinschaftslager.items[k];
                    char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s und %s vereinigen sich zu %s",
                        dk.name, dg.name, dg.name);
                    addLog(buf, 4);
                    dk.bewohnerIDs.clear();
                    doerfer.erase(doerfer.begin() + klein);
                    mergeGeschehen = true;
                }
            }
        }
    }

    void reisendenTick(Person& p) {
        if (p.modus != PersonModus::Reisender)return;
        p.reiseZaehler++;

        int zielIdx = dorfIndexVonID(p.reisezielDorfID);
        if (zielIdx < 0) { p.modus = PersonModus::Normal; return; }
        Dorf& ziel = doerfer[zielIdx];

        int dist = p.distTo(ziel.wx, ziel.wy);
        if (dist <= ziel.radius) {
            handelZwischenDoerfern(p, ziel);
            int heimIdx = dorfIndexVonID(p.dorfID);
            if (heimIdx >= 0 && lebendigeBewohner(zielIdx) > lebendigeBewohner(heimIdx) * 2) {
                trittDorfBei(p, zielIdx);
                char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s bleibt in %s", p.name, ziel.name);
                addLog(buf, 0);
            }
            else {
                if (heimIdx >= 0) {
                    p.zielX = doerfer[heimIdx].wx + ri(-3, 3);
                    p.zielY = doerfer[heimIdx].wy + ri(-3, 3);
                    p.hatZiel = true;
                }
                p.modus = PersonModus::Normal;
                p.reisezielDorfID = -1;
            }
        }
        else if (p.reiseZaehler > 500) {
            p.modus = PersonModus::Einsiedler;
            p.istEinsiedler = true;
            p.reisezielDorfID = -1;
        }
    }

    void handelZwischenDoerfern(Person& p, Dorf& d) {
        int saison = zeit.saison();
        static const IT handelswaren[] = {
            IT::NahrIT,IT::Brot,IT::Holz,IT::Stein,IT::Erz,IT::Stoff,
            IT::Werkzeug,IT::Salz,IT::Bier,IT::Medizin,IT::Kerze
        };
        for (IT it : handelswaren) {
            if (p.inv.anz(it) > 3) {
                int menge = p.inv.anz(it) / 2;
                d.gemeinschaftslager.add(it, menge);
                p.inv.nimm(it, menge);
                float gewinn = markt.getPreis(it, saison) * menge * 0.8f;
                p.muenzen += gewinn;
            }
            if (d.gemeinschaftslager.hat(it, 3) && !p.inv.hat(it, 5)) {
                int menge = 2;
                d.gemeinschaftslager.nimm(it, menge);
                p.inv.add(it, menge);
                float kosten = markt.getPreis(it, saison) * menge * 0.9f;
                p.muenzen -= std::min(kosten, p.muenzen);
            }
        }
        p.verbSkill(SK::Handel, 1.f);
        handels++;
        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s handelt in %s", p.name, d.name);
        addLog(buf, 6);
    }

    void einsiedlerTick(Person& p) {
        if (!p.istEinsiedler && p.modus != PersonModus::Einsiedler)return;
        if (rf() < 0.003f) {
            Tile t = welt.getTile(p.wx, p.wy);
            if (isBebaubar(t) && p.inv.hat(IT::Brett, 6) && p.inv.hat(IT::NahrIT, 5)) {
                int idx = gruendeDorf(p.wx, p.wy, p.id);
                trittDorfBei(p, idx);
                p.istEinsiedler = false;
                char buf[80]; _snprintf_s(buf, 80, _TRUNCATE,
                    "%s gruendet neues Dorf: %s", p.name, doerfer[idx].name);
                addLog(buf, 3);
            }
        }
        if (rf() < 0.002f) {
            int nahe = naechstesDorf(p.wx, p.wy);
            if (nahe >= 0) {
                int dist = abs(doerfer[nahe].wx - p.wx) + abs(doerfer[nahe].wy - p.wy);
                if (dist < 50) {
                    trittDorfBei(p, nahe);
                    p.istEinsiedler = false;
                    char buf[80]; _snprintf_s(buf, 80, _TRUNCATE,
                        "%s schliesst sich %s an", p.name, doerfer[nahe].name);
                    addLog(buf, 0);
                }
            }
        }
    }

    void auswanderungsTick() {
        if (zeit.tick % 480 != 7)return;
        for (int di = 0; di < (int)doerfer.size(); di++) {
            int n = lebendigeBewohner(di);
            if (n < 8)continue;
            Dorf& d = doerfer[di];
            bool ueberbevoelkert = (n > 30);
            bool zuArm = (d.wohlstand / n < 5.f);
            if (!ueberbevoelkert && !zuArm)continue;
            int anzahl = ri(1, 3);
            int ausgew = 0;
            for (int pid : d.bewohnerIDs) {
                if (ausgew >= anzahl)break;
                for (auto& p : plist) {
                    if (!p || !p->lebt || p->id != pid)continue;
                    if (p->modus != PersonModus::Normal)continue;
                    if (p->partnerID >= 0)continue;
                    if (p->alterJ() < 18 || p->alterJ() > 50)continue;
                    int zielIdx = naechstesDorf(d.wx, d.wy, d.id);
                    if (zielIdx >= 0 && rf() < 0.5f) {
                        p->modus = PersonModus::Reisender;
                        p->reisezielDorfID = doerfer[zielIdx].id;
                        p->reiseZaehler = 0;
                        p->zielX = doerfer[zielIdx].wx + ri(-5, 5);
                        p->zielY = doerfer[zielIdx].wy + ri(-5, 5);
                        p->hatZiel = true;
                    }
                    else {
                        p->modus = PersonModus::Einsiedler;
                        p->istEinsiedler = true;
                        p->zielX = d.wx + ri(-40, 40);
                        p->zielY = d.wy + ri(-40, 40);
                        p->hatZiel = true;
                    }
                    ausgew++;
                    char buf[80]; _snprintf_s(buf, 80, _TRUNCATE,
                        "%s verlasst %s (%s)", p->name, d.name, zuArm ? "zu arm" : "Platzmangel");
                    addLog(buf, 0);
                    break;
                }
            }
        }
    }

    void weiseDorfZu(Person& p) {
        if (p.dorfID >= 0)return;
        for (int i = 0; i < (int)doerfer.size(); i++) {
            int dist = abs(doerfer[i].wx - p.wx) + abs(doerfer[i].wy - p.wy);
            if (dist <= doerfer[i].radius + 5) {
                trittDorfBei(p, i);
                return;
            }
        }
        if (rf() < 0.1f && (int)doerfer.size() < 20) {
            int idx = gruendeDorf(p.wx, p.wy, p.id);
            trittDorfBei(p, idx);
        }
        else {
            p.istEinsiedler = true;
            p.modus = PersonModus::Einsiedler;
        }
    }

    bool kaufeVomMarkt(Person& p, IT was, int menge = 1) {
        for (auto& a : markt.liste) {
            if (a.verkID == p.id || a.item != was || a.menge < menge)continue;
            float pr = a.preis * menge;
            if (p.muenzen < pr)continue;
            p.muenzen -= pr; p.inv.add(was, menge); a.menge -= menge;
            if (a.verkID > 0) {
                for (auto& s : plist) { if (s && s->lebt && s->id == a.verkID) { s->muenzen += pr; s->handelAnzahl++; break; } }
            }
            handels++; handelsvolumen[(int)was]++;
            return true;
        }
        return false;
    }

    // ================================================================
    //  UEBERLEBEN
    // ================================================================
    void wasserTrinken(Person& p) {
        if (p.inv.nimm(IT::Wasser, 2)) { p.durst = fc(p.durst - 35, 0, 100); strcpy_s(p.aktion, 80, "Trinkt"); return; }
        Tile cur = welt.getTile(p.wx, p.wy);
        if (istWasserquelle(cur)) {
            int m = (cur == Tile::Fluss) ? 15 : (cur == Tile::See) ? 12 : 8;
            p.inv.add(IT::Wasser, m); p.durst = fc(p.durst - 50, 0, 100);
            strcpy_s(p.aktion, 80, cur == Tile::Fluss ? "Schoepft Flusswasser" : "Schoepft Wasser"); return;
        }
        for (int ddy = -1; ddy <= 1; ddy++) {
            for (int ddx = -1; ddx <= 1; ddx++) {
                if (ddx == 0 && ddy == 0)continue;
                Tile nt = welt.getTile(p.wx + ddx, p.wy + ddy);
                if (istWasserquelle(nt)) {
                    p.inv.add(IT::Wasser, (nt == Tile::Fluss) ? 12 : 10);
                    p.durst = fc(p.durst - 45, 0, 100);
                    strcpy_s(p.aktion, 80, "Trinkt am Fluss"); return;
                }
            }
        }
        if (kaufeVomMarkt(p, IT::Wasser, 3)) { strcpy_s(p.aktion, 80, "Kauft Wasser"); return; }
        int fx, fy;
        if (welt.findNearestWasser(p.wx, p.wy, 60, fx, fy)) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht zu Wasser"); }
    }

    void essen(Person& p) {
        if (p.inv.nimm(IT::Suppentopf)) { p.hunger = fc(p.hunger - 45, 0, 100); p.hp = fc(p.hp + 2, 0, 100); strcpy_s(p.aktion, 80, "Isst Suppe"); }
        else if (p.inv.nimm(IT::NahrIT)) { p.hunger = fc(p.hunger - 30, 0, 100); p.hp = fc(p.hp + 1, 0, 100); strcpy_s(p.aktion, 80, "Isst Nahrung"); }
        else if (p.inv.nimm(IT::Brot)) { p.hunger = fc(p.hunger - 25, 0, 100); p.hp = fc(p.hp + 1, 0, 100); strcpy_s(p.aktion, 80, "Isst Brot"); }
        else if (p.inv.nimm(IT::Kaese)) { p.hunger = fc(p.hunger - 20, 0, 100); strcpy_s(p.aktion, 80, "Isst Kaese"); }
        else if (p.inv.nimm(IT::Honig)) { p.hunger = fc(p.hunger - 15, 0, 100); p.glueck = fc(p.glueck + 5, 0, 100); p.hp = fc(p.hp + 1, 0, 100); strcpy_s(p.aktion, 80, "Isst Honig"); }
        else if (p.inv.nimm(IT::Fleisch)) { p.hunger = fc(p.hunger - 22, 0, 100); p.hp = fc(p.hp + 1, 0, 100); strcpy_s(p.aktion, 80, "Isst Fleisch"); }
        else if (p.inv.nimm(IT::Fisch)) { p.hunger = fc(p.hunger - 18, 0, 100); strcpy_s(p.aktion, 80, "Isst Fisch"); }
        else if (p.inv.nimm(IT::Pflanze)) { p.hunger = fc(p.hunger - 8, 0, 100); strcpy_s(p.aktion, 80, "Isst Pflanzen"); }
        else {
            if (kaufeVomMarkt(p, IT::NahrIT)) { strcpy_s(p.aktion, 80, "Kauft Nahrung"); }
            else if (kaufeVomMarkt(p, IT::Brot)) { strcpy_s(p.aktion, 80, "Kauft Brot"); }
        }
    }

    void schlafen(Person& p) {
        bool istNacht = (zeit.stunde() >= 22 || zeit.stunde() <= 5);
        if (istNacht && p.hausGebIdx >= 0) {
            if (p.inv.hat(IT::Kerze)) {
                if (rf() < 0.05f) p.inv.nimm(IT::Kerze);
                p.glueck = fc(p.glueck + 2, 0, 100);
            }
        }
        float bonus = p.hausGebIdx >= 0 ? 1.5f : 1.0f;
        p.energie = fc(p.energie + 50 * bonus, 0, 100);
        p.stress = fc(p.stress - 20, 0, 100);
        p.hp = fc(p.hp + 3.0f * bonus, 0, 100);
        p.gesundheit = fc(p.gesundheit + 1.5f * bonus, 0, 100);
        strcpy_s(p.aktion, 80, "Schlaeft");
    }

    void heilen(Person& p) {
        if (p.inv.hat(IT::Medizin)) {
            p.inv.nimm(IT::Medizin);
            p.gesundheit = fc(p.gesundheit + 25, 0, 100); p.hp = fc(p.hp + 15, 0, 100);
            strcpy_s(p.aktion, 80, "Nimmt Medizin");
        }
        else if (p.inv.hat(IT::Pflanze, 2)) {
            p.inv.nimm(IT::Pflanze, 2);
            p.gesundheit = fc(p.gesundheit + 8, 0, 100);
            strcpy_s(p.aktion, 80, "Heilt mit Pflanzen");
        }
        else if (kaufeVomMarkt(p, IT::Medizin)) {
            strcpy_s(p.aktion, 80, "Kauft Medizin");
        }
    }

    // ================================================================
    //  HAUS BAUEN
    // ================================================================
    bool hausBauen(Person& p) {
        bool mitZiegel = p.inv.hat(IT::Ziegel, 6);
        bool mitBrettStein = p.inv.hat(IT::Brett, 8) && p.inv.hat(IT::Stein, 4);

        if (!mitZiegel && !mitBrettStein) {
            if (p.inv.hat(IT::Ton, 4) && p.inv.hat(IT::Kohle, 1)) {
                p.inv.nimm(IT::Ton, 4); p.inv.nimm(IT::Kohle, 1);
                p.inv.add(IT::Ziegel, 6);
                strcpy_s(p.aktion, 80, "Brennt Ziegel"); return true;
            }
            if (p.inv.hat(IT::Holz, 3) && !p.inv.hat(IT::Brett, 4)) {
                p.inv.nimm(IT::Holz, 3); p.inv.add(IT::Brett, 4);
                strcpy_s(p.aktion, 80, "Saeagt Bretter"); return true;
            }
            if (!p.inv.hat(IT::Brett, 4)) {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx, fy)) {
                    if (p.distTo(fx, fy) <= 1) { welt.dekrementRess(fx, fy); p.inv.add(IT::Holz, 2); strcpy_s(p.aktion, 80, "Faellt Holz"); }
                    else { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Holt Holz f.Haus"); }
                    return true;
                }
            }
            if (!p.inv.hat(IT::Stein, 4)) {
                int fx, fy;
                bool gef = welt.findNearest(p.wx, p.wy, Tile::Huegel, 20, fx, fy);
                if (!gef)gef = welt.findNearest(p.wx, p.wy, Tile::Berg, 25, fx, fy);
                if (gef) {
                    if (p.distTo(fx, fy) <= 1) { welt.dekrementRess(fx, fy); p.inv.add(IT::Stein, 2); strcpy_s(p.aktion, 80, "Sammelt Stein"); }
                    else { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Holt Stein f.Haus"); }
                    return true;
                }
            }
            if (!p.inv.hat(IT::Ton, 2)) {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Huegel, 20, fx, fy)) {
                    if (p.distTo(fx, fy) <= 1) { p.inv.add(IT::Ton, 3); strcpy_s(p.aktion, 80, "Grabt Ton"); }
                    else { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Sucht Ton"); }
                    return true;
                }
            }
            return false;
        }

        int fx, fy;
        if (welt.findFreiePlatz(p.wx, p.wy, 6, fx, fy)) {
            p.hausGebIdx = baueGebaeude(p, GebTyp::Haus, fx, fy);
            return true;
        }
        return false;
    }

    // ================================================================
    //  KLEIDUNG
    // ================================================================
    void kleidungBesorgen(Person& p) {
        bool istWinter = (zeit.saison() == 3);
        IT ziel = istWinter ? IT::KleidungWarm : IT::KleidungEinfach;

        if (kaufeVomMarkt(p, ziel)) {
            if (ziel == IT::KleidungWarm)p.kleidHaltbarkeit[1] = 100;
            else p.kleidHaltbarkeit[0] = 100;
            strcpy_s(p.aktion, 80, istWinter ? "Kauft warme Kleidung" : "Kauft Kleidung");
            return;
        }
        if (p.inv.hat(IT::Stoff, 3)) {
            p.inv.nimm(IT::Stoff, 3); p.inv.add(IT::KleidungEinfach, 1);
            p.kleidHaltbarkeit[0] = 80;
            strcpy_s(p.aktion, 80, "Naht Kleidung"); return;
        }
        if (p.inv.hat(IT::Pelz, 2)) {
            p.inv.nimm(IT::Pelz, 2); p.inv.add(IT::KleidungWarm, 1);
            p.kleidHaltbarkeit[1] = 100;
            strcpy_s(p.aktion, 80, "Naht warme Kleidung"); return;
        }
    }

    void kleidungVerschleiss(Person& p) {
        if (zeit.tick % 48 != p.id % 48)return;
        bool draussen = (p.hausGebIdx < 0);
        int wear = draussen ? 2 : 0;
        if (p.kleidHaltbarkeit[0] > 0)p.kleidHaltbarkeit[0] = std::max(0, p.kleidHaltbarkeit[0] - wear);
        if (p.kleidHaltbarkeit[1] > 0)p.kleidHaltbarkeit[1] = std::max(0, p.kleidHaltbarkeit[1] - wear);
        if (p.kleidHaltbarkeit[2] > 0)p.kleidHaltbarkeit[2] = std::max(0, p.kleidHaltbarkeit[2] - 1);
        if (p.kleidHaltbarkeit[0] <= 0 && p.inv.hat(IT::KleidungEinfach)) {
            p.inv.nimm(IT::KleidungEinfach); p.kleidHaltbarkeit[0] = 0;
            addLog((std::string(p.name) + " Kleidung zerfallen!").c_str(), 0);
        }
        if (p.kleidHaltbarkeit[1] <= 0 && p.inv.hat(IT::KleidungWarm)) {
            p.inv.nimm(IT::KleidungWarm); p.kleidHaltbarkeit[1] = 0;
        }
    }

    void werkzeugBenutzen(Person& p) {
        if (!p.inv.hat(IT::Werkzeug))return;
        p.werkzeugHaltbarkeit--;
        if (p.werkzeugHaltbarkeit <= 0) {
            p.inv.nimm(IT::Werkzeug);
            p.werkzeugHaltbarkeit = 0;
            char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s Werkzeug kaputt!", p.name);
            addLog(buf, 0);
        }
    }

    void waschen(Person& p) {
        if (p.inv.hat(IT::Seife)) {
            p.inv.nimm(IT::Seife);
            p.sauberkeit = fc(p.sauberkeit + 50, 0, 100);
            p.gesundheit = fc(p.gesundheit + 3, 0, 100);
            strcpy_s(p.aktion, 80, "Waesch sich");
        }
        else {
            Tile cur = welt.getTile(p.wx, p.wy);
            if (istWasserquelle(cur) && p.inv.nimm(IT::Wasser, 1)) {
                p.sauberkeit = fc(p.sauberkeit + 20, 0, 100);
                strcpy_s(p.aktion, 80, "Waesch sich am Fluss");
            }
        }
    }

    void steuerZahlen(Person& p) {
        p.steuerZaehler++;
        if (p.steuerZaehler < 720)return;
        p.steuerZaehler = 0;
        if (p.istKind())return;

        float steuer = 2.0f;
        if (p.hausGebIdx >= 0)steuer += 1.5f;
        if (p.ruf > 50) steuer *= 1.3f;

        if (p.muenzen >= steuer) {
            p.muenzen -= steuer;
            for (auto& o : plist) {
                if (o && o->lebt && o->beruf == JB::Wache) {
                    o->muenzen += steuer / (float)(std::max(1, (int)plist.size() / 20));
                    break;
                }
            }
        }
        else {
            if (p.inv.nimm(IT::NahrIT, 1)) {
            }
            else {
                p.stress = fc(p.stress + 15, 0, 100);
                char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s kann Steuer nicht zahlen!", p.name);
                addLog(buf, 1);
            }
        }
    }

    // ================================================================
    //  GEBAEUDE BAUEN
    // ================================================================
    int baueGebaeude(Person& p, GebTyp typ, int wx, int wy) {
        Gebaeude g; g.typ = typ; g.wx = wx; g.wy = wy; g.eigentuemerID = p.id; g.level = 1;
        g.isolierung = 20 + (int)(p.skill(SK::Bauen) / 5);
        g.heizung = p.inv.hat(IT::Holz, 5) ? 30 : 10;
        g.holzVorrat = p.inv.hat(IT::Holz, 5) ? 20 : 5;
        if (p.inv.hat(IT::Holz, 5))p.inv.nimm(IT::Holz, 5);

        if (p.inv.hat(IT::Ziegel, 6)) { g.isolierung += 15; p.inv.nimm(IT::Ziegel, 6); }
        if (p.inv.hat(IT::Kalk, 2)) { g.isolierung += 10; p.inv.nimm(IT::Kalk, 2); }

        switch (typ) {
        case GebTyp::Haus:
            strcpy_s(g.name, 32, "Haus"); welt.setTile(wx, wy, Tile::Haus);
            p.inv.nimm(IT::Brett, 8); p.inv.nimm(IT::Stein, 4);
            break;
        case GebTyp::Mine:
            strcpy_s(g.name, 32, "Mine"); welt.setTile(wx, wy, Tile::Mine);
            p.inv.nimm(IT::Brett, 5); p.inv.nimm(IT::Werkzeug, 1);
            p.verbSkill(SK::Bergbau, 5); break;
        case GebTyp::Werkstatt:
            strcpy_s(g.name, 32, "Werkstatt"); welt.setTile(wx, wy, Tile::Werkstatt);
            p.inv.nimm(IT::Brett, 12); p.inv.nimm(IT::Stein, 6); break;
        case GebTyp::Farm:
            strcpy_s(g.name, 32, "Farm"); welt.setTile(wx, wy, Tile::Farm);
            p.inv.nimm(IT::Brett, 4); break;
        case GebTyp::Lager:
            strcpy_s(g.name, 32, "Lager"); welt.setTile(wx, wy, Tile::Lager);
            p.inv.nimm(IT::Brett, 6); p.inv.nimm(IT::Stein, 3); break;
        case GebTyp::Muehle:
            strcpy_s(g.name, 32, "Muehle"); welt.setTile(wx, wy, Tile::Muehle);
            p.inv.nimm(IT::Brett, 10); p.inv.nimm(IT::Stein, 8); p.inv.nimm(IT::Seil, 3); break;
        case GebTyp::Imkerei:
            strcpy_s(g.name, 32, "Imkerei"); welt.setTile(wx, wy, Tile::Imkerei);
            p.inv.nimm(IT::Brett, 6); p.inv.nimm(IT::Stoff, 2); break;
        case GebTyp::Salzwerk:
            strcpy_s(g.name, 32, "Salzwerk"); welt.setTile(wx, wy, Tile::Salzwerk);
            p.inv.nimm(IT::Stein, 10); p.inv.nimm(IT::Kohle, 3); break;
        case GebTyp::Topferei:
            strcpy_s(g.name, 32, "Topferei"); welt.setTile(wx, wy, Tile::Topferei);
            p.inv.nimm(IT::Stein, 6); p.inv.nimm(IT::Brett, 4); break;
        case GebTyp::Glashuette:
            strcpy_s(g.name, 32, "Glashuette"); welt.setTile(wx, wy, Tile::Glashuette);
            p.inv.nimm(IT::Stein, 12); p.inv.nimm(IT::Kohle, 5); break;
        case GebTyp::Taverne:
            strcpy_s(g.name, 32, "Taverne"); welt.setTile(wx, wy, Tile::Tavernen);
            p.inv.nimm(IT::Brett, 15); p.inv.nimm(IT::Stein, 8); break;
        case GebTyp::Friedhof:
            strcpy_s(g.name, 32, "Friedhof"); welt.setTile(wx, wy, Tile::Friedhof);
            p.inv.nimm(IT::Stein, 5); break;
        case GebTyp::Kirche:
            strcpy_s(g.name, 32, "Kirche"); welt.setTile(wx, wy, Tile::Kirche);
            p.inv.nimm(IT::Brett, 10); p.inv.nimm(IT::Stein, 8); break;
        case GebTyp::Wachturm:
            strcpy_s(g.name, 32, "Wachturm"); welt.setTile(wx, wy, Tile::Wachturm);
            p.inv.nimm(IT::Brett, 6); p.inv.nimm(IT::Stein, 6); break;
        default:break;
        }
        int idx = (int)gebaeude.size();
        gebaeude.push_back(g);
        p.eigeneGebaeude.push_back(idx);
        gebaeudeAnz++;
        p.verbSkill(SK::Bauen, 3);
        char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "%s baut %s bei (%d,%d)", p.name, g.name, wx, wy);
        addLog(buf, 5); return idx;
    }

    bool hatGebaeude(const Person& p, GebTyp typ)const {
        for (int idx : p.eigeneGebaeude) {
            if (idx >= 0 && idx < (int)gebaeude.size() && gebaeude[idx].typ == typ && gebaeude[idx].eigentuemerID == p.id)return true;
        }
        return false;
    }

    // ================================================================
    //  ARBEIT
    // ================================================================
    void arbeit(Person& p) {
        if (p.istKind()) {
            SK s = (SK)ri(0, SK_COUNT - 1); p.verbSkill(s, 0.05f);
            strcpy_s(p.aktion, 80, "Lernt und spielt"); return;
        }
        if (zeit.tick % 120 == p.id % 120)p.beruf = p.berechneberuf();
        float eff = p.istGreis() ? 0.5f : 1.f;
        int saison = zeit.saison();

        switch (p.beruf) {
        case JB::Holzfaeller: {
            int fx, fy;
            if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx, fy)) {
                if (p.distTo(fx, fy) > 1) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht zu Wald"); }
                else {
                    int r = welt.getRess(fx, fy);
                    if (r > 0) {
                        welt.dekrementRess(fx, fy);
                        int menge = (int)(1 + p.skill(SK::Holzfaellen) / 30.f);
                        if (p.hatFunktionsfaehigesWerkzeug()) { menge += 1; werkzeugBenutzen(p); }
                        p.inv.add(IT::Holz, menge);
                        if (rf() < 0.15f)p.inv.add(IT::Harz, 1);
                        p.verbSkill(SK::Holzfaellen, 0.3f);
                        if (p.inv.anz(IT::Holz) > 12)
                            markt.addAngebot(p.id, IT::Holz, p.inv.anz(IT::Holz) - 8, markt.getPreis(IT::Holz, saison) * 0.9f, p.wx, p.wy);
                        strcpy_s(p.aktion, 80, "Faellt Baum");
                    }
                }
            }
            else strcpy_s(p.aktion, 80, "Sucht Wald...");
            break;
        }
        case JB::Bergmann: {
            bool hatMine = hatGebaeude(p, GebTyp::Mine);
            int fx, fy; bool gef = false;
            if (hatMine) { for (int idx : p.eigeneGebaeude) { if (idx < (int)gebaeude.size() && gebaeude[idx].typ == GebTyp::Mine) { fx = gebaeude[idx].wx; fy = gebaeude[idx].wy; gef = true; break; } } }
            else { gef = welt.findNearest(p.wx, p.wy, Tile::Huegel, 20, fx, fy); if (!gef)gef = welt.findNearest(p.wx, p.wy, Tile::Berg, 25, fx, fy); }
            if (gef) {
                if (p.distTo(fx, fy) > 1) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht zu Mine"); }
                else {
                    int r = welt.getRess(fx, fy); Tile t = welt.getTile(fx, fy);
                    if (r > 0 || hatMine) {
                        if (r > 0)welt.dekrementRess(fx, fy);
                        float bonus = hatMine ? 1.5f : 1.f;
                        bonus *= (1.f + p.skill(SK::Bergbau) / 50.f) * eff;
                        if (p.hatFunktionsfaehigesWerkzeug()) { bonus *= 1.5f; werkzeugBenutzen(p); }
                        if (rf() < 0.5f) { p.inv.add(IT::Stein, (int)(bonus * 1.5f)); strcpy_s(p.aktion, 80, "Baut Stein ab"); }
                        else if (rf() < 0.6f || t == Tile::Huegel) {
                            p.inv.add(IT::Erz, (int)bonus);
                            if (rf() < 0.25f)p.inv.add(IT::Ton, 1);
                            strcpy_s(p.aktion, 80, "Schuerft Erz");
                        }
                        else { p.inv.add(IT::Kohle, (int)(bonus * .5f)); strcpy_s(p.aktion, 80, "Foerdert Kohle"); }
                        p.verbSkill(SK::Bergbau, 0.4f);
                        if (p.inv.anz(IT::Erz) > 8) markt.addAngebot(p.id, IT::Erz, p.inv.anz(IT::Erz) - 4, markt.getPreis(IT::Erz, saison) * 0.85f, p.wx, p.wy);
                        if (p.inv.anz(IT::Kohle) > 6) markt.addAngebot(p.id, IT::Kohle, p.inv.anz(IT::Kohle) - 3, markt.getPreis(IT::Kohle, saison) * 0.85f, p.wx, p.wy);
                    }
                }
            }
            else strcpy_s(p.aktion, 80, "Sucht Huegel...");
            break;
        }
        case JB::Bergbaumeister: {
            int fx, fy; bool gef = false;
            for (int idx : p.eigeneGebaeude) { if (idx < (int)gebaeude.size() && gebaeude[idx].typ == GebTyp::Mine) { fx = gebaeude[idx].wx; fy = gebaeude[idx].wy; gef = true; break; } }
            if (!gef)gef = welt.findNearest(p.wx, p.wy, Tile::Berg, 30, fx, fy);
            if (gef) {
                if (p.distTo(fx, fy) > 1) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht zu Tiefmine"); }
                else {
                    float bonus = 2.f + p.skill(SK::Bergbaumeister) / 20.f;
                    p.inv.add(IT::Erz, (int)bonus); p.inv.add(IT::Stein, (int)(bonus * 1.5f));
                    if (rf() < 0.3f) { p.inv.add(IT::Gold, 1); strcpy_s(p.aktion, 80, "Findet Gold!"); }
                    else strcpy_s(p.aktion, 80, "Tiefbau: Erz+Stein");
                    p.verbSkill(SK::Bergbaumeister, 0.5f);
                    if (p.inv.anz(IT::Gold) > 2) markt.addAngebot(p.id, IT::Gold, 1, markt.getPreis(IT::Gold, saison) * 0.9f, p.wx, p.wy);
                }
            }
            break;
        }
        case JB::Jaeger: {
            int fx, fy;
            if (welt.findNearest(p.wx, p.wy, Tile::Wald, 20, fx, fy)) {
                if (p.distTo(fx, fy) > 2) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht jagen"); }
                else if (rf() < 0.4f + p.skill(SK::Jagd) / 200.f) {
                    p.inv.add(IT::Fleisch, ri(1, 4)); p.inv.add(IT::Pelz, ri(1, 2));
                    if (rf() < 0.3f)p.inv.add(IT::Fett, 1);
                    p.verbSkill(SK::Jagd, 0.4f);
                    markt.addAngebot(p.id, IT::Fleisch, 2, markt.getPreis(IT::Fleisch, saison) * 0.9f, p.wx, p.wy);
                    markt.addAngebot(p.id, IT::Pelz, 1, markt.getPreis(IT::Pelz, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, "Jagt Tier");
                }
            }
            break;
        }
        case JB::Fischer: {
            int fx, fy;
            if (welt.findNearestWasser(p.wx, p.wy, 20, fx, fy)) {
                if (p.distTo(fx, fy) > 1) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht fischen"); }
                else {
                    Tile wt = welt.getTile(fx, fy);
                    int menge = ri(1, 3) + (wt == Tile::Fluss ? 1 : 0);
                    p.inv.add(IT::Fisch, menge);
                    if (p.inv.anz(IT::Wasser) < 12)p.inv.add(IT::Wasser, 8);
                    p.verbSkill(SK::Fischen, 0.3f);
                    if (p.inv.anz(IT::Fisch) > 8) markt.addAngebot(p.id, IT::Fisch, p.inv.anz(IT::Fisch) - 4, markt.getPreis(IT::Fisch, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, wt == Tile::Fluss ? "Angelt im Fluss" : "Angelt");
                }
            }
            break;
        }
        case JB::Koch: {
            if (p.inv.hat(IT::Fleisch, 1) && p.inv.hat(IT::Pflanze, 2) && p.inv.hat(IT::Salz, 1)) {
                p.inv.nimm(IT::Fleisch, 1); p.inv.nimm(IT::Pflanze, 2); p.inv.nimm(IT::Salz, 1);
                p.inv.add(IT::Suppentopf, 2);
                markt.addAngebot(p.id, IT::Suppentopf, 2, markt.getPreis(IT::Suppentopf, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Kochen, 0.6f); strcpy_s(p.aktion, 80, "Kocht Suppe");
            }
            else if (p.inv.hat(IT::Fleisch, 1) && rf() < .6f) {
                p.inv.nimm(IT::Fleisch, 1); p.inv.add(IT::NahrIT, 3);
                markt.addAngebot(p.id, IT::NahrIT, 2, markt.getPreis(IT::NahrIT, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Kochen, 0.4f); strcpy_s(p.aktion, 80, "Kocht Fleisch");
            }
            else if (p.inv.hat(IT::Pflanze, 3)) {
                p.inv.nimm(IT::Pflanze, 3); p.inv.add(IT::Brot, 2);
                markt.addAngebot(p.id, IT::Brot, 1, markt.getPreis(IT::Brot, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Kochen, 0.5f); strcpy_s(p.aktion, 80, "Backt Brot");
            }
            else if (p.inv.hat(IT::Fisch)) {
                p.inv.nimm(IT::Fisch); p.inv.add(IT::NahrIT, 2);
                p.verbSkill(SK::Kochen, 0.3f); strcpy_s(p.aktion, 80, "Kocht Fisch");
            }
            else if (p.inv.hat(IT::Pflanze, 2)) {
                p.inv.nimm(IT::Pflanze, 2); p.inv.add(IT::NahrIT, 2);
                p.verbSkill(SK::Kochen, 0.2f); strcpy_s(p.aktion, 80, "Kocht Pflanzen");
            }
            break;
        }
        case JB::Schmied: {
            bool hatWerkstatt = hatGebaeude(p, GebTyp::Werkstatt);
            int fx, fy;
            if (!hatWerkstatt && p.inv.hat(IT::Brett, 12) && p.inv.hat(IT::Stein, 6)) {
                if (welt.findFreiePlatz(p.wx, p.wy, 5, fx, fy))baueGebaeude(p, GebTyp::Werkstatt, fx, fy);
            }
            if (p.inv.hat(IT::Erz, 3)) {
                p.inv.nimm(IT::Erz, 3);
                bool mitKohle = p.inv.nimm(IT::Kohle, 1);
                int menge = mitKohle ? 2 : 1;
                p.inv.add(IT::Werkzeug, menge);
                if (menge > 0) {
                    markt.addAngebot(p.id, IT::Werkzeug, menge, markt.getPreis(IT::Werkzeug, saison) * (mitKohle ? 1.1f : 0.85f), p.wx, p.wy);
                }
                p.verbSkill(SK::Schmieden, 0.6f);
                strcpy_s(p.aktion, 80, mitKohle ? "Schmiedet Stahlwerkzeug" : "Schmiedet Werkzeug");
            }
            else if (p.inv.hat(IT::Erz, 5) && rf() < .15f) {
                p.inv.nimm(IT::Erz, 5); p.inv.add(IT::Waffe, 1);
                markt.addAngebot(p.id, IT::Waffe, 1, markt.getPreis(IT::Waffe, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Schmieden, 1.f); strcpy_s(p.aktion, 80, "Schmiedet Waffe");
            }
            else strcpy_s(p.aktion, 80, "Wartet auf Erz...");
            break;
        }
        case JB::Tischler: {
            if (p.inv.hat(IT::Holz, 3)) {
                p.inv.nimm(IT::Holz, 3); p.inv.add(IT::Brett, 4);
                p.verbSkill(SK::Tischlern, 0.4f); strcpy_s(p.aktion, 80, "Saeagt Bretter");
                if (p.inv.anz(IT::Brett) > 10) markt.addAngebot(p.id, IT::Brett, p.inv.anz(IT::Brett) - 6, markt.getPreis(IT::Brett, saison) * 0.9f, p.wx, p.wy);
            }
            else if (p.inv.hat(IT::Brett, 5) && rf() < .3f) {
                p.inv.nimm(IT::Brett, 5); p.inv.add(IT::Moebel, 1);
                markt.addAngebot(p.id, IT::Moebel, 1, markt.getPreis(IT::Moebel, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Tischlern, 0.8f); strcpy_s(p.aktion, 80, "Baut Moebel");
            }
            else {
                if (p.inv.hat(IT::Brett, 8) && p.inv.hat(IT::Seil, 2) && rf() < 0.1f) {
                    p.inv.nimm(IT::Brett, 8); p.inv.nimm(IT::Seil, 2);
                    p.muenzen += 15;
                    p.verbSkill(SK::Tischlern, 1.5f); strcpy_s(p.aktion, 80, "Baut Karren");
                }
                int fx2, fy2;
                if (welt.findNearest(p.wx, p.wy, Tile::Wald, 15, fx2, fy2)) { p.zielX = fx2; p.zielY = fy2; p.hatZiel = true; strcpy_s(p.aktion, 80, "Holt Holz"); }
            }
            break;
        }
        case JB::Bauer: {
            bool hatFarm = hatGebaeude(p, GebTyp::Farm);
            if (!hatFarm && p.inv.hat(IT::Brett, 4) && saison != 3) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 4, fx, fy))baueGebaeude(p, GebTyp::Farm, fx, fy);
            }
            if (saison != 3) {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Flachland, 10, fx, fy)) {
                    if (p.distTo(fx, fy) > 1) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht aufs Feld"); }
                    else {
                        int menge = ri(1, 3) + (saison == 2 ? 2 : 0); if (hatFarm)menge = (int)(menge * 1.5f);
                        p.inv.add(IT::Pflanze, menge);
                        if (rf() < 0.1f && hatFarm)p.inv.add(IT::Wolle, 1);
                        p.verbSkill(SK::Holzfaellen, 0.1f);
                        if (p.inv.anz(IT::Pflanze) > 15) markt.addAngebot(p.id, IT::Pflanze, p.inv.anz(IT::Pflanze) - 8, markt.getPreis(IT::Pflanze, saison) * 0.85f, p.wx, p.wy);
                        strcpy_s(p.aktion, 80, saison == 2 ? "Erntet Feld" : "Pflegt Feld");
                    }
                }
            }
            else strcpy_s(p.aktion, 80, "Winterpause");
            break;
        }
        case JB::Gerber: {
            if (p.inv.hat(IT::Pelz, 2)) {
                p.inv.nimm(IT::Pelz, 2); p.inv.add(IT::Stoff, 2); p.inv.add(IT::Leder, 1);
                markt.addAngebot(p.id, IT::Stoff, 2, markt.getPreis(IT::Stoff, saison) * 0.85f, p.wx, p.wy);
                markt.addAngebot(p.id, IT::Leder, 1, markt.getPreis(IT::Leder, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Gerben, 0.4f); strcpy_s(p.aktion, 80, "Gerbt Pelze zu Stoff+Leder");
            }
            else if (p.inv.hat(IT::Wolle, 3)) {
                p.inv.nimm(IT::Wolle, 3); p.inv.add(IT::Stoff, 4);
                markt.addAngebot(p.id, IT::Stoff, 3, markt.getPreis(IT::Stoff, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Gerben, 0.5f); strcpy_s(p.aktion, 80, "Spinnt Wolle");
            }
            if (p.inv.hat(IT::Stoff, 4) && rf() < 0.4f) {
                p.inv.nimm(IT::Stoff, 4); p.inv.add(IT::KleidungEinfach, 1);
                markt.addAngebot(p.id, IT::KleidungEinfach, 1, markt.getPreis(IT::KleidungEinfach, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Gerben, 0.6f); strcpy_s(p.aktion, 80, "Naht Kleidung");
            }
            if (p.inv.hat(IT::Leder, 3) && p.inv.hat(IT::Stoff, 2) && rf() < 0.3f) {
                p.inv.nimm(IT::Leder, 3); p.inv.nimm(IT::Stoff, 2); p.inv.add(IT::KleidungWarm, 1);
                markt.addAngebot(p.id, IT::KleidungWarm, 1, markt.getPreis(IT::KleidungWarm, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Gerben, 0.8f); strcpy_s(p.aktion, 80, "Naht warme Kleidung");
            }
            break;
        }
        case JB::Brauer: {
            if (p.inv.hat(IT::Pflanze, 4) && p.inv.hat(IT::Wasser, 3)) {
                p.inv.nimm(IT::Pflanze, 4); p.inv.nimm(IT::Wasser, 3);
                int anzahl = p.inv.hat(IT::Keramik) ? 3 : 2;
                p.inv.add(IT::Bier, anzahl);
                if (rf() < 0.3f)p.inv.add(IT::Essig, 1);
                markt.addAngebot(p.id, IT::Bier, anzahl, markt.getPreis(IT::Bier, saison) * .9f, p.wx, p.wy);
                if (p.inv.hat(IT::Essig)) markt.addAngebot(p.id, IT::Essig, 1, markt.getPreis(IT::Essig, saison) * .9f, p.wx, p.wy);
                p.verbSkill(SK::Brauen, 0.4f); strcpy_s(p.aktion, 80, "Braut Bier");
            }
            break;
        }
        case JB::Arzt: {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || p.distTo(o->wx, o->wy) > 6)continue;
                if (o->gesundheit < 60 && ri(0, 12) == 0) {
                    float arztkosten = 5.f + p.skill(SK::Heilen) / 10.f;
                    if (p.inv.hat(IT::Medizin)) {
                        float heil = 12 + p.skill(SK::Heilen) / 6.f;
                        o->gesundheit = fc(o->gesundheit + heil, 0, 100); o->hp = fc(o->hp + 8, 0, 100);
                        p.inv.nimm(IT::Medizin);
                        if (o->muenzen >= arztkosten) { o->muenzen -= arztkosten; p.muenzen += arztkosten; }
                        p.ruf += 2; p.verbSkill(SK::Heilen, 0.5f);
                        _snprintf_s(p.aktion, 80, _TRUNCATE, "Behandelt %s (%.0fM)", o->name, arztkosten);
                    }
                    else if (p.inv.hat(IT::Pflanze, 4)) {
                        p.inv.nimm(IT::Pflanze, 4); p.inv.add(IT::Medizin, 1);
                        p.verbSkill(SK::Heilen, 0.3f); strcpy_s(p.aktion, 80, "Stellt Medizin her");
                    }
                    if (p.inv.hat(IT::Seife) && o->sauberkeit < 30) {
                        p.inv.nimm(IT::Seife); o->sauberkeit = fc(o->sauberkeit + 40, 0, 100);
                        o->gesundheit = fc(o->gesundheit + 5, 0, 100);
                    }
                    break;
                }
            }
            break;
        }
        case JB::Lehrer: {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || p.distTo(o->wx, o->wy) > 3)continue;
                if (ri(0, 25) == 0) {
                    SK s = (SK)ri(0, SK_COUNT - 1);
                    if (p.skill(s) > o->skill(s) + 10) {
                        o->verbSkill(s, 0.5f * p.skill(SK::Lehren) / 100.f);
                        o->bildung = fc(o->bildung + 1, 0, 100);
                        float lehrkosten = 2.f;
                        if (o->muenzen >= lehrkosten) { o->muenzen -= lehrkosten; p.muenzen += lehrkosten; }
                        p.verbSkill(SK::Lehren, 0.2f);
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
            if (rf() < .4f)p.muenzen += 2.5f;
            strcpy_s(p.aktion, 80, "Bewacht Siedlung");
            break;
        }
        case JB::Seiler: {
            if (p.inv.hat(IT::Pflanze, 4)) {
                p.inv.nimm(IT::Pflanze, 4); p.inv.add(IT::Seil, 3);
                markt.addAngebot(p.id, IT::Seil, 2, markt.getPreis(IT::Seil, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Seilen, 0.4f); strcpy_s(p.aktion, 80, "Dreht Seile");
            }
            else if (p.inv.hat(IT::Wolle, 3)) {
                p.inv.nimm(IT::Wolle, 3); p.inv.add(IT::Seil, 4); p.inv.add(IT::Stoff, 1);
                markt.addAngebot(p.id, IT::Seil, 3, markt.getPreis(IT::Seil, saison) * 0.95f, p.wx, p.wy);
                p.verbSkill(SK::Seilen, 0.5f); strcpy_s(p.aktion, 80, "Spinnt Wollseile");
            }
            else {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Flachland, 10, fx, fy)) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Sammelt Pflanzen f.Seil"); }
            }
            break;
        }
        case JB::Toepfer: {
            if (p.inv.hat(IT::Ton, 4) && p.inv.hat(IT::Kohle, 1)) {
                p.inv.nimm(IT::Ton, 4); p.inv.nimm(IT::Kohle, 1);
                if (rf() < 0.5f) {
                    p.inv.add(IT::Keramik, 3);
                    markt.addAngebot(p.id, IT::Keramik, 2, markt.getPreis(IT::Keramik, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, "Brennt Keramik");
                }
                else {
                    p.inv.add(IT::Ziegel, 6);
                    markt.addAngebot(p.id, IT::Ziegel, 4, markt.getPreis(IT::Ziegel, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, "Brennt Ziegel");
                }
                p.verbSkill(SK::Toepfern, 0.5f);
            }
            else {
                int fx, fy;
                if (!p.inv.hat(IT::Ton, 2) && welt.findNearest(p.wx, p.wy, Tile::Huegel, 20, fx, fy)) {
                    if (p.distTo(fx, fy) <= 1) { p.inv.add(IT::Ton, 3); strcpy_s(p.aktion, 80, "Grabt Ton"); }
                    else { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Sucht Ton"); }
                }
                if (!p.inv.hat(IT::Kohle, 1)) { kaufeVomMarkt(p, IT::Kohle, 2); strcpy_s(p.aktion, 80, "Kauft Kohle"); }
            }
            break;
        }
        case JB::Glasmacher: {
            bool hatHuette = hatGebaeude(p, GebTyp::Glashuette);
            if (!hatHuette && p.inv.hat(IT::Stein, 12) && p.inv.hat(IT::Kohle, 5)) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 5, fx, fy))baueGebaeude(p, GebTyp::Glashuette, fx, fy);
            }
            if (hatHuette && p.inv.hat(IT::Stein, 4) && p.inv.hat(IT::Kalk, 2) && p.inv.hat(IT::Kohle, 2)) {
                p.inv.nimm(IT::Stein, 4); p.inv.nimm(IT::Kalk, 2); p.inv.nimm(IT::Kohle, 2);
                p.inv.add(IT::Glas, 3);
                markt.addAngebot(p.id, IT::Glas, 2, markt.getPreis(IT::Glas, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Glasmachen, 0.6f); strcpy_s(p.aktion, 80, "Schmilzt Glas");
            }
            else {
                if (!p.inv.hat(IT::Kalk, 1) && p.inv.hat(IT::Stein, 3)) {
                    p.inv.nimm(IT::Stein, 3); p.inv.add(IT::Kalk, 2); strcpy_s(p.aktion, 80, "Brennt Kalk");
                }
                strcpy_s(p.aktion, 80, "Beschafft Materialien f.Glas");
            }
            break;
        }
        case JB::Muller: {
            bool hatMuehle = hatGebaeude(p, GebTyp::Muehle);
            if (!hatMuehle && p.inv.hat(IT::Brett, 10) && p.inv.hat(IT::Stein, 8) && p.inv.hat(IT::Seil, 3)) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 5, fx, fy))baueGebaeude(p, GebTyp::Muehle, fx, fy);
            }
            if (p.inv.hat(IT::Pflanze, 5)) {
                p.inv.nimm(IT::Pflanze, 5);
                int menge = hatMuehle ? 4 : 2;
                p.inv.add(IT::Brot, menge);
                markt.addAngebot(p.id, IT::Brot, menge - 1, markt.getPreis(IT::Brot, saison) * 0.85f, p.wx, p.wy);
                p.verbSkill(SK::Muellern, 0.4f); strcpy_s(p.aktion, 80, hatMuehle ? "Mahlt Getreide (Muehle)" : "Stampft Getreide");
            }
            else {
                kaufeVomMarkt(p, IT::Pflanze, 5); strcpy_s(p.aktion, 80, "Kauft Getreide");
            }
            break;
        }
        case JB::Imker: {
            bool hatImkerei = hatGebaeude(p, GebTyp::Imkerei);
            if (!hatImkerei && p.inv.hat(IT::Brett, 6) && p.inv.hat(IT::Stoff, 2)) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 8, fx, fy))baueGebaeude(p, GebTyp::Imkerei, fx, fy);
            }
            if (hatImkerei && saison != 3) {
                float bonus = 1.f + p.skill(SK::Imkern) / 80.f;
                if (rf() < 0.5f * bonus) {
                    p.inv.add(IT::Honig, ri(1, 3));
                    markt.addAngebot(p.id, IT::Honig, 1, markt.getPreis(IT::Honig, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, "Erntet Honig");
                }
                if (rf() < 0.3f * bonus) {
                    p.inv.add(IT::Wachs, 1); strcpy_s(p.aktion, 80, "Erntet Wachs");
                }
                p.verbSkill(SK::Imkern, 0.3f);
                if (p.inv.hat(IT::Wachs, 3) && p.inv.hat(IT::Seil, 1)) {
                    p.inv.nimm(IT::Wachs, 3); p.inv.nimm(IT::Seil, 1); p.inv.add(IT::Kerze, 4);
                    markt.addAngebot(p.id, IT::Kerze, 3, markt.getPreis(IT::Kerze, saison) * 0.9f, p.wx, p.wy);
                    strcpy_s(p.aktion, 80, "Macht Wachskerzen");
                }
            }
            else strcpy_s(p.aktion, 80, hatImkerei ? "Winterpause (Imkerei)" : "Baut Imkerei");
            break;
        }
        case JB::Salzsieder: {
            bool hatSalzwerk = hatGebaeude(p, GebTyp::Salzwerk);
            if (!hatSalzwerk && p.inv.hat(IT::Stein, 10) && p.inv.hat(IT::Kohle, 3)) {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Wueste, 30, fx, fy))
                    baueGebaeude(p, GebTyp::Salzwerk, fx, fy);
                else if (welt.findFreiePlatz(p.wx, p.wy, 8, fx, fy))
                    baueGebaeude(p, GebTyp::Salzwerk, fx, fy);
            }
            if (hatSalzwerk) {
                if (p.inv.hat(IT::Wasser, 5) && p.inv.hat(IT::Kohle, 1)) {
                    p.inv.nimm(IT::Wasser, 5); p.inv.nimm(IT::Kohle, 1);
                    int salz = 2 + (int)(p.skill(SK::Salzsieden) / 30);
                    p.inv.add(IT::Salz, salz);
                    markt.addAngebot(p.id, IT::Salz, salz - 1, markt.getPreis(IT::Salz, saison) * 0.9f, p.wx, p.wy);
                    p.verbSkill(SK::Salzsieden, 0.4f); strcpy_s(p.aktion, 80, "Siedet Salz");
                }
                else {
                    int fx, fy;
                    if (welt.findNearest(p.wx, p.wy, Tile::Wueste, 30, fx, fy)) {
                        if (p.distTo(fx, fy) <= 1) { p.inv.add(IT::Salz, 2); strcpy_s(p.aktion, 80, "Gewinnt Salz"); }
                        else { p.zielX = fx; p.zielY = fy; p.hatZiel = true; strcpy_s(p.aktion, 80, "Geht zur Salzwueste"); }
                    }
                }
            }
            else strcpy_s(p.aktion, 80, "Baut Salzwerk...");
            break;
        }
        case JB::Buchbinder: {
            if (p.inv.hat(IT::Pergament, 3) && p.inv.hat(IT::Leder, 1)) {
                p.inv.nimm(IT::Pergament, 3); p.inv.nimm(IT::Leder, 1);
                p.inv.add(IT::Buch, 1);
                markt.addAngebot(p.id, IT::Buch, 1, markt.getPreis(IT::Buch, saison) * 0.9f, p.wx, p.wy);
                p.verbSkill(SK::Buchbinden, 0.6f); strcpy_s(p.aktion, 80, "Bindet Buch");
            }
            else {
                if (p.inv.hat(IT::Leder, 2) && p.inv.hat(IT::Asche, 1)) {
                    p.inv.nimm(IT::Leder, 2); p.inv.nimm(IT::Asche, 1);
                    p.inv.add(IT::Pergament, 3); strcpy_s(p.aktion, 80, "Macht Pergament");
                }
                else {
                    kaufeVomMarkt(p, IT::Leder, 2); kaufeVomMarkt(p, IT::Pergament, 2);
                    strcpy_s(p.aktion, 80, "Kauft Materialien f.Buch");
                }
            }
            break;
        }
        case JB::Wirt: {
            bool hatTaverne = hatGebaeude(p, GebTyp::Taverne);
            if (!hatTaverne && p.inv.hat(IT::Brett, 15) && p.inv.hat(IT::Stein, 8)) {
                int fx, fy;
                if (welt.findFreiePlatz(p.wx, p.wy, 5, fx, fy))baueGebaeude(p, GebTyp::Taverne, fx, fy);
            }
            if (hatTaverne) {
                int fx = -1, fy = -1;
                for (auto& g : gebaeude) { if (g.eigentuemerID == p.id && g.typ == GebTyp::Taverne) { fx = g.wx; fy = g.wy; break; } }
                if (fx >= 0) {
                    if (!p.inv.hat(IT::Bier, 5)) kaufeVomMarkt(p, IT::Bier, 3);
                    if (!p.inv.hat(IT::NahrIT, 5)) kaufeVomMarkt(p, IT::NahrIT, 3);
                    for (auto& o : plist) {
                        if (!o || !o->lebt || o->id == p.id || o->distTo(fx, fy) > 3)continue;
                        if (o->stress > 50 && o->muenzen > 3 && p.inv.hat(IT::Bier)) {
                            float preis = markt.getPreis(IT::Bier, saison) * 1.4f;
                            if (o->muenzen >= preis) {
                                o->muenzen -= preis; p.muenzen += preis;
                                p.inv.nimm(IT::Bier); o->inv.add(IT::Bier, 1);
                                o->stress = fc(o->stress - 20, 0, 100); o->glueck = fc(o->glueck + 15, 0, 100);
                                strcpy_s(p.aktion, 80, "Zapft Bier fuer Gast");
                            }
                        }
                        if (o->hunger > 60 && o->muenzen > 4 && p.inv.hat(IT::NahrIT)) {
                            float preis = markt.getPreis(IT::NahrIT, saison) * 1.5f;
                            if (o->muenzen >= preis) {
                                o->muenzen -= preis; p.muenzen += preis;
                                p.inv.nimm(IT::NahrIT); o->inv.add(IT::NahrIT, 1);
                                o->hunger = fc(o->hunger - 20, 0, 100);
                                strcpy_s(p.aktion, 80, "Serviert Essen");
                            }
                        }
                    }
                }
            }
            else strcpy_s(p.aktion, 80, "Plant Taverne");
            break;
        }
        case JB::Forscher: {
            p.verbSkill(SK::Forschen, 0.5f); p.verbSkill((SK)ri(0, SK_COUNT - 1), 0.2f);
            p.bildung = fc(p.bildung + 0.5f, 0, 100);
            if (p.inv.hat(IT::Fett, 2) && p.inv.hat(IT::Asche, 2)) {
                p.inv.nimm(IT::Fett, 2); p.inv.nimm(IT::Asche, 2);
                p.inv.add(IT::Seife, 3);
                markt.addAngebot(p.id, IT::Seife, 2, markt.getPreis(IT::Seife, saison) * 0.9f, p.wx, p.wy);
                strcpy_s(p.aktion, 80, "Stellt Seife her");
            }
            if (rf() < .01f) { char b[80]; _snprintf_s(b, 80, _TRUNCATE, "%s macht Entdeckung!", p.name); addLog(b, 3); }
            else strcpy_s(p.aktion, 80, "Forscht");
            break;
        }
        case JB::Haendler: {
            tausch(p); break;
        }
        default: {
            if (rf() < .5f) {
                p.inv.add(IT::Pflanze, ri(1, 2));
                if (p.inv.anz(IT::Pflanze) > 10) markt.addAngebot(p.id, IT::Pflanze, p.inv.anz(IT::Pflanze) - 5, markt.getPreis(IT::Pflanze, saison) * 0.8f, p.wx, p.wy);
                strcpy_s(p.aktion, 80, "Sammelt Pflanzen");
            }
            else {
                int fx, fy;
                if (welt.findNearest(p.wx, p.wy, Tile::Wald, 10, fx, fy)) { p.zielX = fx; p.zielY = fy; p.hatZiel = true; }
            }
            break;
        }
        }
        float ek = rf() * 0.8f + 0.15f;
        if (p.beruf == JB::Bergmann || p.beruf == JB::Bergbaumeister)ek *= 1.3f;
        if (p.beruf == JB::Glasmacher || p.beruf == JB::Salzsieder)ek *= 1.2f;
        p.energie = fc(p.energie - ek, 0, 100);
    }

    // ================================================================
    //  TAUSCH
    // ================================================================
    void tausch(Person& p) {
        int saison = zeit.saison();
        IT wunsch = IT::NahrIT; float maxB = 0;
        auto check = [&](IT it, float b) {if (b > maxB) { maxB = b; wunsch = it; }};
        check(IT::NahrIT, p.hunger);
        check(IT::Wasser, p.durst);
        check(IT::Medizin, p.gesundheit < 40 ? 70.f : 0.f);
        check(IT::Brett, p.hausGebIdx < 0 ? 55.f : 0.f);
        check(IT::Werkzeug, !p.inv.hat(IT::Werkzeug) ? 50.f : 0.f);
        check(IT::KleidungWarm, saison == 3 && !p.hatWarmeKleidung() ? 90.f : 0.f);
        check(IT::KleidungEinfach, !p.hatIrgendKleidung() ? 70.f : 0.f);
        check(IT::Kerze, saison == 3 && !p.inv.hat(IT::Kerze) ? 30.f : 0.f);
        check(IT::Seife, p.sauberkeit < 20 ? 40.f : 0.f);
        check(IT::Bier, p.stress > 70 ? 40.f : 0.f);
        check(IT::Salz, !p.inv.hat(IT::Salz) && p.beruf == JB::Koch ? 35.f : 0.f);
        check(IT::Buch, p.bildung < 30 && p.muenzen>30 ? 20.f : 0.f);
        check(IT::Seil, p.beruf == JB::Muller && !p.inv.hat(IT::Seil) ? 45.f : 0.f);
        check(IT::Ton, p.beruf == JB::Toepfer && !p.inv.hat(IT::Ton, 3) ? 60.f : 0.f);
        check(IT::Kohle, (p.beruf == JB::Glasmacher || p.beruf == JB::Salzsieder) && !p.inv.hat(IT::Kohle, 2) ? 55.f : 0.f);

        markt.registriereNachfrage(wunsch);
        kaufeVomMarkt(p, wunsch);

        struct { IT it; int min; } angebote[] = {
            {IT::Holz,8},{IT::Stein,5},{IT::Erz,4},{IT::NahrIT,10},
            {IT::Brot,5},{IT::Fleisch,6},{IT::Fisch,5},{IT::Brett,6},
            {IT::Werkzeug,1},{IT::Moebel,1},{IT::Bier,3},{IT::Stoff,4},
            {IT::Medizin,1},{IT::Gold,1},{IT::Pelz,3},{IT::Kohle,4},
            {IT::Keramik,3},{IT::Seil,4},{IT::Leder,3},{IT::Seife,2},
            {IT::Kerze,3},{IT::Honig,2},{IT::Salz,4},{IT::Glas,2},
            {IT::Pergament,2},{IT::Buch,1},{IT::KleidungEinfach,1},
            {IT::KleidungWarm,1},{IT::Essig,2},{IT::Fett,2},{IT::Wachs,2},
            {IT::Suppentopf,2},{IT::Ziegel,6},{IT::Kalk,3}
        };
        for (auto& ao : angebote) {
            int cnt = p.inv.anz(ao.it);
            if (cnt > ao.min) {
                float auf = (p.beruf == JB::Haendler) ? 1.2f : 0.9f;
                auf += p.skill(SK::Handel) / 400.f;
                markt.addAngebot(p.id, ao.it, cnt - ao.min, markt.getPreis(ao.it, saison) * auf, p.wx, p.wy);
            }
        }
    }

    // ================================================================
    //  SOZIAL
    // ================================================================
    void sozial(Person& p) {
        if (p.inv.hat(IT::Buch) && rf() < 0.15f) {
            p.bildung = fc(p.bildung + 3, 0, 100);
            SK s = (SK)ri(0, SK_COUNT - 1);
            p.verbSkill(s, 0.3f);
            if (rf() < 0.05f)p.inv.nimm(IT::Buch);
            strcpy_s(p.aktion, 80, "Liest Buch"); return;
        }
        if (p.partnerID < 0 && p.alterJ() >= 18 && p.alterJ() <= 52 && rf() < .06f) {
            for (auto& o : plist) {
                if (!o || !o->lebt || o->id == p.id || o->maennl == p.maennl || o->partnerID >= 0 || o->istKind())continue;
                if (p.distTo(o->wx, o->wy) > 20)continue;
                float k = 1.f - std::abs(p.empathie - o->empathie) / 100.f - std::abs(p.intel - o->intel) / 200.f;
                if (k > .4f && rf() < k * .2f) {
                    p.partnerID = o->id; o->partnerID = p.id;
                    p.glueck = fc(p.glueck + 20, 0, 100); o->glueck = fc(o->glueck + 20, 0, 100);
                    char buf[80]; _snprintf_s(buf, 80, _TRUNCATE, "Hochzeit: %s & %s", p.name, o->name);
                    addLog(buf, 4); break;
                }
            }
        }
        if (p.partnerID >= 0 && p.maennl && !p.istGreis()) {
            for (auto& partner : plist) {
                if (!partner || partner->id != p.partnerID || !partner->lebt || partner->istKind())continue;
                bool res = p.inv.anz(IT::NahrIT) >= 3 && p.inv.anz(IT::Wasser) >= 5;
                if (res && partner->alterJ() >= 18 && partner->alterJ() <= 43 && p.alterJ() <= 53 && rf() < .04f)
                    neuesKind(p, *partner);
                break;
            }
        }
        if (p.inv.hat(IT::Bier) && rf() < .12f) {
            p.inv.nimm(IT::Bier);
            p.glueck = fc(p.glueck + 15, 0, 100); p.stress = fc(p.stress - 15, 0, 100);
            strcpy_s(p.aktion, 80, "Trinkt Bier");
        }
        p.glueck = fc(p.glueck + rf() * 2, 0, 100);
        p.stress = fc(p.stress - rf() * 3, 0, 100);
    }

    void neuesKind(Person& v, Person& m) {
        bool male = (rf() < .5f);
        auto c = std::make_shared<Person>(zname(male), 0, male, v.wx, v.wy);
        auto inh = [](float a, float b) {return fc((a + b) / 2 + (-4 + rf() * 8), 1, 100); };
        c->intel = inh(v.intel, m.intel); c->staerke = inh(v.staerke, m.staerke);
        c->empathie = inh(v.empathie, m.empathie); c->fleiss = inh(v.fleiss, m.fleiss);
        c->lernfg = inh(v.lernfg, m.lernfg);
        c->bildung = std::min(v.bildung, m.bildung) * 0.3f;
        for (int s = 0; s < SK_COUNT; s++) c->skills[s] = (v.skills[s] + m.skills[s]) * .06f;
        c->vaterID = v.id; c->mutterID = m.id;
        if (v.hausGebIdx >= 0)c->hausGebIdx = v.hausGebIdx;
        v.kinderIDs.push_back(c->id); m.kinderIDs.push_back(c->id);
        v.glueck = fc(v.glueck + 25, 0, 100); m.glueck = fc(m.glueck + 25, 0, 100);
        if (v.inv.hat(IT::KleidungEinfach) || m.inv.hat(IT::KleidungEinfach)) {
            c->inv.add(IT::KleidungEinfach, 1); c->kleidHaltbarkeit[0] = 60;
            if (v.inv.hat(IT::KleidungEinfach))v.inv.nimm(IT::KleidungEinfach);
            else m.inv.nimm(IT::KleidungEinfach);
        }
        char buf[128]; _snprintf_s(buf, 128, _TRUNCATE, "Geburt: %s (Eltern: %s & %s)", c->name, v.name, m.name);
        addLog(buf, 2); neuePersonen.push_back(c); geburten++;
    }

    void sterbenCheck(Person& p) {
        if (p.hp <= 0) { tod(p, "Verletzung/Erschoepfung"); return; }
        if (p.hunger >= 100) { tod(p, "Verhungert"); return; }
        if (p.durst >= 100) { tod(p, "Verdurstet"); return; }
        if (p.sauberkeit < 10 && rf() < 0.002f) { tod(p, "Seuche (Schmutz)"); return; }
        // Erfrieren: nur ohne Haus UND ohne jede Kleidung, sehr seltener Tick-Tod
        if (zeit.saison() == 3 && p.hausGebIdx < 0 && !p.hatIrgendKleidung() && rf() < .0008f) { tod(p, "Erfroren"); return; }
        if (p.alterJ() > 72) { float sc = 0.000004f * (p.alterJ() - 72); if (rf() < sc)tod(p, "Hohes Alter"); }
    }
    void tod(Person& p, const char* ursache) {
        p.lebt = false; tode++;
        strncpy_s(p.todesursache, 64, ursache, _TRUNCATE);
        char b[128]; _snprintf_s(b, 128, _TRUNCATE, "TOD: %s (%dJ) - %s", p.name, p.alterJ(), ursache);
        addLog(b, 1);
        if (p.partnerID >= 0) {
            for (auto& o : plist)if (o && o->id == p.partnerID) { o->partnerID = -1; o->glueck = fc(o->glueck - 25, 0, 100); break; }
        }
        for (int i = 0; i < IT_COUNT; i++) {
            if (p.inv.items[i] > 0 && i != (int)IT::Muenzen) {
                markt.addAngebot(-1, (IT)i, p.inv.items[i], itBasiswert((IT)i) * 0.5f, p.wx, p.wy);
            }
        }
    }

    // ================================================================
    //  EREIGNISSE
    // ================================================================
    void ereignisse() {
        if (rf() < .0012f) {
            int k = ri(1, 8); char txt[128]; int typ = 3;
            switch (k) {
            case 1:
                _snprintf_s(txt, 128, _TRUNCATE, "DUERRE! Felder trocknen aus.");
                for (auto& p : plist)if (p->lebt)p->inv.nimm(IT::Pflanze, p->inv.anz(IT::Pflanze) / 2);
                break;
            case 2:
                _snprintf_s(txt, 128, _TRUNCATE, "FLUT! Keller unter Wasser.");
                for (auto& p : plist)if (p->lebt && rf() < .2f)p->inv.nimm(IT::NahrIT, p->inv.anz(IT::NahrIT) / 3);
                break;
            case 3: {
                static const char* sn[] = { "Pest","Grippe","Typhus","Cholera" };
                const char* s = sn[ri(0, 3)];
                _snprintf_s(txt, 128, _TRUNCATE, "SEUCHE: %s bricht aus!", s); typ = 1;
                for (auto& p : plist)if (p->lebt && rf() < .2f * (p->sauberkeit < 30 ? 2.f : 1.f)) {
                    p->gesundheit = fc(p->gesundheit - 25, 0, 100);
                    if (p->inv.hat(IT::Seife))p->gesundheit = fc(p->gesundheit + 10, 0, 100);
                }
                break;
            }
            case 4: {
                static const char* en[] = { "Muehle","Glasherstellung","Stahllegierung","Seifenherstellung" };
                _snprintf_s(txt, 128, _TRUNCATE, "ENTDECKUNG: %s!", en[ri(0, 4)]); typ = 2;
                for (auto& p : plist)if (p->lebt && p->intel > 60)p->verbSkill(SK::Forschen, 10);
                break;
            }
            case 5: {
                int n2 = ri(8, 20);
                _snprintf_s(txt, 128, _TRUNCATE, "EINWANDERUNG: %d Personen kommen!", n2); typ = 2;
                for (int i = 0; i < n2; i++) {
                    bool m = (rf() < .5f);
                    auto np = std::make_shared<Person>(zname(m), ri(20 * 12, 35 * 12), m, ri(-10, 10), ri(-10, 10));
                    np->beruf = np->berechneberuf();
                    np->inv.add(IT::Wasser, ri(20, 35)); np->inv.add(IT::NahrIT, ri(5, 10));
                    np->inv.add(IT::KleidungEinfach, 1); np->kleidHaltbarkeit[0] = ri(30, 70);
                    neuePersonen.push_back(np);
                }
                break;
            }
            case 6:
                if (zeit.saison() == 2) {
                    _snprintf_s(txt, 128, _TRUNCATE, "REICHE ERNTE! Alle Felder voll."); typ = 2;
                    for (auto& p : plist)if (p->lebt && (p->beruf == JB::Bauer || hatGebaeude(*p, GebTyp::Farm)))
                        p->inv.add(IT::Pflanze, ri(8, 20));
                }
                else return;
                break;
            case 7: {
                _snprintf_s(txt, 128, _TRUNCATE, "HANDELSZUG kommt! Markt gefuellt."); typ = 6;
                static const IT handelsware[] = { IT::Salz,IT::Glas,IT::Seife,IT::Buch,IT::Schmuck,IT::Stoff,IT::Leder,IT::Keramik };
                for (IT it : handelsware) {
                    markt.addAngebot(-1, it, ri(3, 8), itBasiswert(it) * 1.2f, 0, 0);
                }
                break;
            }
            case 8: {
                _snprintf_s(txt, 128, _TRUNCATE, "BRAND! Ein Haus brennt nieder!"); typ = 1;
                if (!gebaeude.empty()) {
                    int idx = ri(0, (int)gebaeude.size() - 1);
                    if (gebaeude[idx].typ == GebTyp::Haus) {
                        welt.setTile(gebaeude[idx].wx, gebaeude[idx].wy, Tile::Flachland);
                        for (auto& p : plist) {
                            if (p->lebt && p->hausGebIdx == idx)p->hausGebIdx = -1;
                        }
                        gebaeude[idx].haltbarkeit = 0;
                    }
                }
                break;
            }
            default:return;
            }
            addLog(txt, typ);
        }
    }

    // ================================================================
    //  INIT
    // ================================================================
    void init(int n = 80) {
        plist.clear(); neuePersonen.clear(); gebaeude.clear(); logbuch.clear();
        geburten = tode = handels = gebaeudeAnz = raeuberfaelle = 0;
        zeit.tick = 0; gNextID = 1; gNextDorfID = 1; markt.init(); histBev.clear(); friedhof.clear();
        doerfer.clear();
        memset(&rekorde, 0, sizeof(rekorde)); memset(handelsvolumen, 0, sizeof(handelsvolumen));
        wetter = WetterSystem{};

        for (int cy = -5; cy <= 5; cy++) for (int cx = -5; cx <= 5; cx++) welt.getChunk(cx, cy);

        for (int ry = -20; ry <= 20; ry++) {
            Tile t = welt.getTile(14, ry);
            if (t != Tile::Berg && t != Tile::Wasser) {
                welt.setTile(14, ry, Tile::Fluss);
                int cx2, cy2, lx2, ly2; Welt::worldToChunk(14, ry, cx2, cy2, lx2, ly2);
                welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
            }
        }
        for (int ry = -20; ry <= 20; ry++) {
            Tile t = welt.getTile(-14, ry);
            if (t != Tile::Berg && t != Tile::Wasser) {
                welt.setTile(-14, ry, Tile::Fluss);
                int cx2, cy2, lx2, ly2; Welt::worldToChunk(-14, ry, cx2, cy2, lx2, ly2);
                welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
            }
        }
        for (int dy = -2; dy <= 2; dy++) for (int dx = -3; dx <= 3; dx++) {
            int wx2 = 18 + dx, wy2 = 18 + dy;
            if (welt.getTile(wx2, wy2) != Tile::Berg) {
                welt.setTile(wx2, wy2, Tile::See);
                int cx2, cy2, lx2, ly2; Welt::worldToChunk(wx2, wy2, cx2, cy2, lx2, ly2);
                welt.getChunk(cx2, cy2).ressLeft[ly2][lx2] = 9999;
                welt.getChunk(cx2, cy2).bebaut[ly2][lx2] = false;
            }
        }

        for (int i = 0; i < n; i++) {
            bool m = (rf() < .5f);
            int alt; float r = rf();
            if (r < .15f) alt = ri(1 * 12, 12 * 12);
            else if (r < .80f) alt = ri(18 * 12, 42 * 12);
            else alt = ri(43 * 12, 60 * 12);
            auto p = std::make_shared<Person>(zname(m), alt, m, 0, 0);
            if (!p->istKind())p->beruf = p->berechneberuf();
            p->inv.add(IT::Wasser, ri(25, 40));
            p->inv.add(IT::NahrIT, ri(8, 15));
            p->inv.add(IT::KleidungEinfach, 1);
            p->kleidHaltbarkeit[0] = ri(60, 100);
            p->werkzeugHaltbarkeit = ri(30, 60);
            // Jeder bekommt etwas Pelz als Grundschutz fuer den Winter
            if (rf() < 0.5f) p->inv.add(IT::Pelz, 1);
            p->durst = rf() * 10.f; p->hunger = rf() * 10.f;
            p->muenzen = rf() * 20 + 8;
            if (p->beruf == JB::Holzfaeller) { p->inv.add(IT::Holz, ri(5, 10)); }
            if (p->beruf == JB::Bergmann) { p->inv.add(IT::Erz, ri(3, 6)); p->inv.add(IT::Werkzeug, 1); p->werkzeugHaltbarkeit = 50; }
            if (p->beruf == JB::Schmied) { p->inv.add(IT::Werkzeug, ri(1, 3)); p->werkzeugHaltbarkeit = 60; }
            if (p->beruf == JB::Gerber) { p->inv.add(IT::Stoff, ri(2, 4)); p->inv.add(IT::Pelz, ri(1, 2)); }
            if (p->beruf == JB::Koch) { p->inv.add(IT::NahrIT, ri(5, 10)); }
            plist.push_back(p);
        }

        static const struct { IT it; int m; float prMul; } startware[] = {
            {IT::Holz,20,1.0f},{IT::Stein,15,1.0f},{IT::Erz,10,1.0f},
            {IT::Brett,10,1.0f},{IT::Werkzeug,5,1.0f},{IT::Stoff,8,1.0f},
            {IT::NahrIT,25,1.0f},{IT::Brot,10,1.0f},{IT::Wasser,30,0.8f},
            {IT::Seil,8,1.0f},{IT::Salz,10,1.0f},{IT::Keramik,6,1.0f},
            {IT::Seife,4,1.2f},{IT::Kerze,6,1.2f},{IT::KleidungEinfach,15,1.1f},
            {IT::KleidungWarm,14,1.5f},{IT::Medizin,5,1.3f},{IT::Leder,8,1.0f},
        };
        for (auto& sw : startware)
            markt.addAngebot(-1, sw.it, sw.m, itBasiswert(sw.it) * sw.prMul, 0, 0);

        Gebaeude mkt; mkt.typ = GebTyp::Marktstand; mkt.wx = 0; mkt.wy = 0;
        mkt.eigentuemerID = -1; mkt.level = 1; strcpy_s(mkt.name, 32, "Zentralmarkt");
        gebaeude.push_back(mkt); welt.setTile(0, 0, Tile::Markt); gebaeudeAnz = 1;

        {
            int idx = gruendeDorf(0, 0, -1);
            doerfer[idx].hatMarkt = true;
            strncpy_s(doerfer[idx].name, 32, "Mittelburg", _TRUNCATE);
            strncpy_s(doerfer[idx].spezialitat, 32, "Handel", _TRUNCATE);
            doerfer[idx].radius = 12;

            static const int startpos[][2] = { {35,-20},{-30,25},{20,35} };
            for (auto& sp : startpos) {
                int dx2 = sp[0], dy2 = sp[1];
                Tile t = welt.getTile(dx2, dy2);
                if (t != Tile::Wasser && t != Tile::Berg) {
                    gruendeDorf(dx2, dy2, -1);
                }
            }
            for (auto& p : plist) {
                if (p->distTo(0, 0) <= 15) trittDorfBei(*p, 0);
                else weiseDorfZu(*p);
            }
            int einsiedlerAnz = ri(2, 4);
            for (auto& p : plist) {
                if (einsiedlerAnz <= 0)break;
                if (p->dorfID < 0 || rf() < 0.05f) {
                    p->dorfID = -1; p->istEinsiedler = true;
                    p->modus = PersonModus::Einsiedler;
                    p->wx = ri(-80, 80); p->wy = ri(-80, 80);
                    einsiedlerAnz--;
                }
            }
        }
        addLog("Siedlung gegruendet! Wirtschaft & Realismus aktiv.", 3);
        addLog("Alle Waren muessen hergestellt oder gekauft werden!", 6);
    }

    // ================================================================
    //  SCHRITT
    // ================================================================
    void schritt() {
        zeit.tick++; markt.tick(zeit.saison()); wetter.tick(zeit.saison()); ereignisse();
        dorfTick();
        auswanderungsTick();

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

        if (zeit.saison() == 3 && zeit.tick % 24 == 0) {
            for (auto& g : gebaeude) {
                if (g.typ == GebTyp::Haus && g.holzVorrat > 0) { g.holzVorrat--; g.heizung = std::min(100, g.heizung + 5); }
                else g.heizung = std::max(0, g.heizung - 2);
            }
        }

        if (zeit.tick % 360 == 0) {
            for (auto& g : gebaeude) {
                if (g.typ == GebTyp::Marktstand) {
                    if (markt.angebot_s[(int)IT::Wasser] < 3) markt.addAngebot(-1, IT::Wasser, 5, 1.5f, 0, 0);
                    if (markt.angebot_s[(int)IT::Salz] < 2)   markt.addAngebot(-1, IT::Salz, 3, 5.f, 0, 0);
                }
            }
        }

        bool istNacht = (zeit.stunde() >= 22 || zeit.stunde() <= 5);
        bool istWinter = (zeit.saison() == 3);
        float nm = istWinter ? 1.3f : (zeit.saison() == 2) ? 1.1f : 1.0f;

        if (wetter.gibtRegen()) {
            for (auto& p : plist) {
                if (p->lebt && p->hausGebIdx < 0 && p->inv.anz(IT::Wasser) < 20)p->inv.add(IT::Wasser, 2);
                if (p->lebt && p->hausGebIdx < 0 && rf() < 0.05f && p->inv.hat(IT::Pflanze, 2))
                    p->inv.nimm(IT::Pflanze, 1);
            }
        }

        for (auto& p : plist) {
            if (!p->lebt)continue;
            if (zeit.tick % 720 == (p->id % 720))p->alter++;

            float hr = 0.28f * nm * (p->istKind() ? .7f : 1.f);
            p->hunger = fc(p->hunger + rf() * hr + hr * .18f, 0, 100);
            p->durst = fc(p->durst + rf() * .18f + .05f, 0, 100);
            p->energie = fc(p->energie - rf() * .35f - .1f, 0, 100);
            // Nachts: automatische Basis-Regeneration (auch wenn Person nicht explizit schlaeft)
            if (istNacht) p->energie = fc(p->energie + 8.f, 0, 100);
            p->stress = fc(p->stress + rf() * .12f, 0, 100);

            if (zeit.tick % 48 == p->id % 48)p->sauberkeit = fc(p->sauberkeit - rf() * 3 - 1, 0, 100);
            if (p->sauberkeit < 30)p->gesundheit = fc(p->gesundheit - 0.2f, 0, 100);

            kleidungVerschleiss(*p);
            steuerZahlen(*p);

            if (p->hausGebIdx >= 0 && p->hausGebIdx < (int)gebaeude.size()) {
                auto& haus = gebaeude[p->hausGebIdx];
                float schutz = fc((haus.isolierung + haus.heizung) / 120.f, 0.50f, 0.98f);
                p->gesundheit = fc(p->gesundheit + wetter.gesundheitsMod() * (1.f - schutz), 0, 100);
                if (istWinter && haus.heizung > 20)p->gesundheit = fc(p->gesundheit + 0.3f, 0, 100);
            }
            else {
                p->gesundheit = fc(p->gesundheit + wetter.gesundheitsMod(), 0, 100);
                if (wetter.verlangsamt() && p->hatZiel && rf() < 0.5f)p->bewegeZuZiel();
            }

            if (p->hunger > 72)p->gesundheit = fc(p->gesundheit - 0.7f, 0, 100);
            if (p->durst > 72) p->gesundheit = fc(p->gesundheit - 1.1f, 0, 100);
            // Natuerliche Gesundheits-Regen: wenn satt + nicht durstig + nicht dreckig
            if (p->hunger < 50 && p->durst < 40 && p->sauberkeit>35)
                p->gesundheit = fc(p->gesundheit + 0.15f, 0, 100);
            // Passive HP-Regen solange Gesundheit OK
            if (p->hp < 100 && p->gesundheit>40)
                p->hp = fc(p->hp + 0.08f, 0, 100);
            // HP sinkt nur bei wirklich kritischer Gesundheit (unter 15 statt 25)
            if (p->gesundheit < 15)p->hp = fc(p->hp - 0.3f, 0, 100);

            if (istWinter && p->hausGebIdx < 0) {
                // Schaden: ohne warme Kleidung maessig; mit warmer Kleidung kaum was
                float k = istNacht ? 0.35f : 0.12f;
                if (!p->hatWarmeKleidung())      p->gesundheit = fc(p->gesundheit - k, 0, 100);
                else if (!p->hatIrgendKleidung())p->gesundheit = fc(p->gesundheit - k * 0.5f, 0, 100);
                else                            p->gesundheit = fc(p->gesundheit - 0.03f, 0, 100);
            }

            sterbenCheck(*p); if (!p->lebt)continue;
            if (p->hatZiel)p->bewegeZuZiel();

            reisendenTick(*p);
            einsiedlerTick(*p);

            if (p->durst > 55) { wasserTrinken(*p); continue; }
            if (p->energie < 15) { schlafen(*p); continue; }
            if (p->hunger > 65) { essen(*p); continue; }
            if (p->energie < 35) { schlafen(*p); continue; }
            if (p->durst > 35) { wasserTrinken(*p); continue; }
            if (p->hunger > 50) { essen(*p); continue; }
            if (p->gesundheit < 40) { heilen(*p); continue; }
            if (p->sauberkeit < 25 && rf() < 0.3f) { waschen(*p); continue; }
            if (istWinter && !p->hatWarmeKleidung()) { kleidungBesorgen(*p); continue; }
            if (!p->hatIrgendKleidung()) { kleidungBesorgen(*p); continue; }
            if (p->hausGebIdx < 0 && !p->istKind() && !istWinter) { if (hausBauen(*p))continue; }
            if (istWinter && p->hausGebIdx >= 0 && p->inv.hat(IT::Holz, 3)) {
                if (p->hausGebIdx < (int)gebaeude.size()) {
                    auto& haus = gebaeude[p->hausGebIdx];
                    if (haus.holzVorrat < 10) { p->inv.nimm(IT::Holz, 3); haus.holzVorrat += 10; haus.heizung = std::min(100, haus.heizung + 20); strcpy_s(p->aktion, 80, "Heizt Haus"); }
                }
            }
            if (!p->inv.hat(IT::Werkzeug) && (p->beruf == JB::Bergmann || p->beruf == JB::Bergbaumeister)) {
                if (kaufeVomMarkt(*p, IT::Werkzeug)) { p->werkzeugHaltbarkeit = 50; strcpy_s(p->aktion, 80, "Kauft Werkzeug"); }
            }
            float rr = rf();
            if (rr < .52f)     arbeit(*p);
            else if (rr < .67f)tausch(*p);
            else if (rr < .82f)sozial(*p);
            else { SK s = (SK)ri(0, SK_COUNT - 1); p->verbSkill(s, .1f); _snprintf_s(p->aktion, 80, _TRUNCATE, "Uebt %s", skStr(s)); }
        }

        for (auto& np : neuePersonen) {
            plist.push_back(np);
            weiseDorfZu(*np);
        }
        neuePersonen.clear();

        if (zeit.tick % 24 == 0) {
            int leb = 0; for (auto& p : plist)if (p->lebt)leb++;
            histBev.push_back((float)leb);
            if (histBev.size() > 60)histBev.pop_front();
            if (leb > rekorde.maxBev)rekorde.maxBev = leb;
        }

        if (zeit.tick % 120 == 0) {
            for (auto& g : gebaeude) {
                if (g.typ == GebTyp::Haus && g.heizung > 20) {
                    for (auto& p : plist) {
                        if (p->lebt && p->hausGebIdx >= 0) {
                            int idx = p->hausGebIdx;
                            if (idx < (int)gebaeude.size() && gebaeude[idx].eigentuemerID == p->id) {
                                if (rf() < 0.2f)p->inv.add(IT::Asche, 1);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    // ================================================================
    //  STATISTIK
    // ================================================================
    struct Stats {
        int lebendig = 0, kinder = 0, paare = 0, haeuser = 0;
        float avgGlueck = 0, avgGesund = 0, avgHunger = 0, avgAlter = 0, avgBildung = 0;
        std::map<JB, int> berufe;
        int aelteste = 0; float reichste = 0; char reichsterName[32] = "";
        float tempAktuell = 0;
        int ohneKleidung = 0, ohneWerkzeug = 0;
    };
    Stats getStats()const {
        Stats s; int n = 0; s.tempAktuell = zeit.temperatur();
        for (auto& p : plist) {
            if (!p->lebt)continue;
            s.lebendig++; n++;
            if (p->istKind())s.kinder++;
            if (p->partnerID >= 0)s.paare++;
            if (p->hausGebIdx >= 0)s.haeuser++;
            s.avgGlueck += p->glueck; s.avgGesund += p->gesundheit;
            s.avgHunger += p->hunger; s.avgAlter += (float)p->alterJ();
            s.avgBildung += p->bildung;
            s.berufe[p->beruf]++;
            if (p->alterJ() > s.aelteste)s.aelteste = p->alterJ();
            float w = p->muenzen + p->inv.wert();
            if (w > s.reichste) { s.reichste = w; strncpy_s(s.reichsterName, 32, p->name, _TRUNCATE); }
            if (!p->hatIrgendKleidung())s.ohneKleidung++;
            if (!p->inv.hat(IT::Werkzeug) && (p->beruf == JB::Bergmann || p->beruf == JB::Schmied))s.ohneWerkzeug++;
        }
        if (n) { s.avgGlueck /= n; s.avgGesund /= n; s.avgHunger /= n; s.avgAlter /= n; s.avgBildung /= n; }
        s.paare /= 2; return s;
    }
};

// ================================================================
//  TUI
// ================================================================
struct TUI {
    Simulation sim;
    bool simLaeuft = false;
    int speed = 5, btnSel = 0; bool btnFokus = false;
    int lstSel = 0, lstScroll = 0; bool lstDetail = false;
    int aktPanel = 0;
    // FIX #7: PANELS=5 (0=Liste,1=Karte,2=Stats,3=Markt,4=Log, +V=Doerfer als Overlay auf 3)
    static const int PANELS = 5;
    int logScroll = 0;
    int dorfScroll = 0;
    int camX = 0, camY = 0;
    bool folgeSelected = false;
    bool zeigeDorfer = false; // Toggle fuer Doerfer-Panel anstelle Markt
    std::chrono::steady_clock::time_point lastTick;

    void run() {
        con.init();
        SetConsoleTitleA("Lebenssimulation v5.0 - Doerfer, Einsiedler, Haendlerrouten");
        sim.init(80); lastTick = std::chrono::steady_clock::now();
        while (true) {
            con.refreshSize();
            if (simLaeuft) {
                int ms = 1100 - speed * 100;
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count() >= ms) { sim.schritt(); lastTick = now; }
            }
            con.clear(); draw(con.W, con.H); con.flip();
            int k = con.getKey();
            if (k == -1) { Sleep(15); continue; }
            if (!input(k))break;
        }
        con.restore();
    }

    void draw(int W, int H) {
        int btnH = 3, botH = (std::max)(9, H / 4);
        int mainT = btnH + 1, mainH = H - mainT - botH;
        int leftW = (std::max)(24, W * 2 / 5), rightW = W - leftW;
        int statsW = W / 3, handelW = W / 3, logW = W - statsW - handelW;
        // FIX #2: drawTitle() implementiert als lokale Zeile
        drawTitle(0, W);
        drawButtons(1, 0, btnH, W);
        drawListe(mainT, 0, mainH, leftW);
        drawMap(mainT, leftW, mainH, rightW);
        drawStats(H - botH, 0, botH, statsW);
        if (zeigeDorfer)
            drawDoerfer(H - botH, statsW, botH, handelW);
        else
            drawMarkt(H - botH, statsW, botH, handelW);
        drawLog(H - botH, statsW + handelW, botH, logW);
        drawHint(H - 1, W);
    }

    // FIX #2: drawTitle als eigene Methode implementiert
    void drawTitle(int y, int W) {
        con.fillLine(y, ' ', C_HEADER);
        char zeitBuf[80]; sim.zeit.str(zeitBuf, 80);
        auto st = sim.getStats();
        char left[200];
        _snprintf_s(left, 200, _TRUNCATE, "  LEBENSSIM v5.0 DOERFER  |  %s  |  %.1f%cC  |  Bev:%d  Doerfer:%d",
            zeitBuf, st.tempAktuell, '\xF8', st.lebendig, (int)sim.doerfer.size());
        char right[120];
        _snprintf_s(right, 120, _TRUNCATE, " H:%d K:%d Geb:%d Tod:%d Deals:%d  ",
            st.haeuser, st.kinder, sim.geburten, sim.tode, sim.handels);
        con.print(0, y, left, C_HEADER);
        int rp = W - (int)strlen(right); if (rp > 0)con.print(rp, y, right, C_HEADER);
    }

    // ================================================================
    //  DORF-PANEL - FIX #4: Klammerstruktur korrekt
    // ================================================================
    void drawDoerfer(int y, int x, int h, int w) {
        bool act = (aktPanel == 3 && zeigeDorfer);
        con.boxTitle(x, y, w, h, "DOERFER & REISENDE", act);
        if (h < 4 || w < 10)return;
        int iw = w - 2, cy2 = y + 1;

        int einsiedler = 0, reisende = 0;
        for (auto& p : sim.plist) {
            if (!p->lebt)continue;
            if (p->istEinsiedler || p->modus == PersonModus::Einsiedler)einsiedler++;
            if (p->modus == PersonModus::Reisender)reisende++;
        }
        char kopf[80];
        _snprintf_s(kopf, 80, _TRUNCATE, "Doerfer:%d  Einsiedler:%d  Reisende:%d",
            (int)sim.doerfer.size(), einsiedler, reisende);
        con.print(x + 1, cy2++, kopf, C_ACCENT);
        if (cy2 >= y + h - 1)return;

        int rows = h - 3;
        int total = (int)sim.doerfer.size();
        if (dorfScroll > std::max(0, total - rows))dorfScroll = std::max(0, total - rows);
        int start = dorfScroll;

        char hdr[80]; _snprintf_s(hdr, 80, _TRUNCATE, "%-10s %-8s %3s %5s %-12s", "Name", "Typ", "Bew", "Wohlst", "Spezialit.");
        con.print(x + 1, cy2++, hdr, C_HEADER);
        if (cy2 >= y + h - 1)return;

        for (int i = start; i < total && cy2 < y + h - 2; i++) {
            const Dorf& d = sim.doerfer[i];
            int n = sim.lebendigeBewohner(i);
            if (n == 0) { cy2++; continue; }

            Person* selP = getSelectedPerson();
            bool istSelDorf = (selP && selP->dorfID == d.id);
            WORD base = istSelDorf ? C_SEL : C_DEFAULT;

            char row[80];
            _snprintf_s(row, 80, _TRUNCATE, "%-10.10s %-8s %3d %5.0f %-12.12s",
                d.name, Dorf::typStr(d.typ), n, d.wohlstand, d.spezialitat);
            con.print(x + 1, cy2, row, base);
            con.put(x + 1, cy2, d.symbol(), d.farbe());
            cy2++;
        }

        if (total > rows) {
            int pos = (int)((float)(rows - 1) * dorfScroll / (float)std::max(1, total - rows));
            for (int i = 0; i < rows; i++)con.put(x + w - 1, y + 2 + i, '|', C_DIM);
            con.put(x + w - 1, y + 2 + pos, '\xDB', C_ACCENT);
        }

        if (cy2 < y + h - 1) { for (int i = 1; i < iw + 1; i++)con.put(x + i, cy2++, '\xC4', C_BORDER); }
        if (cy2 < y + h - 1) {
            con.print(x + 1, cy2++, "Aktive Reisende:", C_HEADER);
        }
        for (auto& p : sim.plist) {
            if (cy2 >= y + h - 1)break;
            if (!p->lebt || p->modus != PersonModus::Reisender)continue;
            int zIdx = sim.dorfIndexVonID(p->reisezielDorfID);
            char rrow[80];
            _snprintf_s(rrow, 80, _TRUNCATE, "%-10.10s->%-8.8s(%d)",
                p->name,
                zIdx >= 0 ? sim.doerfer[zIdx].name : "???",
                p->reiseZaehler);
            con.print(x + 1, cy2++, rrow, C_HAENDLER_REISE);
        }
    } // Ende drawDoerfer

    void drawButtons(int y, int x, int h, int W) {
        for (int i = 0; i < W; i++)con.put(i, y + h - 1, '\xC4', C_BORDER);
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
            for (int i = 0; i < 10; i++)con.put(sx + 8 + i, y + 1, i < speed ? '\xDB' : '-', C_ACCENT);
            char spd[10]; _snprintf_s(spd, 10, _TRUNCATE, " %d/10", speed);
            con.print(sx + 18, y + 1, spd, C_ACCENT);
        }
        char ss[30]; _snprintf_s(ss, 30, _TRUNCATE, " Kamera:(%d,%d) ", camX, camY);
        con.print(W - 28, y + 1, ss, C_DIM);
        const char* st = simLaeuft ? ">> LAUFEND" : sim.zeit.tick > 0 ? "|| PAUSIERT" : "   GESTOPPT";
        WORD stc = simLaeuft ? C_OK : sim.zeit.tick > 0 ? C_WARN : C_DIM;
        con.print(W - 11, y + 1, st, stc);
    }

    Person* getSelectedPerson() {
        std::vector<int> alive;
        for (int i = 0; i < (int)sim.plist.size(); i++)if (sim.plist[i]->lebt)alive.push_back(i);
        if (alive.empty() || lstSel >= (int)alive.size())return nullptr;
        return sim.plist[alive[lstSel]].get();
    }

    void drawListe(int y, int x, int h, int w) {
        bool act = (aktPanel == 0);
        con.boxTitle(x, y, w, h, lstDetail ? "PERSON DETAIL" : "BEWOHNER", act);
        if (h < 4 || w < 10)return;
        int iw = w - 2, ih = h - 2;
        if (!lstDetail) {
            std::vector<int> alive;
            for (int i = 0; i < (int)sim.plist.size(); i++)if (sim.plist[i]->lebt)alive.push_back(i);
            int total = (int)alive.size();
            if (total == 0) { con.print(x + 1, y + 1, "Alle gestorben!", C_DANGER); return; }
            if (lstSel >= total)lstSel = total - 1;
            if (lstSel < lstScroll)lstScroll = lstSel;
            if (lstSel >= lstScroll + ih)lstScroll = lstSel - ih + 1;
            for (int row = 0; row < ih && row + lstScroll < total; row++) {
                int idx = alive[row + lstScroll];
                Person& p = *sim.plist[idx];
                bool sel = (row + lstScroll == lstSel);
                WORD base = sel ? C_SEL : C_DEFAULT;
                int nmw = iw - 22; if (nmw < 4)nmw = 4;
                char age[6];
                if (p.istKind())_snprintf_s(age, 6, _TRUNCATE, "%2dM", p.alter);
                else _snprintf_s(age, 6, _TRUNCATE, "%2dJ", p.alterJ());
                char warn[4] = "   ";
                if (!p.hatIrgendKleidung())warn[0] = '!';
                if (p.sauberkeit < 20)warn[1] = '~';
                if (p.inv.hat(IT::Buch))warn[2] = 'B';
                char line[128];
                _snprintf_s(line, 128, _TRUNCATE, "%-*.*s%s%s %-8s", nmw, nmw, p.name, age, warn, jbStr(p.beruf));
                con.print(x + 1, y + 1 + row, line, base);
                if (!sel) {
                    int hf = (int)(7 * p.hp / 100);
                    char hb[10]; for (int i = 0; i < 7; i++)hb[i] = (i < hf ? '\xDB' : '-'); hb[7] = 0;
                    WORD hc = p.hp > 60 ? C_OK : p.hp > 30 ? C_WARN : C_DANGER;
                    con.put(x + 1 + iw - 9, y + 1 + row, '[', hc);
                    con.print(x + 1 + iw - 8, y + 1 + row, hb, hc);
                    con.put(x + 1 + iw - 1, y + 1 + row, ']', hc);
                }
                if (sel)for (int fx = x + 1 + (int)strlen(line) + 1; fx < x + 1 + iw; fx++)con.put(fx, y + 1 + row, ' ', C_SEL);
            }
            if (total > ih) {
                int pos = (int)((float)(ih - 1) * lstScroll / (float)(std::max)(1, total - ih));
                for (int i = 0; i < ih; i++)con.put(x + w - 1, y + 1 + i, i == pos ? '\xDB' : '|', C_DIM);
            }
        }
        else {
            Person* pp = getSelectedPerson(); if (!pp) { con.print(x + 2, y + 2, "Niemand", C_WARN); return; }
            Person& p = *pp;
            int cy2 = y + 1;
            char hdr[64]; _snprintf_s(hdr, 64, _TRUNCATE, "%-*s", iw, p.name); con.print(x + 1, cy2++, hdr, C_ACCENT);
            char sub[80]; _snprintf_s(sub, 80, _TRUNCATE, "%dJ  %s  %s  %.1fM", p.alterJ(), p.maennl ? "M" : "W", jbStr(p.beruf), p.muenzen);
            con.print(x + 1, cy2++, sub, C_DIM);
            {
                const char* modStr = "Normal";
                if (p.modus == PersonModus::Einsiedler || p.istEinsiedler)modStr = "Einsiedler";
                else if (p.modus == PersonModus::Reisender)modStr = "Reisender";
                else if (p.modus == PersonModus::Auswanderer)modStr = "Auswanderer";
                char dorfname[32] = "kein Dorf";
                int dIdx = sim.dorfIndexVonID(p.dorfID);
                if (dIdx >= 0)strncpy_s(dorfname, 32, sim.doerfer[dIdx].name, _TRUNCATE);
                char dorfz[80];
                _snprintf_s(dorfz, 80, _TRUNCATE, "Dorf:%-10s  Modus:%-12s", dorfname, modStr);
                WORD dc = (p.istEinsiedler) ? C_EINSIEDLER : (p.modus == PersonModus::Reisender) ? C_HAENDLER_REISE : C_DIM;
                con.print(x + 1, cy2++, dorfz, dc);
                if (p.modus == PersonModus::Reisender) {
                    int zIdx = sim.dorfIndexVonID(p.reisezielDorfID);
                    char rziel[60];
                    _snprintf_s(rziel, 60, _TRUNCATE, "Reiseziel:%-10s  Weg:%d Ticks",
                        zIdx >= 0 ? sim.doerfer[zIdx].name : "???", p.reiseZaehler);
                    con.print(x + 1, cy2++, rziel, C_TRADE);
                }
            }
            char extra[80]; _snprintf_s(extra, 80, _TRUNCATE, "Kleid:%s Sauber:%d%% Bildung:%.0f",
                p.hatWarmeKleidung() ? "Warm" : (p.hatIrgendKleidung() ? "Einfach" : "KEINE!"),
                (int)p.sauberkeit, (float)p.bildung);
            con.print(x + 1, cy2++, extra, p.hatIrgendKleidung() ? C_DIM : C_DANGER);
            char wkz[80]; _snprintf_s(wkz, 80, _TRUNCATE, "Werkzeug: %s (%d/100) Steuer:%d",
                p.inv.hat(IT::Werkzeug) ? "JA" : "NEIN", p.werkzeugHaltbarkeit, p.steuerZaehler / 72);
            con.print(x + 1, cy2++, wkz, p.inv.hat(IT::Werkzeug) ? C_DIM : C_WARN);
            if (cy2 >= y + h - 1)return;
            auto sr = [&](const char* l, float v, WORD c) {
                if (cy2 >= y + h - 1)return;
                int bw = iw - 13; if (bw < 2)bw = 2;
                char lb[10]; _snprintf_s(lb, 10, _TRUNCATE, "%-8s", l);
                con.print(x + 1, cy2, lb, C_DIM); con.put(x + 9, cy2, '[', c);
                int f = (int)(bw * v / 100);
                for (int i = 0; i < bw; i++)con.put(x + 10 + i, cy2, i < f ? '\xDB' : '-', c);
                con.put(x + 10 + bw, cy2, ']', c);
                char pt[6]; _snprintf_s(pt, 6, _TRUNCATE, "%3d%%", (int)v);
                con.print(x + 11 + bw, cy2, pt, c); cy2++;
                };
            sr("HP", p.hp, C_OK); sr("Energie", p.energie, C_ACCENT);
            sr("Hunger", p.hunger, p.hunger > 70 ? C_DANGER : C_WARN);
            sr("Durst", p.durst, p.durst > 70 ? C_DANGER : C_WARN);
            sr("Glueck", p.glueck, C_OK); sr("Gesundh", p.gesundheit, p.gesundheit < 40 ? C_DANGER : C_OK);
            sr("Stress", p.stress, p.stress > 70 ? C_DANGER : C_WARN);
            sr("Sauber", p.sauberkeit, p.sauberkeit < 25 ? C_DANGER : C_DIM);
            if (cy2 < y + h - 1) { char ak[82]; _snprintf_s(ak, 82, _TRUNCATE, ">> %s", p.aktion); con.print(x + 1, cy2++, ak, C_LOG); }
            if (cy2 < y + h - 2)con.print(x + 1, cy2++, "Inventar:", C_DIM);
            for (int i = 0; i < IT_COUNT && cy2 < y + h - 1; i++) {
                if (p.inv.items[i] > 0) {
                    char iv[40]; _snprintf_s(iv, 40, _TRUNCATE, "  %-12s:%3d", itStr((IT)i), p.inv.items[i]);
                    con.print(x + 1, cy2++, iv, C_CHART);
                }
            }
            if (cy2 < y + h - 2)con.print(x + 1, cy2++, "Top Skills:", C_DIM);
            std::vector<std::pair<float, int>> sv;
            for (int i = 0; i < SK_COUNT; i++)sv.push_back({ p.skills[i],i });
            std::sort(sv.rbegin(), sv.rend());
            for (int i = 0; i < 5 && cy2 < y + h - 1; i++) {
                int bw = (std::max)(1, iw - 18);
                int fill = (int)(bw * sv[i].first / 100);
                char sl[16]; _snprintf_s(sl, 16, _TRUNCATE, "%-14s", skStr((SK)sv[i].second));
                con.print(x + 1, cy2, sl, C_DIM);
                for (int b = 0; b < bw; b++)con.put(x + 15 + b, cy2, b < fill ? '\xDB' : '-', C_CHART);
                char sv2[8]; _snprintf_s(sv2, 8, _TRUNCATE, "%5.1f", sv[i].first);
                con.print(x + 15 + bw, cy2, sv2, C_CHART); cy2++;
            }
        }
    }

    void drawMap(int y, int x, int h, int w) {
        bool act = (aktPanel == 1);
        con.boxTitle(x, y, w, h, "WELT (Pfeile=Kamera F=Folgen V=Doerfer)", act);
        if (h < 4 || w < 8)return;
        int dW = w - 2, dH = h - 2;
        if (folgeSelected) { Person* pp = getSelectedPerson(); if (pp) { camX = pp->wx - (dW / 2); camY = pp->wy - (dH / 2); } }

        for (int dy = 0; dy < dH; dy++) {
            for (int dx = 0; dx < dW; dx++) {
                int wx = camX + dx, wy = camY + dy;
                Tile t = sim.welt.getTile(wx, wy);
                WORD col = tileColor(t); char c = tileChar(t);
                if ((t == Tile::Wald || t == Tile::Huegel || t == Tile::Berg) && sim.welt.getRess(wx, wy) <= 2)col = C_DIM;
                if (t == Tile::Fluss && (dx + dy) % 2 == 0)col = C_RIVER | 0x80;
                con.put(x + 1 + dx, y + 1 + dy, c, col);
            }
        }

        for (auto& d : sim.doerfer) {
            // FIX #6: dorfIndexVonID koennte -1 liefern, aber hier iterieren wir direkt
            int ddx = d.wx - camX, ddy = d.wy - camY;
            if (ddx >= 0 && ddx < dW && ddy >= 0 && ddy < dH) {
                con.put(x + 1 + ddx, y + 1 + ddy, d.symbol(), d.farbe());
            }
            if (ddx + 1 >= 0 && ddx + 1 + 8 < dW && ddy >= 0 && ddy < dH) {
                char lbl[20]; _snprintf_s(lbl, 20, _TRUNCATE, "%-7.7s", d.name);
                con.print(x + 1 + ddx + 1, y + 1 + ddy, lbl, d.farbe());
            }
            int n2 = (int)d.bewohnerIDs.size();
            if (ddx >= 0 && ddx < dW && ddy + 1 >= 0 && ddy + 1 < dH) {
                char nbuf[8]; _snprintf_s(nbuf, 8, _TRUNCATE, "[%d]", n2);
                con.print(x + 1 + ddx, y + 1 + ddy + 1, nbuf, C_DIM);
            }
        }

        std::map<std::pair<int, int>, std::pair<int, PersonModus>> personPos;
        for (auto& p : sim.plist) {
            if (!p->lebt)continue;
            int dx = p->wx - camX, dy = p->wy - camY;
            if (dx >= 0 && dx < dW && dy >= 0 && dy < dH) {
                auto& slot = personPos[{dx, dy}];
                slot.first++;
                if (p->modus == PersonModus::Reisender || p->modus == PersonModus::Einsiedler)
                    slot.second = p->modus;
            }
        }
        for (auto& pp : personPos) {
            int dx = pp.first.first, dy = pp.first.second;
            int cnt = pp.second.first;
            PersonModus mod = pp.second.second;
            char c; WORD col;
            if (mod == PersonModus::Reisender) { c = 'R'; col = C_HAENDLER_REISE; }
            else if (mod == PersonModus::Einsiedler) { c = 'E'; col = C_EINSIEDLER; }
            else {
                c = (cnt > 9) ? '+' : ('0' + cnt);
                col = cnt > 5 ? C_DANGER : cnt > 2 ? C_WARN : C_PERSON;
            }
            con.put(x + 1 + dx, y + 1 + dy, c, col);
        }

        Person* selP = getSelectedPerson();
        if (selP) {
            int dx = selP->wx - camX, dy = selP->wy - camY;
            if (dx >= 0 && dx < dW && dy >= 0 && dy < dH)con.put(x + 1 + dx, y + 1 + dy, '@', C_BTNSEL);
            if (selP->hatZiel) {
                int tdx = selP->zielX - camX, tdy = selP->zielY - camY;
                if (tdx >= 0 && tdx < dW && tdy >= 0 && tdy < dH)
                    con.put(x + 1 + tdx, y + 1 + tdy, 'x', C_TRADE);
            }
        }

        if (dH > 4)con.print(x + 1, y + h - 2,
            ".Feld T=Wald H=Haus D=Dorf G=Grossdorf #=Stadt w=Weiler R=Reisend E=Einsiedler", C_DIM);
        char coord[40]; _snprintf_s(coord, 40, _TRUNCATE, "(%d,%d) %dDoerfer", camX, camY, (int)sim.doerfer.size());
        con.print(x + w - 1 - (int)strlen(coord) - 1, y, coord, C_DIM);
    }

    void drawStats(int y, int x, int h, int w) {
        bool act = (aktPanel == 2);
        con.boxTitle(x, y, w, h, "STATISTIK", act);
        if (h < 4 || w < 10)return;
        auto st = sim.getStats();
        int cy2 = y + 1, iw = w - 2;
        char kz[128]; _snprintf_s(kz, 128, _TRUNCATE, "Bev:%d (K:%d)  Paare:%d  H:%d  OhneKleid:%d",
            st.lebendig, st.kinder, st.paare, st.haeuser, st.ohneKleidung);
        con.print(x + 1, cy2++, kz, C_ACCENT); if (cy2 >= y + h - 1)return;
        WORD tc = st.tempAktuell < 0 ? C_ACCENT : st.tempAktuell>25 ? C_DANGER : C_OK;
        char ts[80]; _snprintf_s(ts, 80, _TRUNCATE, "T:%.1f%cC  Ald:%dJ  Bildung:%.0f%%  Reichst:%s(%.0f)",
            st.tempAktuell, '\xF8', st.aelteste, st.avgBildung, st.reichsterName, st.reichste);
        con.print(x + 1, cy2++, ts, tc); if (cy2 >= y + h - 1)return;
        auto sb = [&](const char* l, float v, WORD c) {
            if (cy2 >= y + h - 1)return;
            int bw = iw - 13; if (bw < 4)bw = 4;
            int f = (int)(bw * v / 100);
            char lb[12]; _snprintf_s(lb, 12, _TRUNCATE, "%-8s[", l);
            con.print(x + 1, cy2, lb, C_DIM);
            for (int i = 0; i < bw; i++)con.put(x + 10 + i, cy2, i < f ? '\xDB' : '-', c);
            char pt[8]; _snprintf_s(pt, 8, _TRUNCATE, "]%3d%%", (int)v);
            con.print(x + 10 + bw, cy2, pt, c); cy2++;
            };
        sb("Glueck", st.avgGlueck, C_OK);
        sb("Gesundh", st.avgGesund, st.avgGesund < 40 ? C_DANGER : C_OK);
        sb("Hunger", st.avgHunger, st.avgHunger > 70 ? C_DANGER : C_WARN);
        if (cy2 < y + h - 3 && !sim.histBev.empty()) {
            if (cy2 < y + h - 1) { for (int i = 1; i < iw + 1; i++)con.put(x + i, cy2, '\xC4', C_BORDER); cy2++; }
            con.print(x + 1, cy2++, "Bev-Verlauf:", C_HEADER);
            if (cy2 < y + h - 1) {
                float mx = 0, mn = 9999;
                for (auto v : sim.histBev) { if (v > mx)mx = v; if (v < mn)mn = v; }
                if (mx - mn < 1)mx = mn + 1;
                int cw = iw - 2, n = (int)sim.histBev.size();
                for (int i = 0; i < cw && i < n; i++) {
                    float val = sim.histBev[(int)((float)i * n / cw)];
                    int bH = (int)(4 * (val - mn) / (mx - mn));
                    char c2 = bH >= 3 ? '\xDB' : bH >= 2 ? '\xB2' : bH >= 1 ? '\xB1' : '\xB0';
                    con.put(x + 1 + i, cy2, c2, val > sim.histBev[0] * 0.9f ? C_OK : C_WARN);
                }
                char mm[32]; _snprintf_s(mm, 32, _TRUNCATE, "[%.0f-%.0f]", mn, mx);
                con.print(x + 1 + cw - (int)strlen(mm), cy2, mm, C_DIM); cy2++;
            }
        }
        if (cy2 < y + h - 1) { for (int i = 1; i < iw + 1; i++)con.put(x + i, cy2, '\xC4', C_BORDER); cy2++; }
        if (cy2 < y + h - 2)con.print(x + 1, cy2++, "Berufe:", C_HEADER);
        int maxC = 1; for (auto& b : st.berufe)if (b.second > maxC)maxC = b.second;
        int bwJ = (std::min)(iw - 20, 18);
        for (auto& b : st.berufe) {
            if (cy2 >= y + h - 1)break;
            int f = (int)((float)bwJ * b.second / maxC);
            char jb[12]; _snprintf_s(jb, 12, _TRUNCATE, "%-10s", jbStr(b.first));
            con.print(x + 1, cy2, jb, C_DIM);
            for (int i = 0; i < bwJ; i++)con.put(x + 11 + i, cy2, i < f ? '\xDB' : '-', C_CHART);
            char cnt[6]; _snprintf_s(cnt, 6, _TRUNCATE, "%3d", b.second);
            con.print(x + 11 + bwJ, cy2, cnt, C_CHART); cy2++;
        }
    }

    void drawMarkt(int y, int x, int h, int w) {
        bool act = (aktPanel == 3 && !zeigeDorfer);
        con.boxTitle(x, y, w, h, "MARKT & PREISE", act);
        if (h < 4 || w < 10)return;
        int cy2 = y + 1, iw = w - 2;
        int saison = sim.zeit.saison();
        char ks[80]; _snprintf_s(ks, 80, _TRUNCATE, "Deals:%d  Ang:%d  Saisonmod:%s",
            sim.handels, (int)sim.markt.liste.size(),
            saison == 0 ? "Fruehling" : saison == 1 ? "Sommer" : saison == 2 ? "Herbst" : "Winter");
        con.print(x + 1, cy2++, ks, C_ACCENT); if (cy2 >= y + h - 1)return;
        if (cy2 < y + h - 1)con.print(x + 1, cy2++, "Ware       Preis  Ang/Nfr", C_HEADER);
        static const IT wl[] = {
            IT::NahrIT,IT::Brot,IT::Suppentopf,IT::Wasser,IT::Holz,IT::Brett,IT::Stein,
            IT::Erz,IT::Kohle,IT::Werkzeug,IT::Stoff,IT::Leder,IT::Seil,
            IT::KleidungEinfach,IT::KleidungWarm,IT::Medizin,IT::Seife,IT::Kerze,
            IT::Bier,IT::Honig,IT::Salz,IT::Keramik,IT::Glas,IT::Buch,IT::Gold,IT::Waffe
        };
        for (IT it : wl) {
            if (cy2 >= y + h - 1)break;
            int i = (int)it;
            float pr = sim.markt.getPreis(it, saison);
            float ang = sim.markt.angebot_s[i], nfr = sim.markt.nachfrage_s[i];
            float ratio = nfr / (ang > 0.01f ? ang : 0.01f);
            WORD pc = ratio > 1.5f ? C_DANGER : ratio > 0.9f ? C_WARN : C_OK;
            char arr = ratio > 1.3f ? '\x18' : ratio < 0.7f ? '\x19' : '-';
            int bw = 6;
            int af = (int)(bw * ang / (ang + nfr + 0.01f));
            char row[64]; _snprintf_s(row, 64, _TRUNCATE, "%-9s%5.1fM %c ", itStr(it), pr, arr);
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
        if (h < 4 || w < 10)return;
        int iw = w - 2, rows = h - 2;
        auto& lg = sim.logbuch;
        int total = (int)lg.size();
        int maxS = (std::max)(0, total - rows);
        if (logScroll > maxS)logScroll = maxS;
        int start = total - rows - logScroll; if (start < 0)start = 0;
        for (int i = 0; i < rows && start + i < total; i++) {
            const LogEintrag& e = lg[start + i];
            WORD c = C_DIM;
            switch (e.typ) {
            case 1:c = C_DANGER; break; case 2:c = C_OK; break; case 3:c = C_WARN; break;
            case 4:c = C_ACCENT; break; case 5:c = C_CHART; break; case 6:c = C_TRADE; break;
            }
            char line[256]; _snprintf_s(line, 255, _TRUNCATE, "%-*.*s", iw, iw, e.text.c_str());
            con.print(x + 1, y + 1 + i, line, c);
        }
        if (total > rows) {
            for (int i = 0; i < rows; i++)con.put(x + w - 1, y + 1 + i, '|', C_DIM);
            int pos = rows - 1 - (int)((float)(rows - 1) * logScroll / (float)(std::max)(1, maxS));
            con.put(x + w - 1, y + 1 + pos, '\xDB', C_ACCENT);
        }
    }

    void drawHint(int y, int) {
        con.fillLine(y, ' ', C_HEADER);
        con.print(0, y, " TAB:Panel  Pfeile:Nav/Kamera  ENTER:Detail  F=Folgen  1:Start  2:Pause  3:Reset  +/-:Speed  S:Schritt  V=Doerfer  G=GehZuDorf  Q:Quit", C_HEADER);
    }

    bool input(int k) {
        if (k == 'q' || k == 'Q')return false;
        if (k == '\t') { btnFokus = false; aktPanel = (aktPanel + 1) % PANELS; return true; }
        if (k == 1020) { btnFokus = false; aktPanel = (aktPanel + PANELS - 1) % PANELS; return true; }
        if (k == 1000) {
            if (btnFokus)btnSel = (std::max)(0, btnSel - 1);
            else if (aktPanel == 0 && !lstDetail)lstSel = (std::max)(0, lstSel - 1);
            else if (aktPanel == 1)camY--;
            else if (aktPanel == 4)logScroll = (std::min)(logScroll + 1, (int)sim.logbuch.size());
            else if (zeigeDorfer && aktPanel == 3)dorfScroll = (std::max)(0, dorfScroll - 1);
            return true;
        }
        if (k == 1001) {
            if (btnFokus) { btnSel = (std::min)(4, btnSel + 1); }
            else if (aktPanel == 0 && !lstDetail) { int al = 0; for (auto& p : sim.plist)if (p->lebt)al++; lstSel = (std::min)((std::max)(0, al - 1), lstSel + 1); }
            else if (aktPanel == 1)camY++;
            else if (aktPanel == 4)logScroll = (std::max)(0, logScroll - 1);
            else if (zeigeDorfer && aktPanel == 3)dorfScroll = (std::min)(dorfScroll + 1, (int)sim.doerfer.size());
            return true;
        }
        if (k == 1002) { if (btnFokus)btnSel = (std::max)(0, btnSel - 1); else if (aktPanel == 1)camX--; return true; }
        if (k == 1003) { if (btnFokus)btnSel = (std::min)(4, btnSel + 1); else if (aktPanel == 1)camX++; return true; }
        if (k == 1004) {
            if (aktPanel == 0)lstSel = (std::max)(0, lstSel - 10);
            else if (aktPanel == 1)camY -= 5;
            else if (aktPanel == 4)logScroll = (std::min)(logScroll + 10, (int)sim.logbuch.size());
            return true;
        }
        if (k == 1005) {
            if (aktPanel == 0) { int al = 0; for (auto& p : sim.plist)if (p->lebt)al++; lstSel = (std::min)((std::max)(0, al - 1), lstSel + 10); }
            else if (aktPanel == 1)camY += 5;
            else if (aktPanel == 4)logScroll = (std::max)(0, logScroll - 10);
            return true;
        }
        if (k == '\r') {
            if (btnFokus) {
                switch (btnSel) {
                case 0:simLaeuft = true; if (sim.zeit.tick == 0)sim.init(80); break;
                case 1:simLaeuft = false; break;
                case 2:simLaeuft = false; sim.init(80); break;
                case 3:speed = (std::min)(10, speed + 1); break;
                case 4:speed = (std::max)(1, speed - 1); break;
                }
            }
            else if (aktPanel == 0)lstDetail = !lstDetail;
            return true;
        }
        if (k == '1') { simLaeuft = true; if (sim.zeit.tick == 0)sim.init(80); return true; }
        if (k == '2') { simLaeuft = false; return true; }
        if (k == '3') { simLaeuft = false; sim.init(80); return true; }
        if (k == '+' || k == '=') { speed = (std::min)(10, speed + 1); return true; }
        if (k == '-' || k == '_') { speed = (std::max)(1, speed - 1); return true; }
        if (k == 'b' || k == 'B') { btnFokus = !btnFokus; return true; }
        if (k == 's' || k == 'S') { if (!simLaeuft)sim.schritt(); return true; }
        if (k == 'f' || k == 'F') { folgeSelected = !folgeSelected; return true; }
        if (k == 'c' || k == 'C') { Person* pp = getSelectedPerson(); if (pp) { camX = pp->wx - (con.W / 3) / 2; camY = pp->wy - (con.H / 3) / 2; }return true; }
        if (k == 'w' || k == 'W') { camY -= 3; return true; }
        if (k == 'a' || k == 'A') { camX -= 3; return true; }
        if (k == 'x' || k == 'X' || k == 'y') { camY += 3; return true; }
        // FIX #7: V togglet Doerfer-Panel (kein separates Panel mehr noetig)
        if (k == 'v' || k == 'V') { zeigeDorfer = !zeigeDorfer; aktPanel = 3; return true; }
        if (k == 'g' || k == 'G') {
            Person* pp = getSelectedPerson();
            if (pp && pp->dorfID >= 0) {
                int dIdx = sim.dorfIndexVonID(pp->dorfID);
                if (dIdx >= 0) {
                    camX = sim.doerfer[dIdx].wx - (con.W / 3) / 2;
                    camY = sim.doerfer[dIdx].wy - (con.H / 3) / 2;
                }
            }
            return true;
        }
        if (k == 'd' || k == 'D') { camX += 3; return true; }
        return true;
    }
};

int main() {
    TUI ui;
    ui.run();
    return 0;
}