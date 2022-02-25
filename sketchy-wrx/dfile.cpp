#include "olcPixelGameEngine.h"
#include "dfile.h"
#include <regex>
#include <iomanip>

dfile::dfile(olc::Sprite* charSet, int w, int h) :
    _charSet(charSet)
{
    _w = w;
    _h = h;

    _dfile.resize(_w * _h);
    _selStart = olc::vi2d(0, 0);
    _selEnd = olc::vi2d(_w - 1, _h - 1);
    _opaquePaste = true;
}

dfile::dfile(const dfile& other) {
}


int dfile::ascii2zeddy(int c) {
    const char* zeddycs = " !!!!!!!!!!\"\xa3$:?()><=+-*/;,.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    auto idx = strrchr(zeddycs, toupper(c));
    if (idx == nullptr || *idx == '!') {
        return -1;
    }
    return idx - zeddycs;
}

void getFirstAndLast(std::vector<int> dfile, int y, int w, int& f, int& l) {
    f = w + 1;
    l = -1;
    for (int x = 0; x < w; ++x) {
        if (dfile[x + y * w]) {
            f = x;
            l = x;
            break;
        }
    }
    for (int x = w - 1; x >= 0; --x) {
        if (dfile[x + y * w]) {
            l = x;
            break;
        }
    }
}


void dfile::draw(olc::PixelGameEngine* pge) {
    int xo = 4;
    int yo = 4;

    auto lcol = olc::Pixel(0xFFF0F0F0);

    for (int y = 0; y < _h; ++y) {
        for (int x = 0; x < _w; ++x) {
            //int f, l;
            //getFirstAndLast(_dfile, y, _w, f, l);

            auto xc = x / 8;
            auto yc = y / 8;
            auto dcol = (xc & 1) ^ (yc & 1) ? olc::Pixel(0xFFD0D0D0) : olc::Pixel(0xFFE0E0E0);
            if (_dfile[x + _w * y] == 0) {
                //if (x >= f && x <= l)
                //    pge->FillRect(x * 4 + xo, y * 4 + yo, 4, 4, olc::WHITE);
                //else 
                    pge->FillRect(x * 4 + xo, y * 4 + yo, 4, 4, (x & 1) ^ (y & 1) ? lcol : dcol);
            }
            else
                pge->FillRect(x * 4 + xo, y * 4 + yo, 4, 4, olc::BLACK);
        }
    }

    int w, h;
    getSpriteExtent(w, h);
    if (w + h != 0)
        pge->DrawRect(4, 4, (((w-1) / 8) + 1) * 8 * 4, h * 4, olc::GREY);
}


void dfile::minimap(olc::PixelGameEngine* pge, int xo, int yo) {

    pge->FillRect(xo, yo, _w, _h, olc::WHITE);

    for (int y = 0; y < _h; ++y) {
        for (int x = 0; x < _w; ++x) {
            if (_dfile[x + _w * y] != 0) {
                pge->Draw(x + xo , y + yo, olc::BLACK);
            }
        }
    }
}


void dfile::plot(int x, int y) {
    _dfile[x + _w * y] = 1;
}

void dfile::unplot(int x, int y) {
    _dfile[x + _w * y] = 0;
}

void dfile::cls() {
    std::fill(_dfile.begin(), _dfile.end(), 0);
}


void dfile::fill(int c) {

    olc::vi2d start, end;
    getSelectRectNormal(start, end);

    for (int row = start.y; row <= end.y; ++row) {
        for (int x = start.x; x <= end.x; ++x) {
            _dfile[row * _w + x] = c;
        }
    }
}


void dfile::load(const std::string& filename) {

    byte w, h, m;
    int n = 0;

    std::ifstream sprbin(filename, std::ios::binary);
    sprbin >> std::noskipws;

    if (sprbin.is_open()) {
        sprbin >> w;
        sprbin >> h;

        while (!sprbin.eof()) {
            sprbin >> m;
            if (n < _w * _h)
                _dfile[n++] = m;
        }
    }
}


void dfile::save(const std::string& filename) {

    std::ofstream sprbin(filename, std::ios::binary);
    if (sprbin.is_open())
    {
        sprbin << (byte)_w;
        sprbin << (byte)_h;
        for (int n = 0; n < _w * _h; ++n) {
            sprbin << (byte)_dfile[n];
        }
        sprbin.close();
    }
}


std::string dfile::serialise() {

    std::stringstream output;
    std::stringstream comment;

    int bw, bh;
    getSpriteExtent(bw, bh);
    if (bw + bh == 0)
        return "";

    bw = ((bw - 1) / 8) + 1;

    for (int line = 0; line < bh; ++line) {
        comment.str("");
        output << "\t.byte\t";
        for (int c = 0; c < bw; ++c) {
            byte b = 0;
            for (int bit = 7; bit >= 0; --bit) {
                int x = _dfile[c * 8 + (7 - bit) + line * _w];
                b |= x ? 1 << bit : 0;
                comment << (x ? "#" : " ");
            }

            output << "$" << std::hex << std::setw(2) << std::setfill('0') << (int)b;
            if (c != bw - 1) {
                output << ",";
            }
        }
        output << "\t\t; '" << comment.str() << "'" << std::endl;
    }

    return output.str();
}



void dfile::deserialise(std::string input) {

    std::string line;
    std::stringstream instr(input);
    std::regex reg((const char*)"\\$[0-9A-Fa-f]{2}");

    auto lineNum = 0;
    while (std::getline(instr, line, '\n')) {
        auto c = 0;
        for (std::sregex_iterator it = std::sregex_iterator(line.begin(), line.end(), reg); it != std::sregex_iterator(); ++it) {
            std::smatch match = *it;
            auto mc = match.str().substr(1, 2);
            auto m = std::strtol(mc.c_str(), nullptr, 16);
            for (int bit = 7; bit >= 0; --bit) {
                _dfile[c * 8 + (7 - bit) + lineNum * _w] = m & (1 << bit) ? 1 : 0;
            }
            ++c;
        }

        ++lineNum;
    }
}



copyBuffer dfile::copy() {

    olc::vi2d end;
    copyBuffer cb;

    getSelectRectNormal(cb.pos, end);

    cb.w = end.x - cb.pos.x + 1;
    cb.h = end.y - cb.pos.y + 1;
    cb.data.resize(cb.w * cb.h);
    for (int y = 0; y < cb.h; ++y) {
        for (int x = 0; x < cb.w; ++x) {
            cb.data[x + cb.w * y] = _dfile[cb.pos.x + x + (cb.pos.y + y) * _w];
        }
    }
    return cb;
}


void dfile::invert() {

    olc::vi2d end;
    copyBuffer cb;

    getSelectRectNormal(cb.pos, end);

    cb.w = end.x - cb.pos.x + 1;
    cb.h = end.y - cb.pos.y + 1;
    cb.data.resize(cb.w * cb.h);
    for (int y = 0; y < cb.h; ++y) {
        for (int x = 0; x < cb.w; ++x) {
            _dfile[cb.pos.x + x + (cb.pos.y + y) * _w] = _dfile[cb.pos.x + x + (cb.pos.y + y) * _w] ^ 1;
        }
    }
}


void dfile::paste(copyBuffer& cb) {

    int sxo = 0;
    int syo = 0;

    auto pos = cb.pos;

    int xc = std::min(cb.w, _w - pos.x);
    int yc = std::min(cb.h, _h - pos.y);

    if (pos.x < 0) {
        sxo = -pos.x;
        xc += pos.x;
        pos.x = 0;
    }
    if (pos.y < 0) {
        syo = -pos.y;
        yc += pos.y;
        pos.y = 0;
    }

    for (int x = 0; x < xc; x++)
        for (int y = 0; y < yc; y++) {
            auto c = cb.data[x + sxo + (y + syo) * cb.w];
            if (!_opaquePaste && c == 0)
                continue;

            _dfile[pos.x + x + (pos.y + y) * _w] = c;
        }
}


void dfile::draw(olc::PixelGameEngine* pge, copyBuffer& cb) {

    int xo = 4;
    int yo = 4;
    int sxo = 0;
    int syo = 0;

    auto pos = cb.pos;

    int xc = std::min(cb.w, _w - pos.x);
    int yc = std::min(cb.h, _h - pos.y);

    if (pos.x < 0) {
        sxo = -pos.x;
        xc += pos.x;
        pos.x = 0;
    }
    if (pos.y < 0) {
        syo = -pos.y;
        yc += pos.y;
        pos.y = 0;
    }

    for (int x = 0; x < xc; x++)
        for (int y = 0; y < yc; y++) {
            auto c = cb.data[x + sxo + (syo + y) * cb.w];

            if (!_opaquePaste && c == 0)
                continue;

            pge->FillRect((pos.x + x) * 4 + xo, (pos.y + y) * 4 + yo, 4, 4, c ? olc::BLACK : olc::WHITE);
        }

    pge->DrawRect(pos.x * 4 + xo, pos.y * 4 + yo, xc * 4 - 1, yc * 4 - 1, olc::RED);
}
