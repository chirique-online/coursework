#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "Texture.h"

#include "ObjLoader.h"

#include "debout.h"

//внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

float view_matrix[16];
double full_time = 0;
int location = 0;

bool texturing = true;
bool lightning = true;
bool alpha = true;


void switchModes(OpenGL* sender, KeyEventArg arg)
{
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case 'L':
		lightning = !lightning;
		break;
	case 'T':
		texturing = !texturing;
		break;
	case 'A':
		alpha = !alpha;
		break;
	}
}

//умножение матриц c[M1][N1] = a[M1][N1] * b[M2][N2]
template<typename T, int M1, int N1, int M2, int N2>
void MatrixMultiply(const T* a, const T* b, T* c)
{
	for (int i = 0; i < M1; ++i)
	{
		for (int j = 0; j < N2; ++j)
		{
			c[i * N2 + j] = T(0);
			for (int k = 0; k < N1; ++k)
			{
				c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
			}
		}
	}
}

void setBlackMaterial() {
	float amb[] = { 0.01f, 0.01f, 0.01f, 1.0f };
	float dif[] = { 0.03f, 0.03f, 0.03f, 1.0f };
	float spec[] = { 0.12f, 0.12f, 0.12f, 1.0f };
	float sh = 0.2f * 128;

	glMaterialfv(GL_FRONT, GL_AMBIENT, amb); glMaterialfv(GL_FRONT, GL_DIFFUSE, dif); glMaterialfv(GL_FRONT, GL_SPECULAR, spec); glMaterialf(GL_FRONT, GL_SHININESS, sh);
}

void setWhiteShinyMaterial() {
	float amb[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float spec[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float sh = 72.0f;

	glMaterialfv(GL_FRONT, GL_AMBIENT, amb); glMaterialfv(GL_FRONT, GL_DIFFUSE, dif); glMaterialfv(GL_FRONT, GL_SPECULAR, spec); glMaterialf(GL_FRONT, GL_SHININESS, sh);
}

void setWhiteMatteMaterial() {
	float amb[] = { 0.55f, 0.55f, 0.55f, 1.0f };
	float dif[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	float spec[] = { 0.05f, 0.05f, 0.05f, 1.0f };
	float sh = 10.0f;

	glMaterialfv(GL_FRONT, GL_AMBIENT, amb); glMaterialfv(GL_FRONT, GL_DIFFUSE, dif); glMaterialfv(GL_FRONT, GL_SPECULAR, spec); glMaterialf(GL_FRONT, GL_SHININESS, sh);
}

GuiTextRectangle text;

GLuint texId;

ObjModel f, model, coffepot;

Shader phong_sh;
Shader blackening_sh;
Shader simple_texture_sh;

Texture model_tex, black, coffepot_tex, outer_door, inner_door, glass, panel, tile;

// функция для вычисления нормали к плоскости по трем точкам
void calculateNormal(float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3,
	float& nx, float& ny, float& nz) {

	float v1x = x2 - x1;
	float v1y = y2 - y1;
	float v1z = z2 - z1;

	float v2x = x3 - x1;
	float v2y = y3 - y1;
	float v2z = z3 - z1;

	nx = v1y * v2z - v1z * v2y;
	ny = v1z * v2x - v1x * v2z;
	nz = v1x * v2y - v1y * v2x;

	float length = sqrt(nx * nx + ny * ny + nz * nz);
	if (length > 0) {
		nx /= length;
		ny /= length;
		nz /= length;
	}
}

// функция для отрисовки квада с автоматическим расчетом нормали
void drawQuadWithNormals(float x1, float y1, float z1, float u1, float v1,
	float x2, float y2, float z2, float u2, float v2,
	float x3, float y3, float z3, float u3, float v3,
	float x4, float y4, float z4, float u4, float v4) {
	float nx, ny, nz;
	calculateNormal(x1, y1, z1, x2, y2, z2, x3, y3, z3, nx, ny, nz);

	glBegin(GL_QUADS);
	glNormal3f(nx, ny, nz);

	glTexCoord2f(u1, v1); glVertex3f(x1, y1, z1);
	glTexCoord2f(u2, v2); glVertex3f(x2, y2, z2);
	glTexCoord2f(u3, v3); glVertex3f(x3, y3, z3);
	glTexCoord2f(u4, v4); glVertex3f(x4, y4, z4);

	glEnd();
}


void ruchka() {
	const int number = 13;
	double radius = 0.55;
	double length = 0.3;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double y = radius * cos(angle);
		double z = radius * sin(angle);

		double normal[] = { 0.0, y / radius, z / radius };
		glNormal3dv(normal);

		glVertex3d(0.0, y, z);
		glVertex3d(length, y, z);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(1.0, 0.0, 0.0);
	glVertex3d(length, 0.0, 0.0);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(length, radius * cos(angle), radius * sin(angle));
	}
	glEnd();

	radius = 0.4;
	length = 0.5;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double y = radius * cos(angle);
		double z = radius * sin(angle);

		double normal[] = { 0.0, y / radius, z / radius };
		glNormal3dv(normal);

		glVertex3d(0.0, y, z);
		glVertex3d(length, y, z);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(1.0, 0.0, 0.0);
	glVertex3d(length, 0.0, 0.0);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(length, radius * cos(angle), radius * sin(angle));
	}
	glEnd();
}

void dvernyie_petli() {
	const int number = 7;
	double radius = 0.1;
	double height = 1.6;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double x = radius * cos(angle);
		double y = radius * sin(angle);


		double normal[] = { x / radius, y / radius, 0.0 };
		glNormal3dv(normal);

		glVertex3d(x, y, 0.0);
		glVertex3d(x, y, height);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, -1.0);
	glVertex3d(0.0, 0.0, 0.0);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(radius * cos(angle), radius * sin(angle), 0.0);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, 1.0);
	glVertex3d(0.0, 0.0, height);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(radius * cos(angle), radius * sin(angle), height);
	}
	glEnd();
}

void krutilka() {
	const int number = 8;
	double radius = 0.6;
	double height = 0.3;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double x = radius * cos(angle);
		double y = radius * sin(angle);


		double normal[] = { x / radius, y / radius, 0.0 };
		glNormal3dv(normal);

		glVertex3d(x, y, 0.0);
		glVertex3d(x, y, height);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.4, 1.0);
	glVertex3d(0.0, 0.0, height);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(radius * cos(angle), radius * sin(angle), height);
	}
	glEnd();
}

void nozhka() {
	const int number = 9;
	double radiusBottom = 0.175; double radiusTop = 0.4;
	double height = 0.3;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double cosA = cos(angle);
		double sinA = sin(angle);

		double xBottom = radiusBottom * cosA;
		double yBottom = radiusBottom * sinA;

		double xTop = radiusTop * cosA;
		double yTop = radiusTop * sinA;

		double normalX = cosA;
		double normalY = sinA;
		double normalZ = (radiusBottom - radiusTop) / height;
		glNormal3d(normalX, normalY, normalZ);

		glVertex3d(xBottom, yBottom, 0.0);
		glVertex3d(xTop, yTop, height);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, -1.0);
	glVertex3d(0.0, 0.0, 0.0);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(radiusBottom * cos(angle), radiusBottom * sin(angle), 0.0);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, 1.0);
	glVertex3d(0.0, 0.0, height);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glVertex3d(radiusTop * cos(angle), radiusTop * sin(angle), height);
	}
	glEnd();
}

// параметры вращения двери
float doorAngle = 0.0f;
const float maxDoorAngle = 105.0f;
const float doorSpeed = 90.0f;
bool isOpening = false;
bool isMoving = false;

void updateDoorAnimation(double delta_time) {
	static bool wasKeyPressed = false;
	if (gl.isKeyPressed('O') && !wasKeyPressed) {
		isMoving = !isMoving;  // старт/пауза
		wasKeyPressed = true;

		// если дверь в крайнем положении, меняем направление
		if (doorAngle >= maxDoorAngle) isOpening = false;
		else if (doorAngle <= 0) isOpening = true;
	}
	if (!gl.isKeyPressed('O')) wasKeyPressed = false;

	if (isMoving) {
		doorAngle += (isOpening ? doorSpeed : -doorSpeed) * delta_time;

		// остановка при достижении границ
		if (doorAngle >= maxDoorAngle) {
			doorAngle = maxDoorAngle;
			isMoving = false;
		}
		else if (doorAngle <= 0) {
			doorAngle = 0;
			isMoving = false;
		}
	}
}

void drawDoor() {
	glPushMatrix();
	glTranslatef(3.3f, -4.0f, 0.0f);
	glRotatef(-doorAngle, 0.0f, 0.0f, 1.0f);
	glTranslatef(-3.3f, 4.0f, 0.0f);

	drawQuadWithNormals(3.3f, -4.0f, -2.7f, 1, 1, //НИЖНИЙ КУСОК ДВЕРИ
		2.6f, -4.0f, -2.7f, 1, 0,
		2.6f, 4.0f, -2.7f, 0, 0,
		3.3f, 4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(3.3f, 4.0f, 3.0f, 1, 1, 	//ВЕРХНИЙ КУСОК ДВЕРИ
		2.6f, 4.0f, 3.0f, 1, 0,
		2.6f, -4.0f, 3.0f, 0, 0,
		3.3f, -4.0f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, 4.0f, -2.7f, 1, 1, 	//ВНУТРЕННИЙ КУСОК ДВЕРИ
		2.6f, 4.0f, -2.7f, 1, 0,
		2.6f, 4.0f, 3.0f, 0, 0,
		3.3f, 4.0f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, -4.0f, 3.0f, 1, 1,	//ЛЕВЫЙ КУСОК ДВЕРИ
		2.6f, -4.0f, 3.0f, 1, 0,
		2.6f, -4.0f, -2.7f, 0, 0,
		3.3f, -4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(2.6f, 4.0f, -1.7f, 1, 1,	//ПЕРЕДНИЙ НИЖНИЙ КУСОК ДВЕРИ СЗАДИ

		2.6f, 4.0f, -2.7f, 1, 0,
		2.6f, -4.0f, -2.7f, 0, 0,
		2.6f, -4.0f, -1.7f, 0, 1);

	drawQuadWithNormals(3.3f, -2.7f, -2.7f, 1, 1,	//ЛЕВЫЙ ВЕРХНИЙ КУСОК ДВЕРИ
		3.3f, -2.7f, 3.0f, 1, 0,
		3.3f, -4.0f, 3.0f, 0, 0,
		3.3f, -4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(2.6f, -2.7f, 3.0f, 1, 1,	//ЛЕВЫЙ ВЕРХНИЙ КУСОК ДВЕРИ СЗАДИ
		2.6f, -2.7f, -2.7f, 1, 0,
		2.6f, -4.0f, -2.7f, 0, 0,
		2.6f, -4.0f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, 4.0f, -1.7f, 1, 1,	//ПРАВЫЙ ВЕРХНИЙ КУСОК ДВЕРИ
		3.3f, 4.0f, 2.0f, 1, 0,
		3.3f, 2.7f, 2.0f, 0, 0,
		3.3f, 2.7f, -1.7f, 0, 1);

	drawQuadWithNormals(2.6f, 4.0f, 3.0f, 1, 1,	//ПРАВЫЙ ВЕРХНИЙ КУСОК ДВЕРИ СЗАДИ
		2.6f, 4.0f, -2.7f, 1, 0,
		2.6f, 2.7f, -2.7f, 0, 0,
		2.6f, 2.7f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, 2.7f, 2.0f, 1, 1,	//ПРАВЫЙ ВНУТРЕННИЙ КУСОК ДВЕРИ
		2.6f, 2.7f, 2.0f, 1, 0,
		2.6f, 2.7f, -1.7f, 0, 0,
		3.3f, 2.7f, -1.7f, 0, 1);

	drawQuadWithNormals(3.3f, -2.7f, -1.7f, 1, 1,	//ЛЕВЫЙ ВНУТРЕННИЙ КУСОК ДВЕРИ
		2.6f, -2.7f, -1.7f, 1, 0,
		2.6f, -2.7f, 2.0f, 0, 0,
		3.3f, -2.7f, 2.0f, 0, 1);

	//ВЫДВИНУТАЯ ВПЕРЕД ЧАСТЬ СПРАВА
	drawQuadWithNormals(3.3f, 2.7f, -2.7f, 1, 1,	//СТЕНОЧКА СПРАВА
		3.7f, 2.7f, -2.3f, 1, 0,
		3.7f, 2.7f, 2.6f, 0, 0,
		3.3f, 2.7f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, 4.0f, 3.0f, 1, 1,	//СТЕНОЧКА СЛЕВА
		3.7f, 4.0f, 2.6f, 1, 0,
		3.7f, 4.0f, -2.3f, 0, 0,
		3.3f, 4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(3.7f, 4.0f, 2.6f, 1, 1,	//СТЕНОЧКА СПЕРЕДИ 
		3.7f, 2.7f, 2.6f, 1, 0,
		3.7f, 2.7f, -2.3f, 0, 0,
		3.7f, 4.0f, -2.3f, 0, 1);

	drawQuadWithNormals(3.7f, 4.0f, -2.3f, 1, 1,	//СТЕНОЧКА СНИЗУ
		3.7f, 2.7f, -2.3f, 1, 0,
		3.3f, 2.7f, -2.7f, 0, 0,
		3.3f, 4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(3.3f, 4.0f, 3.0f, 1, 1,	//СТЕНОЧКА СПЕРЕДИ СВЕРХУ
		3.3f, 2.7f, 3.0f, 1, 0,
		3.7f, 2.7f, 2.6f, 0, 0,
		3.7f, 4.0f, 2.6f, 0, 1);

	Shader::DontUseShaders();

	drawQuadWithNormals(3.3f, -2.7f, 3.0f, 1, 1,	//КВАДРАТИК ЧЕРНЫЙ СВЕРХУ
		3.3f, -2.7f, 2.0f, 1, 0,
		3.3f, 2.7f, 2.0f, 0, 0,
		3.3f, 2.7f, 3.0f, 0, 1);

	drawQuadWithNormals(3.3f, 2.7f, -2.7f, 1, 1,	//КВАДРАТИК ЧЕРНЫЙ CНИЗУ
		3.3f, 2.7f, -1.7f, 1, 0,
		3.3f, -2.7f, -1.7f, 0, 0,
		3.3f, -2.7f, -2.7f, 0, 1);

	drawQuadWithNormals(3.3f, -2.7f, 2.0f, 1, 1,	//ВЕРХНИЙ ВНУТРЕННИЙ КУСОК ДВЕРИ
		2.6f, -2.7f, 2.0f, 1, 0,
		2.6f, 2.7f, 2.0f, 0, 0,
		3.3f, 2.7f, 2.0f, 0, 1);

	drawQuadWithNormals(3.3f, 2.7f, -1.7f, 1, 1,	//НИЖНИЙ ВНУТРЕННИЙ КУСОК ДВЕРИ
		2.6f, 2.7f, -1.7f, 1, 0,
		2.6f, -2.7f, -1.7f, 0, 0,
		3.3f, -2.7f, -1.7f, 0, 1);

	drawQuadWithNormals(2.6f, -2.7f, 2.0f, 1, 1,	//КВАДРАТИК ЧЕРНЫЙ СВЕРХУ СЗАДИ
		2.6f, -2.7f, 3.0f, 1, 0,
		2.6f, 2.7f, 3.0f, 0, 0,
		2.6f, 2.7f, 2.0f, 0, 1);

	bool b_light = glIsEnabled(GL_LIGHTING);
	if (b_light)
		glDisable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0);
	outer_door.Bind();

	glColor4d(0.71f, 0.396f, 0.176f, 0.75f);	//КВАДРАТИК СТЕКЛЯННЫЙ СПЕРЕДИ
	drawQuadWithNormals(3.3f, -2.7f, 2.0f, 1, 1,
		3.3f, -2.7f, -1.7f, 1, 0,
		3.3f, 2.7f, -1.7f, 0, 0,
		3.3f, 2.7f, 2.0f, 0, 1);

	glActiveTexture(GL_TEXTURE0);
	inner_door.Bind();
	drawQuadWithNormals(2.6f, -2.7f, -1.7f, 1, 1,	//КВАДРАТИК СТЕКЛЯННЫЙ СЗАДИ
		2.6f, -2.7f, 2.0f, 1, 0,
		2.6f, 2.7f, 2.0f, 0, 0,
		2.6f, 2.7f, -1.7f, 0, 1);

	glBindTexture(GL_TEXTURE_2D, 0);

	if (b_light)
		glEnable(GL_LIGHTING);

	glPopMatrix();
}

// параметры вращения модельки
float modelRotation = 0.0f;
const float rotationSpeed = 180.0f; // градусов в секунду

GLint u_isRotatingLoc = -1;
bool isRotating = false;

void updateModelAnimation(double delta_time) {
	static bool wasKeyPressed = false;
	if (gl.isKeyPressed(' ')) {
		if (!wasKeyPressed) {
			isRotating = !isRotating;
			wasKeyPressed = true;

			// обновляем uniform
			blackening_sh.UseShader();
			glUniform1iARB(u_isRotatingLoc, isRotating ? 1 : 0);
		}
	}
	else {
		wasKeyPressed = false;
	}

	if (isRotating) {
		modelRotation += rotationSpeed * delta_time;
		if (modelRotation >= 360.0f) {
			modelRotation -= 360.0f;
		}
	}
}

float modelZPosition = 0.0f;
float modelScale = 0.1f;
float moveSpeed = 1.0f;    
float scaleSpeed = 0.2f;  

void updateModelPositionAndScale(double delta_time) {
	// перемещение по Z (W/S)
	if (gl.isKeyPressed('W')) {
		modelZPosition += moveSpeed * delta_time;
	}
	if (gl.isKeyPressed('S')) {
		modelZPosition -= moveSpeed * delta_time;
	}

	// масштабирование (Q/E)
	if (gl.isKeyPressed('E')) {
		modelScale += scaleSpeed * delta_time;
	}
	if (gl.isKeyPressed('Q')) {
		modelScale -= scaleSpeed * delta_time;
		if (modelScale < 0.01f) modelScale = 0.01f;
	}
}

void drawModel() {

	glPushMatrix();

	glRotated(modelRotation, 0, 0, 1);

	Shader::DontUseShaders(); //ТАРЕЛОЧКА

	const int number = 20;
	double radius = 2.05;
	double height = 0.1;

	glActiveTexture(GL_TEXTURE0);
	glass.Bind();
	glColor4d(1.0, 1.0, 1.0, 0.4);

	bool b_light = glIsEnabled(GL_LIGHTING);
	if (b_light)
		glDisable(GL_LIGHTING);

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		double x = radius * cos(angle);
		double y = radius * sin(angle);

		double normal[] = { x / radius, y / radius, 0.0 };
		glNormal3dv(normal);

		glTexCoord2d((double)i / number, 0.0);
		glVertex3d(x, y, 0.0);

		glTexCoord2d((double)i / number, 1.0);
		glVertex3d(x, y, height);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, -1.0);
	glTexCoord2d(0.5, 0.5);
	glVertex3d(0.0, 0.0, 0.0);
	for (int i = 0; i <= number; i++) {
		double angle = i * (2 * 3.1416 / number);
		glTexCoord2d(0.5 + 0.5 * cos(angle), 0.5 + 0.5 * sin(angle));
		glVertex3d(radius * cos(angle), radius * sin(angle), 0.0);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0.0, 0.0, 1.0);
	glTexCoord2d(0.5, 0.5);
	glVertex3d(0.0, 0.0, height);
	for (int i = number; i >= 0; i--) {
		double angle = i * (2 * 3.1416 / number);
		glTexCoord2d(0.5 + 0.5 * cos(angle), 0.5 + 0.5 * sin(angle));
		glVertex3d(radius * cos(angle), radius * sin(angle), height);
	}
	glEnd();

	if (b_light)
		glEnable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopMatrix();


	glPushMatrix(); //МОДЕЛЬКА
	glTranslated(0, 0, 0.3);
	glTranslated(0, 0, modelZPosition);
	glScaled(modelScale, modelScale, modelScale);
	glRotated(modelRotation, 0, 0, 1);

	blackening_sh.UseShader();

	glUniform1iARB(u_isRotatingLoc, isRotating ? 1 : 0);

	glActiveTexture(GL_TEXTURE0);
	model_tex.Bind();
	glActiveTexture(GL_TEXTURE1);
	black.Bind();

	blackening_sh.UseShader();

	glUniform1iARB(u_isRotatingLoc, isRotating ? 1 : 0);

	glActiveTexture(GL_TEXTURE0);
	model_tex.Bind();
	glActiveTexture(GL_TEXTURE1);
	black.Bind();

	location = glGetUniformLocationARB(blackening_sh.program, "time");
	glUniform1fARB(location, full_time);
	location = glGetUniformLocationARB(blackening_sh.program, "model_tex");
	glUniform1iARB(location, 0);
	location = glGetUniformLocationARB(blackening_sh.program, "black");
	glUniform1iARB(location, 1);

	model.Draw();

	phong_sh.UseShader();
	glShadeModel(GL_SMOOTH);
	glPopMatrix();

}

void drawMicrowave() {
	drawQuadWithNormals(2.6f, -6.0f, -2.7f, 1, 1, //ЛЕВАЯ СТЕНКА
		2.6f, -6.0f, 3.0f, 1, 0,
		-3.2f, -6.0f, 3.0f, 0, 0,
		-3.2f, -6.0f, -2.7f, 0, 1);

	drawQuadWithNormals(-3.2f, 4.0f, -2.7f, 1, 1,	//ПРАВАЯ СТЕНКА
		-3.2f, 4.0f, 3.0f, 1, 0,
		2.6f, 4.0f, 3.0f, 0, 0,
		2.6f, 4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(-3.2f, -6.0f, 3.0f, 1, 1,	//ВЕРХНЯЯ СТЕНКА
		2.6f, -6.0f, 3.0f, 1, 0,
		2.6f, 4.0f, 3.0f, 0, 0,
		-3.2f, 4.0f, 3.0f, 0, 1);

	drawQuadWithNormals(-3.2f, 4.0f, -2.7f, 1, 1,	//НИЖНЯЯ СТЕНКА
		2.6f, 4.0f, -2.7f, 1, 0,
		2.6f, -6.0f, -2.7f, 0, 0,
		-3.2f, -6.0f, -2.7f, 0, 1);

	drawQuadWithNormals(-3.2f, 4.0f, -2.7f, 1, 1,	// ЗАДНЯЯ СТЕНКА
		-3.2f, -6.0f, -2.7f, 1, 0,
		-3.2f, -6.0f, 3.0f, 0, 0,
		-3.2f, 4.0f, 3.0f, 0, 1);

	//ВЫДВИНУТАЯ ЧАСТЬ СЛЕВА ГДЕ РУЧКИ
	drawQuadWithNormals(3.2f, -5.9f, 2.9f, 1, 1,	// ВЕРХНИЙ СКЛОН
		3.2f, -4.0f, 2.9f, 1, 0,
		2.6f, -4.0f, 3.0f, 0, 0,
		2.6f, -6.0f, 3.0f, 0, 1);

	drawQuadWithNormals(2.6f, -6.0f, -2.7f, 1, 1,	// НИЖНИЙ СКЛОН
		2.6f, -4.0f, -2.7f, 1, 0,
		3.2f, -4.0f, -2.6f, 0, 0,
		3.2f, -5.9f, -2.6f, 0, 1);

	drawQuadWithNormals(2.6f, -6.0f, -2.7f, 1, 1,	// ЛЕВЫЙ СКЛОН
		3.2f, -5.9f, -2.6f, 1, 0,
		3.2f, -5.9f, 2.9f, 0, 0,
		2.6f, -6.0f, 3.0f, 0, 1);

	drawQuadWithNormals(2.6f, -4.0f, -2.7f, 1, 1,	// ПРАВАЯ МАЛЕНЬКАЯ СТЕНКА
		2.6f, -4.0f, 3.0f, 1, 0,
		3.2f, -4.0f, 2.9f, 0, 0,
		3.2f, -4.0f, -2.6f, 0, 1);

	drawQuadWithNormals(2.6f, 4.0f, -1.8f, 1, 1,	// НИЖНЯЯ ЧАСТЬ ПЕРЕДНЕЙ СТЕНКИ
		2.6f, -4.0f, -1.8f, 1, 0,
		2.6f, -4.0f, -2.7f, 0, 0,
		2.6f, 4.0f, -2.7f, 0, 1);

	drawQuadWithNormals(2.6f, 4.0f, 3.0f, 1, 1,	// ВЕРХНЯЯ ЧАСТЬ ПЕРЕДНЕЙ СТЕНКИ
		2.6f, -4.0f, 3.0f, 1, 0,
		2.6f, -4.0f, 2.1f, 0, 0,
		2.6f, 4.0f, 2.1f, 0, 1);

	drawQuadWithNormals(2.6f, -3.6f, 2.1f, 1, 1,	// ЛЕВАЯ ЧАСТЬ ПЕРЕДНЕЙ СТЕНКИ
		2.6f, -4.0f, 2.1f, 1, 0,
		2.6f, -4.0f, -1.8f, 0, 0,
		2.6f, -3.6f, -1.8f, 0, 1);

	drawQuadWithNormals(2.6f, 4.0f, 2.1f, 1, 1,	// ПРАВАЯ ЧАСТЬ ПЕРЕДНЕЙ СТЕНКИ
		2.6f, 3.6f, 2.1f, 1, 0,
		2.6f, 3.6f, -1.8f, 0, 0,
		2.6f, 4.0f, -1.8f, 0, 1);

	//ВНУТРЕННЯЯ ЧАСТЬ
	drawQuadWithNormals(-1.9f, -3.6f, -1.8f, 1, 1,	// ЛЕВАЯ ВНУТРЕННЯЯ СТЕНКА
		-1.9f, -3.6f, 2.1f, 1, 0,
		2.6f, -3.6f, 2.1f, 0, 0,
		2.6f, -3.6f, -1.8f, 0, 1);

	drawQuadWithNormals(-1.9f, -3.6f, -1.8f, 1, 1,	// ЛЕВАЯ УГЛОВАЯ ЧАСТЬ (нижняя)
		-2.6f, -2.9f, -1.1f, 1, 0,
		-2.6f, -2.9f, 1.4f, 0, 0,
		-1.9f, -3.6f, 2.1f, 0, 1);

	drawQuadWithNormals(2.6f, 3.6f, -1.8f, 1, 1,	// ПРАВАЯ ВНУТРЕННЯЯ СТЕНКА

		2.6f, 3.6f, 2.1f, 1, 0,
		-1.9f, 3.6f, 2.1f, 0, 0,
		-1.9f, 3.6f, -1.8f, 0, 1);
	drawQuadWithNormals(-1.9f, 3.6f, 2.1f, 1, 1,	// ЛЕВАЯ УГЛОВАЯ ЧАСТЬ (верхняя)

		-2.6f, 2.9f, 1.4f, 1, 0,
		-2.6f, 2.9f, -1.1f, 0, 0,
		-1.9f, 3.6f, -1.8f, 0, 1);

	drawQuadWithNormals(-1.9f, 3.6f, -1.8f, 1, 1,	// НИЖНЯЯ ВНУТРЕННЯЯ СТЕНКА
		-1.9f, -3.6f, -1.8f, 1, 0,
		2.6f, -3.6f, -1.8f, 0, 0,
		2.6f, 3.6f, -1.8f, 0, 1);

	drawQuadWithNormals(-1.9f, 3.6f, -1.8f, 1, 1,	// НИЖНЯЯ УГЛОВАЯ ЧАСТЬ
		-2.6f, 2.9f, -1.1f, 1, 0,
		-2.6f, -2.9f, -1.1f, 0, 0,
		-1.9f, -3.6f, -1.8f, 0, 1);

	drawQuadWithNormals(2.6f, 3.6f, 2.1f, 1, 1,	// ВЕРХНЯЯ ВНУТРЕННЯЯ СТЕНКА
		2.6f, -3.6f, 2.1f, 1, 0,
		-1.9f, -3.6f, 2.1f, 0, 0,
		-1.9f, 3.6f, 2.1f, 0, 1);

	drawQuadWithNormals(-1.9f, -3.6f, 2.1f, 1, 1,	// ВЕРХНЯЯ УГЛОВАЯ ЧАСТЬ
		-2.6f, -2.9f, 1.4f, 1, 0,
		-2.6f, 2.9f, 1.4f, 0, 0,
		-1.9f, 3.6f, 2.1f, 0, 1);

	drawQuadWithNormals(-2.6f, 2.9f, -1.1f, 1, 1,	// ЗАДНЯЯ ВНУТРЕННЯЯ СТЕНКА
		-2.6f, 2.9f, 1.4f, 1, 0,
		-2.6f, -2.9f, 1.4f, 0, 0,
		-2.6f, -2.9f, -1.1f, 0, 1);
}

void initRender()
{
	phong_sh.VshaderFileName = "shaders/v.vert";
	phong_sh.FshaderFileName = "shaders/light.frag";
	phong_sh.LoadShaderFromFile();
	phong_sh.Compile();

	blackening_sh.VshaderFileName = "shaders/v.vert";
	blackening_sh.FshaderFileName = "shaders/vb.frag";
	blackening_sh.LoadShaderFromFile();
	blackening_sh.Compile();


	u_isRotatingLoc = glGetUniformLocationARB(blackening_sh.program, "u_isRotating");
	if (u_isRotatingLoc == -1) {
		MessageBoxA(0, "Uniform u_isRotating not found!", "Shader Error", 0);
	}

	model_tex.LoadTexture("textures/model.jpg"); //МОЖНО ЗАМЕНИТЬ

	coffepot_tex.LoadTexture("textures/coffepot.png");
	black.LoadTexture("textures/black.png");
	outer_door.LoadTexture("textures/grid.png");
	inner_door.LoadTexture("textures/gridd.png");
	glass.LoadTexture("textures/glass.jpg");
	panel.LoadTexture("textures/panel.png");
	tile.LoadTexture("textures/tile.jpg");


	model.LoadModel("models//gun.obj_m"); //МОЖНО ЗАМЕНИТЬ

	coffepot.LoadModel("models//coffepot.obj_m");
	//==============НАСТРОЙКА ТЕКСТУР================
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);



	//================НАСТРОЙКА КАМЕРЫ======================
	camera.caclulateCameraPos();

	//привязываем камеру к событиям "движка"
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	//==============НАСТРОЙКА СВЕТА===========================
	//привязываем свет к событиям "движка"
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	//========================================================
	//====================Прочее==============================
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);
	//========================================================


	camera.setPosition(19, 0.225, 5.6);

}

void Render(double delta_time)
{
	full_time += delta_time;

	if (gl.isKeyPressed('F'))
	{
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();

	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

	light.SetUpLight();

	gl.DrawAxes();

	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	if (lightning)
		glEnable(GL_LIGHTING);
	if (texturing)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0); //сбрасываем текущую текстуру
	}

	if (alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//=============НАСТРОЙКА МАТЕРИАЛА==============
	setWhiteMatteMaterial();

	//============ РИСОВАТЬ ТУТ ==============

	glActiveTexture(GL_TEXTURE0); //ПАНЕЛЬ
	panel.Bind();
	glColor3d(1.0f, 1.0f, 1.0f);
	drawQuadWithNormals(
		3.2f, -4.0f, -2.6f, 1, 0,
		3.2f, -4.0f, 2.9f, 0, 0,
		3.2f, -5.9f, 2.9f, 0, 1,
		3.2f, -5.9f, -2.6f, 1, 1
	);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0); //СТОЛЕШНИЦА
	tile.Bind();
	glColor3d(1.0f, 1.0f, 1.0f);
	drawQuadWithNormals(4.8f, -8.0f, -3.0f, 1, 1,
		4.8f, 8.0f, -3.0f, 1, 0,
		-4.0f, 8.0f, -3.0f, 0, 0,
		-4.0f, -8.0f, -3.0f, 0, 1);
	glBindTexture(GL_TEXTURE_2D, 0);


	setBlackMaterial();
	phong_sh.UseShader();

	float light_pos[4] = { light.x(),light.y(), light.z(), 1 };
	float light_pos_v[4];

	//переносим координаты света в видовые координаты
	MatrixMultiply<float, 1, 4, 4, 4>(light_pos, view_matrix, light_pos_v);

	location = glGetUniformLocationARB(phong_sh.program, "Ia");
	glUniform3fARB(location, 1.0f, 1.0f, 1.0f);
	location = glGetUniformLocationARB(phong_sh.program, "Id");
	glUniform3fARB(location, 0.9f, 0.9f, 0.9f);
	location = glGetUniformLocationARB(phong_sh.program, "Is");
	glUniform3fARB(location, 0.7f, 0.7f, 0.7f);

	location = glGetUniformLocationARB(phong_sh.program, "ma");
	glUniform3fARB(location, 0.22f, 0.22f, 0.22f);
	location = glGetUniformLocationARB(phong_sh.program, "md");
	glUniform3fARB(location, 0.70f, 0.71f, 0.71f);
	location = glGetUniformLocationARB(phong_sh.program, "ms");
	glUniform4fARB(location, 0.12f, 0.12f, 0.13f, 24.0f);

	location = glGetUniformLocationARB(phong_sh.program, "light_pos_v");
	glUniform3fvARB(location, 1, light_pos_v);

	glPushMatrix();
	drawMicrowave();
	glPopMatrix();

	glPushMatrix();
	glTranslated(3.2, -4.945, 0.35);
	ruchka();
	glTranslated(0, 0, -1.35);
	ruchka();
	glPopMatrix();

	glPushMatrix();
	glTranslated(3.3, -4, 1.1);
	dvernyie_petli();
	glTranslated(0, 0, -3.5);
	dvernyie_petli();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, -1.9);
	krutilka();
	glPopMatrix();

	Shader::DontUseShaders();
	glPushMatrix();
	glTranslated(-2.6, -5.4, -3);
	nozhka();
	glTranslated(0, 8.8, 0);
	nozhka();
	glTranslated(4.65, 0, 0);
	nozhka();
	glTranslated(0, -8.8, 0);
	nozhka();
	glPopMatrix();

	glPushMatrix();
	updateModelAnimation(delta_time);
	updateModelPositionAndScale(delta_time);
	glTranslated(0, 0, -1.601);
	drawModel();
	glPopMatrix();

	glPushMatrix();
	updateDoorAnimation(delta_time);
	drawDoor();
	glPopMatrix();

	setWhiteShinyMaterial();
	glPushMatrix();
	Shader::DontUseShaders();
	glActiveTexture(GL_TEXTURE0);
	coffepot_tex.Bind();
	glShadeModel(GL_SMOOTH);
	glRotated(60, 0, 0, 1);
	glTranslated(1, 1, 3.0);
	glScaled(1.4, 1.4, 1.4);
	glRotated(180, 0, 0, 1);
	coffepot.Draw();
	glPopMatrix();


	//===============================================


	//сбрасываем все трансформации
	glLoadIdentity();
	camera.SetUpCamera();
	Shader::DontUseShaders();
	//рисуем источник света
	light.DrawLightGizmo();

	//================Сообщение в верхнем левом углу=======================
	glActiveTexture(GL_TEXTURE0);
	//переключаемся на матрицу проекции
	glMatrixMode(GL_PROJECTION);
	//сохраняем текущую матрицу проекции с перспективным преобразованием
	glPushMatrix();
	//загружаем единичную матрицу в матрицу проекции
	glLoadIdentity();

	//устанавливаем матрицу паралельной проекции
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

	//переключаемся на моделвью матрицу
	glMatrixMode(GL_MODELVIEW);
	//сохраняем матрицу
	glPushMatrix();
	//сбразываем все трансформации и настройки камеры загрузкой единичной матрицы
	glLoadIdentity();

	//отрисованное тут будет визуалзироватся в 2д системе координат
	//нижний левый угол окна - точка (0,0)
	//верхний правый угол (ширина_окна - 1, высота_окна - 1)


	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "O - " << (isOpening ? L"[открытие]/закрытие" : L"открытие/[закрытие]") << L" двери" << std::endl;
	ss << "SPACE - " << (isRotating ? L"[вкл]выкл  " : L" вкл[выкл] ") << L" вращение объекта" << std::endl;
	ss << L"W/S - перемещение объекта вверх/вниз по оси z" << std::endl;
	ss << L"Q/E - масштабирование объекта уменьшение/увеличение" << std::endl;
	ss << L"F - Свет из камеры" << std::endl;
	ss << L"G - двигать свет по горизонтали" << std::endl;
	ss << L"Коорд. света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
	ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;
	ss << L"delta_time: " << std::setprecision(5) << delta_time << L", full_time: " << std::setprecision(2) << full_time << std::endl << std::endl;


	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());

	text.Draw();

	//восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


}



