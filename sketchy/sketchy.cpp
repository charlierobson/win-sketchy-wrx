#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "sketchyIf.h"

#include "buttons.h"
#include "regions.h"
#include "dfile.h"

#include "sinv.h"

static const int pw = 32;
static const int ph = 64;

class sketchy : public olc::PixelGameEngine, sketchyIf
{
private:
	std::vector<region*> _regions;

	std::map<std::string, std::pair<buttonRegion*, button*>> _clickables;

	std::vector<dfile*> _dfiles;
	int _dfilePage = 0;

	int _mode = 0;
	int _curChar = 0;

	copyBuffer _copyBuffer;

	olc::Pixel _selectColour = olc::DARK_YELLOW;

	dfile* _dfile;

private:
	void DoFileOp(std::function<void(LPOPENFILENAME)> action) {
		OPENFILENAME ofn;
		char filename[MAX_PATH];

		ZeroMemory(&ofn, sizeof(ofn));
		ZeroMemory(&filename, sizeof(filename));

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFilter = "Sketchy Sprite Files (*.ssf)\0*.ssf\0Any File\0*.*\0";
		ofn.lpstrFile = filename;
		ofn.lpstrDefExt = "ssf";
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = "S81ect a File";
		ofn.Flags = OFN_DONTADDTORECENT;

		action(&ofn);
	}

	void DoCopy() {
		_copyBuffer = _dfile->copy();
	}

public:
	sketchy()
	{
		sAppName = "Even Sketchier ZX81 wrx editor V1.1";
	}

	void setMode(int mode) {
		_mode = mode;
	}

	int getMode() {
		return _mode;
	}

	void setCurChar(int curr) {
		_curChar = curr;
	}

	int getCurChar() {
		return _curChar;
	}

	void setPage(int p) {
		_dfilePage = p;
		_dfile = _dfiles[_dfilePage];
	}

	dfile* getDFile() {
		return _dfile;
	}

	copyBuffer& getCopyBuffer() {
		return _copyBuffer;
	}

	void ClickButton(std::string buttonName) override {
		std::pair<buttonRegion*, button*> regionalWineLady = _clickables[buttonName];
		if (regionalWineLady.first != nullptr)
			regionalWineLady.first->select(regionalWineLady.second);
	}


	void SaveToClipboard(std::string content) {
		if (!OpenClipboard(reinterpret_cast<olc::Platform_Windows*>(olc::platform.get())->olc_hWnd))
			return;

		EmptyClipboard();

		auto data = GlobalAlloc(GMEM_MOVEABLE, content.size() + 1);
		if (data == nullptr)
			return;

		auto locked = GlobalLock(data);
		if (locked == nullptr) {
			GlobalFree(data);
			return;
		}

		memset(locked, 0, content.size() + 1);
		memcpy(locked, content.c_str(), content.size());
		GlobalUnlock(data);

		SetClipboardData(CF_TEXT, data);
		CloseClipboard();
	}

	std::string GetFromClipboard() {
		std::string result("");

		if (!IsClipboardFormatAvailable(CF_TEXT))
			return result;

		if (!OpenClipboard(reinterpret_cast<olc::Platform_Windows*>(olc::platform.get())->olc_hWnd))
			return result;

		auto clip = GetClipboardData(CF_TEXT);
		if (clip != NULL) {
			auto lptstr = GlobalLock(clip);
			if (lptstr != NULL)
			{
				result.assign((const char*)lptstr);
				GlobalUnlock(clip);
			}
		}

		CloseClipboard();

		return result;
	}

	void rotateCopyBuffer() {
		auto srcData = std::vector<int>(_copyBuffer.data);

		auto srcw = _copyBuffer.w;
		auto srch = _copyBuffer.h;

		_copyBuffer.w = _copyBuffer.h;
		_copyBuffer.h = srcw;

		for (int y = 0; y < srch; ++y)
			for (int x = 0; x < srcw; ++x) {
				auto dest = _copyBuffer.w - 1 - y + x * _copyBuffer.w;
				_copyBuffer.data[dest] = srcData[x + y * srcw];
			}
	}

public:
	void OnDropFile(const std::string& filename) override
	{
		_dfile->load(filename);
	}

	bool OnUserCreate() override
	{
		auto sprite = new olc::Sprite("zx81font.png");

		for (int i = 0; i < 4; ++i) {
			_dfiles.push_back(new dfile(sprite, pw, ph));
		}

		_dfile = _dfiles[0];

		_regions.push_back(new dfileRegion(this));

		auto row = 8;
		int basex = (pw * 4) + 8;

		auto workButtons = new buttonRegion();

		auto blockButton = new textButton(basex, row, _dfile, "DRAW", [this]() {setMode(1); });
		workButtons->add(blockButton);
		workButtons->select(blockButton);

		auto selectButton = new textButton(basex, row += 12, _dfile, "SELECT", [this]() {setMode(2); });
		workButtons->add(selectButton);
		_clickables["select"] = std::pair<buttonRegion*, button*>(workButtons, selectButton);

		workButtons->add(new textButton(basex, row += 12, _dfile, "COPY", [this]() {
			DoCopy();
			}, false));

		auto pasteButton = new textButton(basex, row += 12, _dfile, "PASTE", [this]() {
			if (getMode() == 5) {
				getDFile()->setOpaquePaste(!getDFile()->getOpaquePaste());
			}
			else {
				setMode(5);
				_copyBuffer.pos = olc::vi2d(0, 0);
			}
			});
		workButtons->add(pasteButton);
		_clickables["paste"] = std::pair<buttonRegion*, button*>(workButtons, pasteButton);

		row += 12;

		workButtons->add(new textButton(basex, row += 12, _dfile, "CLS", [this]() {
				_dfile->cls();
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "INVERT", [this]() {
			_dfile->invert();
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "FILL", [this]() {
			_dfile->fill(1);
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "UNFILL", [this]() {
			_dfile->fill(0);
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "ROTATE", [this]() {
			rotateCopyBuffer();
			}, false));

		row += 12;

		workButtons->add(new textButton(basex, row += 12, _dfile, "->CLIP", [this]() {
			int w, h;
			_dfile->getSpriteExtent(w, h);
			SaveToClipboard(_dfile->serialise());
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "<-CLIP", [this]() {
			_dfile->deserialise(GetFromClipboard());
			}, false));

		row += 12;

		workButtons->add(new textButton(basex, row += 12, _dfile, "LOAD", [this]() {
			DoFileOp([this](LPOPENFILENAMEA ofn) {
				if (GetOpenFileNameA(ofn)) {
					_dfile->load(ofn->lpstrFile);
				}
				});
			}, false));

		workButtons->add(new textButton(basex, row += 12, _dfile, "SAVE", [this]() {
			DoFileOp([this](LPOPENFILENAMEA ofn) {
				if (GetSaveFileNameA(ofn)) {
					_dfile->save(ofn->lpstrFile);
				}
				});
			}, false));

		_regions.push_back(workButtons);

		row += 24;

		auto pageButtons = new buttonRegion();
		for (int i = 0; i < 4; ++i) {
			auto b = new pageButton(basex + i * 10, row, _dfile, 0x1d + i,
				[this](int c) {
					setPage(c - 0x1d);
				},
				[this](int c)->bool {
					return _dfilePage == c - 0x1d;
				} );
			pageButtons->add(b);

			if (i == 0) {
				pageButtons->select(b);
			}
		};

		_regions.push_back(pageButtons);

		_copyBuffer.pos = olc::vi2d(0, 0);
		_copyBuffer.w = 32;
		_copyBuffer.h = 64;
		_copyBuffer.data.assign(std::begin(sinv), std::end(sinv));

		return true;
	}

	struct olc::Pixel getSelectColour() {
		return _selectColour;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		static float totalTime;
		totalTime += fElapsedTime;

		_selectColour = ((int)(totalTime * 2) & 1) == 0 ? olc::Pixel(0xFF2266FF) : olc::Pixel(0xFF0011CC);

		Clear(olc::Pixel(0xFF808080));

		olc::vi2d mousePos(GetMouseX(), GetMouseY());
		olc::HWButton lButton = GetMouse(0);
		olc::HWButton rButton = GetMouse(1);

		if (GetKey(olc::Key::F1).bPressed) setPage(0);
		if (GetKey(olc::Key::F2).bPressed) setPage(1);
		if (GetKey(olc::Key::F3).bPressed) setPage(2);
		if (GetKey(olc::Key::F4).bPressed) setPage(3);

		for (auto region : _regions) {
			if (region->isWithin(mousePos.x, mousePos.y)) {
				region->update(this);
			}
			region->draw(this);
		}

		return true;
	}
};


int main()
{
	sketchy demo;
	if (demo.Construct(pw * 4 + 8 * 8, 8 + ph * 4, 2, 2))
		demo.Start();

	return 0;
};
