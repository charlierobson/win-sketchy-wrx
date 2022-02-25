#include "olcPixelGameEngine.h"
#include "sketchyIf.h"

#include "regions.h"
#include "buttons.h"
#include "dfile.h"

buttonRegion::buttonRegion() {
	region();
	_currentSelection = nullptr;
}

void buttonRegion::add(button* button) {
	_buttons.push_back(button);
	extend(button->_topLeft, button->_bottomRight);
}

void buttonRegion::select(button* button) {
	if (_currentSelection != nullptr && _currentSelection != button) {
		_currentSelection->select(false);
	}

	_currentSelection = button;
	_currentSelection->select(true);
}

void buttonRegion::update(olc::PixelGameEngine* pge) {
	
	olc::vi2d mousePos(pge->GetMouseX(), pge->GetMouseY());
	auto lButton = pge->GetMouse(0);

	for (button* button : _buttons) {
		if (button->checkSelect(mousePos, lButton)) {
			select(button);
		}
	}
}

void buttonRegion::draw(olc::PixelGameEngine* pge) {
	for (button* button : _buttons) {
		button->draw(pge);
	}
}

void buttonRegion::onMouseLeave() {
	for (button* button : _buttons) {
		button->_highlighted = false;
	}
}



dfileRegion::dfileRegion(sketchyIf* core) :
	region(4, 4, core->getDFile()->getW() * 4 + 4, core->getDFile()->getH() * 4 + 4),
	_core(core) {
}

void dfileRegion::update(olc::PixelGameEngine* pge) {
	olc::vi2d mousePos(pge->GetMouseX() - 4, pge->GetMouseY() - 4);

	auto lButton = pge->GetMouse(0);
	auto rButton = pge->GetMouse(1);

	static olc::vi2d startPos, endPos;

	switch (_core->getMode()) {
	case 1:
	{
		auto subPixelX = mousePos.x / 4;
		auto subPixelY = mousePos.y / 4;
		if (lButton.bHeld) {
			_core->getDFile()->plot(subPixelX, subPixelY);
		}
		else if (rButton.bHeld) {
			_core->getDFile()->unplot(subPixelX, subPixelY);
		}
	}
	break;
	case 2:
	{
		if (lButton.bPressed) {
			_core->getDFile()->setSelectRect(mousePos / 4, mousePos / 4);
		}
		else if (lButton.bHeld) {
			olc::vi2d tl, br;
			_core->getDFile()->getSelectRect(tl, br);
			_core->getDFile()->setSelectRect(tl, mousePos / 4);
		}
	}
	break;

	case 5:
	{
		if (lButton.bPressed) {
			startPos = mousePos / 4;
		}
		if (lButton.bHeld) {
			endPos = mousePos / 4;
			_core->getCopyBuffer().pos += endPos - startPos;
			startPos = endPos;
		}
		if (rButton.bPressed) {
			_core->getDFile()->paste(_core->getCopyBuffer());
			_core->getCopyBuffer().pos = olc::vi2d(0, 0);
		}
	}
	break;

	default:
		break;
	}
}

void dfileRegion::draw(olc::PixelGameEngine* pge) {
	_core->getDFile()->draw(pge);

	if (_core->getMode() == 5) {
		_core->getDFile()->draw(pge, _core->getCopyBuffer());
	}
	else if (_core->getMode() > 1) {
		olc::vi2d offs(4, 4);
		olc::vi2d start, end;
		_core->getDFile()->getSelectRectNormal(start, end);
		auto dim = olc::vi2d(end.x - start.x + 1, end.y - start.y + 1);
		pge->DrawRect(start * 4 + offs, dim * 4, _core->getSelectColour());
	}
}

void dfileRegion::onMouseLeave() {
	if (_core->getMode() > 1) {

	}
}

