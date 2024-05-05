#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#include <d3d9.h>
#include <d3dx9shader.h>

namespace DebugMenu
{
	struct TextInfo
	{
		const char*text;
		RECT rect;
	};

	class cDebugMenu
	{
		IDirect3DDevice9* pDeviceRef;
		

	public:
		ID3DXFont *pFont;//debug menu font pointer
		float debugItemIndex;
		TextInfo debugItems[8];
		cDebugMenu(IDirect3DDevice9* i_direct3DDevice)
		{
			//initialize
			debugItemIndex = 0.0f;
			pDeviceRef = i_direct3DDevice;
			pFont = NULL;
			HRESULT result = D3DXCreateFont(pDeviceRef,     //D3D Device

				80,               //Font height

				0,                //Font width

				FW_NORMAL,        //Font Weight

				1,                //MipLevels

				false,            //if Italic

				DEFAULT_CHARSET,  //CharSet

				OUT_DEFAULT_PRECIS, //OutputPrecision

				ANTIALIASED_QUALITY, //Quality

				DEFAULT_PITCH | FF_DONTCARE,//PitchAndFamily

				"Arial",          //pFacename,

				&pFont);         //ppFont
			if (FAILED(result))
			{
				return;
			}
			int i = 0;
			while (i < 8)
			{
				SetRect(&debugItems[i].rect, 0, i * 100, 200, 100);
				i++;
			}
			debugItems[0].text = "Frame Rate";
			debugItems[1].text = "Camera Fly Speed";
			debugItems[2].text = "Reset Camera(press space bar to reset)";
			debugItems[3].text = "Enable/Disable Sphere";
			debugItems[4].text = "Enable/Disable Dynamic Possible Collision Environment :";
			debugItems[5].text = "Enable/Disable Octree Display:";
			debugItems[6].text = "Sound Effect Volume:";
			debugItems[7].text = "Background Music Volume:";
		}
		~cDebugMenu()
		{
			pDeviceRef = NULL;
			if (pFont){
				pFont->Release();
				pFont = NULL;
			}
			int i = 0;
			
		}
		
	};
}
#endif