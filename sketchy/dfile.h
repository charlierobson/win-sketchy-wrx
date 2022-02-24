#pragma once
#include "olcPixelGameEngine.h"


struct copyBuffer
{
	olc::vi2d pos;
	int w, h;
	std::vector<int> data;
};


class dfile
{
private:
	std::vector<int> _dfile;

	olc::vi2d _selStart;
	olc::vi2d _selEnd;

	olc::Sprite* _charSet;
	bool _opaquePaste;

	int _w, _h;

public:
	dfile(olc::Sprite*, int, int);
	dfile(const dfile& other);

	olc::Sprite* charSet() { return _charSet; }

	void draw(olc::PixelGameEngine*);

	void cls();

	void plot(int x, int y);
	void unplot(int x, int y);

	void setSelectRect(olc::vi2d start, olc::vi2d end) {
		_selStart = start;
		_selEnd = end;
	}

	void getSelectRect(olc::vi2d& start, olc::vi2d& end) {
		start = _selStart;
		end = _selEnd;
	}

	void getSelectRectNormal(olc::vi2d& start, olc::vi2d& end) {
		start.x = std::min(_selStart.x, _selEnd.x);
		start.y = std::min(_selStart.y, _selEnd.y);
		end.x = std::max(_selStart.x, _selEnd.x);
		end.y = std::max(_selStart.y, _selEnd.y);
	}

	int getW() {
		return _w;
	}

	int getH() {
		return _h;
	}

	static int ascii2zeddy(int c);

	void fill(int c);

	void load(const std::string& filename);
	void save(const std::string& filename);

	std::string serialise();

	copyBuffer copy();
	void invert();

	void paste(copyBuffer&);
	void draw(olc::PixelGameEngine* pge, copyBuffer& cb);

	bool getOpaquePaste() {
		return _opaquePaste;
	}

	void setOpaquePaste(bool opaquePaste) {
		_opaquePaste = opaquePaste;
	}

	void getSpriteExtent(int& pixW, int& pixH) {
		pixW = 0;
		pixH = 0;
		for (int row = 0; row < _h; ++row) {
			for (int col = _w - 1; col >= 0; --col) {
				if (_dfile[col + _w * row]) {
					if (col + 1 > pixW) {
						pixW = col + 1;
					}
					pixH = row + 1;
					break;
				}
			}
		}
		return;
	}

	void deserialise(std::string);
};
