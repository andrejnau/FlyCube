#pragma once

#include <Windows.h>

class IDXSample
{
public:
	virtual ~IDXSample() {}

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual UINT GetWidth() const = 0;
	virtual UINT GetHeight() const = 0;
};