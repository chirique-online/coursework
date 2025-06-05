#ifndef LIGHT_H
#define LIGHT_H

#include "MyOGL.h"

class Light
{
	double posX = 19;
	double posY = 0.225;
	double posZ = 5.6;

	bool drag = false;
	bool from_camera = false;

public:

	double x() const
	{
		return  posX;
	}
	double y() const
	{
		return  posY;
	}
	double z() const
	{
		return  posZ;
	}

	void SetPosition(double x, double y, double z);

	void StartDrug(OpenGL* sender, KeyEventArg arg);

	void StopDrug(OpenGL* sender, KeyEventArg arg);

	void MoveLight(OpenGL* sender, MouseEventArg arg);

	void SetUpLight();

	void DrawLightGizmo();

};


#endif