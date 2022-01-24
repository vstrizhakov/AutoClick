#include "pch.h"
#include <Windows.h>
#include <thread>
#include <WinUser.h>
#include <ShellScalingApi.h>
#include "Interception.h"
#include "InputManager.h"

using namespace InputSimulator;
using namespace std;

InputManager::InputManager()
{
	_inputManager = this;
}

void InputManager::Initialize()
{
	_context = interception_create_context();
	if (!_context)
	{
		throw gcnew System::InvalidOperationException("Interception driver isn't installed");
	}
	for (int i = 0; i < INTERCEPTION_MAX_DEVICE; i++)
	{
		if (interception_is_keyboard(i) && _keyboardDevice == -1)
		{
			_keyboardDevice = i;
		}
		if (_mouseDevice == -1 && interception_is_mouse(i))
		{
			_mouseDevice = i;
		}
	}

	thread trh(KeyboardHook);
	_keyboardHookThread = &trh;
	trh.detach();
}

void InputManager::Uninitialize()
{
	_keyboardDevice = -1;
	_mouseDevice = -1;
	interception_destroy_context(_context);
}

void InputManager::KeyboardHook()
{
	if (!_inputManager)
	{
		return;
	}

	InterceptionDevice device;
	InterceptionContext context = _inputManager->_context;
	InterceptionStroke stroke;

	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
	while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
	{
		if (interception_is_keyboard(device))
		{
			InterceptionKeyStroke& keyStroke = *(InterceptionKeyStroke*)&stroke;
			if (keyStroke.state & INTERCEPTION_KEY_UP)
			{
				_inputManager->ProcessKeyboardInput(keyStroke.code);
			}
		}
		interception_send(context, device, &stroke, 1);
	}
}

void InputManager::ProcessKeyboardInput(unsigned short keyCode)
{
	KeyPressed(this, MapVirtualKeyA(keyCode, MAPVK_VSC_TO_VK));
}

void InputManager::SendString(String^ string)
{
	if (_keyboardDevice != -1)
	{
		int totalLength = string->Length * 2;
		InterceptionKeyStroke* keyStrokes = new InterceptionKeyStroke[totalLength];
		for (int i = 0; i < totalLength; i += 2)
		{
			char keyCode = string[i / 2];
			int intCode = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);
			// KeyDown
			InterceptionKeyStroke keyDown;
			keyDown.code = intCode;
			keyDown.state = InterceptionKeyState::INTERCEPTION_KEY_DOWN;
			keyDown.information = 0;
			*(keyStrokes + i) = keyDown;
			// KeyUp
			InterceptionKeyStroke keyUp;
			keyUp.code = intCode;
			keyUp.state = InterceptionKeyState::INTERCEPTION_KEY_UP;
			keyUp.information = 0;
			*(keyStrokes + i + 1) = keyUp;
		}
		interception_send(_context, _keyboardDevice, (InterceptionStroke*)keyStrokes, totalLength);
	}
}

void InputManager::SendKeyDown(unsigned short keyCode)
{
	InterceptionKeyStroke keyDown;
	keyDown.code = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);
	keyDown.state = InterceptionKeyState::INTERCEPTION_KEY_DOWN;
	keyDown.information = 0;
	interception_send(_context, _keyboardDevice, (InterceptionStroke*)&keyDown, 1);
}

void InputManager::SendKeyUp(unsigned short keyCode)
{
	InterceptionKeyStroke keyUp;
	keyUp.code = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);
	keyUp.state = InterceptionKeyState::INTERCEPTION_KEY_UP;
	keyUp.information = 0;
	interception_send(_context, _keyboardDevice, (InterceptionStroke*)&keyUp, 1);
}

int get_screen_width()
{
	return GetSystemMetrics(SM_CXSCREEN);
}

int get_screen_height()
{
	return GetSystemMetrics(SM_CYSCREEN);
}

void InputManager::SendLeftMouseClick(double x, double y)
{
	SetCursorPos(x, y);
	INPUT mouseInput;
	mouseInput.type = INPUT_MOUSE;
	mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &mouseInput, sizeof(mouseInput));
	mouseInput.type = INPUT_MOUSE;
	mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &mouseInput, sizeof(mouseInput));
}