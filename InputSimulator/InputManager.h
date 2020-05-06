#pragma once

using namespace System;
using namespace std;

namespace InputSimulator {
	public delegate void KeyPressedEventHandler(System::Object^ sender, unsigned short keyCode);

	public ref class InputManager
	{
	public:
		InputManager();

		void Initialize();
		void Uninitialize();
		void SendString(String^ string);
		void SendKeyUp(unsigned short keyCode);
		void SendKeyDown(unsigned short keyCode);
		void SendLeftMouseClick(double x, double y);

		event KeyPressedEventHandler^ KeyPressed;
	private:
		InterceptionContext _context;
		InterceptionDevice _keyboardDevice = -1;
		InterceptionDevice _mouseDevice = -1;

		thread* _keyboardHookThread;

		static InputManager^ _inputManager;
		static void KeyboardHook();
		void ProcessKeyboardInput(unsigned short keyCode);
	};
}
